/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * TB132FU's retained MediaTek DisplayPort path consumes stereo PCM frames at
 * approximately twice the 48 kHz rate advertised to AudioFlinger.  Kernel
 * channel-count experiments proved that collapsing the link to one hardware
 * channel restores time/pitch only by serializing L and R as alternating mono
 * samples.  Keep the working two-channel transport and compensate at the last
 * userspace boundary instead: write each complete stereo frame twice and map
 * the delegate's physical frame counters back to logical frames.
 *
 * This module is deliberately a narrow wrapper around the exact audited
 * PCM16 Lenovo HAL.  It passes every non-HDMI stream through unchanged and
 * refuses HDMI unless the complete contract is PCM16/stereo/48 kHz with no
 * special output flags.  Unknown delegate binaries and partial contracts fail
 * closed instead of falling back to the known chipmunk path.
 */

#define LOG_TAG "TB132FUHdmiFrameFix"

#include <android/dlext.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <hardware/audio.h>
#include <log/log.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#define DELEGATE_PATH "/vendor/lost+found/audio.primary.mt6893.pcm16.so"
#define DELEGATE_DLOPEN_NAME "audio.primary.mt6893.tb132fu-delegate.so"
#define DELEGATE_SIZE 2592096
#define DELEGATE_BUILD_ID_OFFSET 0x19c

#define LOGICAL_FRAME_BYTES 4U
#define PHYSICAL_FRAMES_PER_LOGICAL_FRAME 2U
#define EXPANDED_FRAME_BYTES \
    (LOGICAL_FRAME_BYTES * PHYSICAL_FRAMES_PER_LOGICAL_FRAME)

typedef int (*open_output_stream_fn)(struct audio_hw_device *,
                                     audio_io_handle_t, audio_devices_t,
                                     audio_output_flags_t,
                                     struct audio_config *,
                                     struct audio_stream_out **,
                                     const char *);
typedef void (*close_output_stream_fn)(struct audio_hw_device *,
                                       struct audio_stream_out *);
typedef int (*device_close_fn)(struct hw_device_t *);
typedef ssize_t (*stream_write_fn)(struct audio_stream_out *, const void *,
                                   size_t);
typedef uint32_t (*stream_get_latency_fn)(const struct audio_stream_out *);
typedef int (*stream_get_render_position_fn)(const struct audio_stream_out *,
                                             uint32_t *);
typedef int (*stream_get_presentation_position_fn)(
        const struct audio_stream_out *, uint64_t *, struct timespec *);

struct stream_context {
    struct audio_stream_out *stream;
    stream_write_fn delegate_write;
    stream_get_latency_fn delegate_get_latency;
    stream_get_render_position_fn delegate_get_render_position;
    stream_get_presentation_position_fn delegate_get_presentation_position;
    pthread_mutex_t io_lock;
    uint8_t *scratch;
    size_t scratch_size;
    bool first_write_logged;
    struct stream_context *next;
};

struct device_context {
    struct audio_hw_device *device;
    open_output_stream_fn delegate_open_output_stream;
    close_output_stream_fn delegate_close_output_stream;
    device_close_fn delegate_close;
};

static pthread_mutex_t g_delegate_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_device_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_streams_lock = PTHREAD_MUTEX_INITIALIZER;
static struct audio_module *g_delegate_module;
static void *g_delegate_handle;
static struct device_context g_device;
static struct stream_context *g_streams;

extern struct audio_module HAL_MODULE_INFO_SYM;

struct checked_word {
    off_t offset;
    uint8_t bytes[4];
};

static const uint8_t expected_build_id[16] = {
    0x9e, 0x8a, 0x45, 0x6a, 0x98, 0x89, 0x2d, 0x2f,
    0xbe, 0xfb, 0x92, 0xfc, 0x12, 0x50, 0x0b, 0x9d,
};

static const struct checked_word expected_delegate_words[] = {
    /* Wrong-handler HDMI::open() sites must remain stock PCM 8.24. */
    { 0x11ec4c, { 0x04, 0x30, 0xa0, 0xe3 } },
    { 0x11ec88, { 0x04, 0x10, 0xa0, 0xe3 } },
    /* Active AudioALSAPlaybackHandlerTVOut sites must be PCM16. */
    { 0x11fc58, { 0x01, 0x20, 0xa0, 0xe3 } },
    { 0x11fcd4, { 0x01, 0x10, 0xa0, 0xe3 } },
};

static int pread_exact(int fd, void *buffer, size_t size, off_t offset)
{
    uint8_t *out = buffer;
    size_t done = 0;

    while (done < size) {
        ssize_t count = pread(fd, out + done, size - done,
                              offset + (off_t)done);

        if (count < 0) {
            if (errno == EINTR)
                continue;
            return -errno;
        }
        if (count == 0)
            return -EIO;
        done += (size_t)count;
    }
    return 0;
}

static int open_verified_delegate(void)
{
    static const uint8_t elf_identity[] = {
        0x7f, 'E', 'L', 'F', 1, 1, 1,
    };
    static const uint8_t elf_arm_machine[] = { 0x28, 0x00 };
    uint8_t observed[sizeof(expected_build_id)];
    android_dlextinfo extinfo = {
        .flags = ANDROID_DLEXT_USE_LIBRARY_FD | ANDROID_DLEXT_FORCE_LOAD,
    };
    struct stat st;
    struct statvfs vfs;
    struct audio_module *module;
    void *handle;
    size_t i;
    int fd;
    int result = -EINVAL;

    pthread_mutex_lock(&g_delegate_lock);
    if (g_delegate_module != NULL) {
        pthread_mutex_unlock(&g_delegate_lock);
        return 0;
    }

    fd = open(DELEGATE_PATH, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        ALOGE("delegate open refused: %s", strerror(errno));
        result = -errno;
        goto out_unlock;
    }
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) ||
        st.st_size != DELEGATE_SIZE) {
        ALOGE("delegate identity refused: wrong type or size");
        goto out_close;
    }
    if (fstatvfs(fd, &vfs) != 0 || !(vfs.f_flag & ST_RDONLY)) {
        ALOGE("delegate identity refused: source is not read-only");
        goto out_close;
    }
    if (pread_exact(fd, observed, sizeof(elf_identity), 0) != 0 ||
        memcmp(observed, elf_identity, sizeof(elf_identity)) != 0 ||
        pread_exact(fd, observed, sizeof(elf_arm_machine), 18) != 0 ||
        memcmp(observed, elf_arm_machine, sizeof(elf_arm_machine)) != 0) {
        ALOGE("delegate identity refused: not audited ARM32 ELF");
        goto out_close;
    }
    if (pread_exact(fd, observed, sizeof(expected_build_id),
                    DELEGATE_BUILD_ID_OFFSET) != 0 ||
        memcmp(observed, expected_build_id, sizeof(expected_build_id)) != 0) {
        ALOGE("delegate identity refused: GNU build ID mismatch");
        goto out_close;
    }
    for (i = 0; i < sizeof(expected_delegate_words) /
                    sizeof(expected_delegate_words[0]); ++i) {
        if (pread_exact(fd, observed,
                        sizeof(expected_delegate_words[i].bytes),
                        expected_delegate_words[i].offset) != 0 ||
            memcmp(observed, expected_delegate_words[i].bytes,
                   sizeof(expected_delegate_words[i].bytes)) != 0) {
            ALOGE("delegate identity refused: instruction mismatch at 0x%lx",
                  (unsigned long)expected_delegate_words[i].offset);
            goto out_close;
        }
    }

    extinfo.library_fd = fd;
    handle = android_dlopen_ext(DELEGATE_DLOPEN_NAME, RTLD_NOW | RTLD_LOCAL,
                                &extinfo);
    if (handle == NULL) {
        ALOGE("verified delegate load failed: %s", dlerror());
        goto out_close;
    }
    module = (struct audio_module *)dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (module == NULL || module->common.tag != HARDWARE_MODULE_TAG ||
        module->common.id == NULL ||
        strcmp(module->common.id, AUDIO_HARDWARE_MODULE_ID) != 0 ||
        module->common.methods == NULL ||
        module->common.methods->open == NULL) {
        ALOGE("verified delegate has an invalid audio HMI");
        dlclose(handle);
        goto out_close;
    }

    g_delegate_handle = handle;
    g_delegate_module = module;
    result = 0;
    ALOGI("exact PCM16 Lenovo delegate verified and loaded");

out_close:
    close(fd);
out_unlock:
    pthread_mutex_unlock(&g_delegate_lock);
    return result;
}

static struct stream_context *lock_stream_context(
        const struct audio_stream_out *stream)
{
    struct stream_context *context;

    pthread_mutex_lock(&g_streams_lock);
    for (context = g_streams; context != NULL; context = context->next) {
        if (context->stream == stream) {
            pthread_mutex_lock(&context->io_lock);
            break;
        }
    }
    pthread_mutex_unlock(&g_streams_lock);
    return context;
}

static ssize_t framefix_write(struct audio_stream_out *stream,
                              const void *buffer, size_t bytes)
{
    const uint8_t *input = buffer;
    struct stream_context *context = lock_stream_context(stream);
    size_t logical_done = 0;
    ssize_t result = -ENODEV;

    if (context == NULL)
        return -ENODEV;
    if (bytes == 0) {
        result = 0;
        goto out;
    }
    if (buffer == NULL || bytes % LOGICAL_FRAME_BYTES != 0) {
        ALOGE("HDMI write refused: %zu bytes is not whole PCM16 stereo frames",
              bytes);
        result = -EINVAL;
        goto out;
    }

    while (logical_done < bytes) {
        size_t logical_remaining = bytes - logical_done;
        size_t logical_capacity =
                context->scratch_size / PHYSICAL_FRAMES_PER_LOGICAL_FRAME;
        size_t logical_chunk = logical_remaining < logical_capacity
                ? logical_remaining : logical_capacity;
        size_t physical_target;
        size_t physical_done = 0;
        size_t frame;

        logical_chunk -= logical_chunk % LOGICAL_FRAME_BYTES;
        if (logical_chunk == 0) {
            ALOGE("HDMI write refused: delegate buffer cannot hold one frame pair");
            result = logical_done != 0 ? (ssize_t)logical_done : -EIO;
            goto out;
        }

        for (frame = 0; frame < logical_chunk / LOGICAL_FRAME_BYTES; ++frame) {
            const uint8_t *source = input + logical_done +
                    frame * LOGICAL_FRAME_BYTES;
            uint8_t *target = context->scratch + frame * EXPANDED_FRAME_BYTES;

            memcpy(target, source, LOGICAL_FRAME_BYTES);
            memcpy(target + LOGICAL_FRAME_BYTES, source, LOGICAL_FRAME_BYTES);
        }
        physical_target = logical_chunk * PHYSICAL_FRAMES_PER_LOGICAL_FRAME;

        while (physical_done < physical_target) {
            ssize_t written = context->delegate_write(
                    stream, context->scratch + physical_done,
                    physical_target - physical_done);

            if (written <= 0 || (size_t)written > physical_target - physical_done ||
                (size_t)written % LOGICAL_FRAME_BYTES != 0) {
                size_t complete =
                        (physical_done / EXPANDED_FRAME_BYTES) *
                        LOGICAL_FRAME_BYTES;

                ALOGE("delegate HDMI write failed after %zu/%zu physical bytes: %zd",
                      physical_done, physical_target, written);
                result = logical_done + complete != 0
                        ? (ssize_t)(logical_done + complete)
                        : (written < 0 ? written : -EIO);
                goto out;
            }
            physical_done += (size_t)written;
        }
        logical_done += logical_chunk;
    }

    if (!context->first_write_logged) {
        context->first_write_logged = true;
        ALOGI("HDMI frame-clock correction active: PCM16 stereo 48 kHz, 2 physical frames per logical frame");
    }
    result = (ssize_t)logical_done;

out:
    pthread_mutex_unlock(&context->io_lock);
    return result;
}

static uint32_t framefix_get_latency(const struct audio_stream_out *stream)
{
    struct stream_context *context = lock_stream_context(stream);
    uint32_t latency = 0;

    if (context == NULL)
        return 0;
    if (context->delegate_get_latency != NULL) {
        latency = context->delegate_get_latency(stream);
        latency = (latency + PHYSICAL_FRAMES_PER_LOGICAL_FRAME - 1U) /
                PHYSICAL_FRAMES_PER_LOGICAL_FRAME;
    }
    pthread_mutex_unlock(&context->io_lock);
    return latency;
}

static int framefix_get_render_position(const struct audio_stream_out *stream,
                                        uint32_t *dsp_frames)
{
    struct stream_context *context = lock_stream_context(stream);
    int result = -ENODEV;

    if (context == NULL)
        return -ENODEV;
    if (context->delegate_get_render_position != NULL) {
        result = context->delegate_get_render_position(stream, dsp_frames);
        if (result == 0 && dsp_frames != NULL)
            *dsp_frames /= PHYSICAL_FRAMES_PER_LOGICAL_FRAME;
    } else {
        result = -ENOSYS;
    }
    pthread_mutex_unlock(&context->io_lock);
    return result;
}

static int framefix_get_presentation_position(
        const struct audio_stream_out *stream, uint64_t *frames,
        struct timespec *timestamp)
{
    struct stream_context *context = lock_stream_context(stream);
    int result = -ENODEV;

    if (context == NULL)
        return -ENODEV;
    if (context->delegate_get_presentation_position != NULL) {
        result = context->delegate_get_presentation_position(
                stream, frames, timestamp);
        if (result == 0 && frames != NULL)
            *frames /= PHYSICAL_FRAMES_PER_LOGICAL_FRAME;
    } else {
        result = -ENOSYS;
    }
    pthread_mutex_unlock(&context->io_lock);
    return result;
}

static void destroy_unpublished_context(struct stream_context *context)
{
    if (context == NULL)
        return;
    pthread_mutex_destroy(&context->io_lock);
    free(context->scratch);
    free(context);
}

static int create_stream_context(struct audio_stream_out *stream,
                                 struct stream_context **context_out)
{
    struct stream_context *context;
    size_t delegate_buffer_size;

    if (stream == NULL || stream->write == NULL ||
        stream->common.get_buffer_size == NULL ||
        stream->common.get_sample_rate == NULL ||
        stream->common.get_channels == NULL ||
        stream->common.get_format == NULL ||
        stream->common.get_sample_rate(&stream->common) != 48000 ||
        stream->common.get_channels(&stream->common) !=
                AUDIO_CHANNEL_OUT_STEREO ||
        stream->common.get_format(&stream->common) !=
                AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("HDMI stream refused: delegate did not preserve PCM16/stereo/48 kHz");
        return -EINVAL;
    }

    delegate_buffer_size = stream->common.get_buffer_size(&stream->common);
    delegate_buffer_size -= delegate_buffer_size % EXPANDED_FRAME_BYTES;
    if (delegate_buffer_size < EXPANDED_FRAME_BYTES) {
        ALOGE("HDMI stream refused: invalid delegate buffer size");
        return -EINVAL;
    }

    context = calloc(1, sizeof(*context));
    if (context == NULL)
        return -ENOMEM;
    context->scratch = malloc(delegate_buffer_size);
    if (context->scratch == NULL) {
        free(context);
        return -ENOMEM;
    }
    if (pthread_mutex_init(&context->io_lock, NULL) != 0) {
        free(context->scratch);
        free(context);
        return -ENOMEM;
    }

    context->stream = stream;
    context->delegate_write = stream->write;
    context->delegate_get_latency = stream->get_latency;
    context->delegate_get_render_position = stream->get_render_position;
    context->delegate_get_presentation_position =
            stream->get_presentation_position;
    context->scratch_size = delegate_buffer_size;
    *context_out = context;
    return 0;
}

static int framefix_open_output_stream(struct audio_hw_device *device,
                                       audio_io_handle_t handle,
                                       audio_devices_t devices,
                                       audio_output_flags_t flags,
                                       struct audio_config *config,
                                       struct audio_stream_out **stream_out,
                                       const char *address)
{
    struct stream_context *context = NULL;
    open_output_stream_fn delegate_open;
    close_output_stream_fn delegate_close;
    int result;

    pthread_mutex_lock(&g_device_lock);
    if (g_device.device != device) {
        pthread_mutex_unlock(&g_device_lock);
        return -ENODEV;
    }
    delegate_open = g_device.delegate_open_output_stream;
    delegate_close = g_device.delegate_close_output_stream;
    pthread_mutex_unlock(&g_device_lock);

    if ((devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) == 0)
        return delegate_open(device, handle, devices, flags, config,
                             stream_out, address);

    if (devices != AUDIO_DEVICE_OUT_AUX_DIGITAL ||
        flags != AUDIO_OUTPUT_FLAG_NONE || config == NULL ||
        stream_out == NULL ||
        config->sample_rate != 48000 ||
        config->channel_mask != AUDIO_CHANNEL_OUT_STEREO ||
        config->format != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("HDMI open refused: devices=%#llx flags=%#x rate=%u channels=%#x format=%#x",
              (unsigned long long)devices, (unsigned int)flags,
              config != NULL ? config->sample_rate : 0,
              config != NULL ? (unsigned int)config->channel_mask : 0,
              config != NULL ? (unsigned int)config->format : 0);
        return -EINVAL;
    }

    result = delegate_open(device, handle, devices, flags, config,
                           stream_out, address);
    if (result != 0)
        return result;
    if (stream_out == NULL || *stream_out == NULL) {
        ALOGE("HDMI open refused: delegate returned no stream");
        return -EIO;
    }

    result = create_stream_context(*stream_out, &context);
    if (result != 0) {
        delegate_close(device, *stream_out);
        *stream_out = NULL;
        return result;
    }

    pthread_mutex_lock(&g_streams_lock);
    {
        struct stream_context *existing;

        for (existing = g_streams; existing != NULL; existing = existing->next) {
            if (existing->stream == *stream_out)
                break;
        }
        if (existing != NULL) {
            pthread_mutex_unlock(&g_streams_lock);
            destroy_unpublished_context(context);
            delegate_close(device, *stream_out);
            *stream_out = NULL;
            ALOGE("HDMI open refused: delegate reused an active stream object");
            return -EBUSY;
        }

        context->next = g_streams;
        g_streams = context;
        (*stream_out)->write = framefix_write;
        if (context->delegate_get_latency != NULL)
            (*stream_out)->get_latency = framefix_get_latency;
        if (context->delegate_get_render_position != NULL)
            (*stream_out)->get_render_position = framefix_get_render_position;
        if (context->delegate_get_presentation_position != NULL)
            (*stream_out)->get_presentation_position =
                    framefix_get_presentation_position;
    }
    pthread_mutex_unlock(&g_streams_lock);

    ALOGI("HDMI stream armed: exact PCM16/stereo/48 kHz contract, buffer=%zu",
          context->scratch_size);
    return 0;
}

static void framefix_close_output_stream(struct audio_hw_device *device,
                                         struct audio_stream_out *stream)
{
    struct stream_context **link;
    struct stream_context *context = NULL;
    close_output_stream_fn delegate_close;

    pthread_mutex_lock(&g_device_lock);
    delegate_close = g_device.device == device
            ? g_device.delegate_close_output_stream : NULL;
    pthread_mutex_unlock(&g_device_lock);
    if (delegate_close == NULL) {
        ALOGE("output close refused for an unknown audio device");
        return;
    }

    pthread_mutex_lock(&g_streams_lock);
    for (link = &g_streams; *link != NULL; link = &(*link)->next) {
        if ((*link)->stream == stream) {
            context = *link;
            *link = context->next;
            pthread_mutex_lock(&context->io_lock);
            stream->write = context->delegate_write;
            stream->get_latency = context->delegate_get_latency;
            stream->get_render_position = context->delegate_get_render_position;
            stream->get_presentation_position =
                    context->delegate_get_presentation_position;
            break;
        }
    }
    pthread_mutex_unlock(&g_streams_lock);

    delegate_close(device, stream);
    if (context != NULL) {
        pthread_mutex_unlock(&context->io_lock);
        destroy_unpublished_context(context);
        ALOGI("HDMI frame-clock stream closed cleanly");
    }
}

static int framefix_device_close(struct hw_device_t *hardware_device)
{
    struct audio_hw_device *device = (struct audio_hw_device *)hardware_device;
    device_close_fn delegate_close;

    pthread_mutex_lock(&g_streams_lock);
    if (g_streams != NULL) {
        pthread_mutex_unlock(&g_streams_lock);
        ALOGE("primary audio close refused while corrected HDMI stream is active");
        return -EBUSY;
    }
    pthread_mutex_unlock(&g_streams_lock);

    pthread_mutex_lock(&g_device_lock);
    if (g_device.device != device) {
        pthread_mutex_unlock(&g_device_lock);
        return -ENODEV;
    }
    delegate_close = g_device.delegate_close;
    device->open_output_stream = g_device.delegate_open_output_stream;
    device->close_output_stream = g_device.delegate_close_output_stream;
    device->common.close = delegate_close;
    memset(&g_device, 0, sizeof(g_device));
    pthread_mutex_unlock(&g_device_lock);
    return delegate_close(hardware_device);
}

static int framefix_module_open(const struct hw_module_t *module,
                                const char *name,
                                struct hw_device_t **hardware_device)
{
    struct hw_device_t *delegate_hardware_device = NULL;
    struct audio_hw_device *device;
    int result;

    if (module == NULL || module != &HAL_MODULE_INFO_SYM.common ||
        name == NULL || strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0 ||
        hardware_device == NULL)
        return -EINVAL;
    *hardware_device = NULL;

    result = open_verified_delegate();
    if (result != 0)
        return result;
    result = g_delegate_module->common.methods->open(
            &g_delegate_module->common, name, &delegate_hardware_device);
    if (result != 0)
        return result;
    if (delegate_hardware_device == NULL ||
        delegate_hardware_device->tag != HARDWARE_DEVICE_TAG) {
        if (delegate_hardware_device != NULL &&
            delegate_hardware_device->close != NULL)
            delegate_hardware_device->close(delegate_hardware_device);
        return -EINVAL;
    }

    device = (struct audio_hw_device *)delegate_hardware_device;
    if (device->open_output_stream == NULL ||
        device->close_output_stream == NULL ||
        device->common.close == NULL) {
        if (device->common.close != NULL)
            device->common.close(&device->common);
        return -EINVAL;
    }

    pthread_mutex_lock(&g_device_lock);
    if (g_device.device != NULL) {
        pthread_mutex_unlock(&g_device_lock);
        device->common.close(&device->common);
        ALOGE("second primary audio device open refused");
        return -EBUSY;
    }
    g_device.device = device;
    g_device.delegate_open_output_stream = device->open_output_stream;
    g_device.delegate_close_output_stream = device->close_output_stream;
    g_device.delegate_close = device->common.close;
    device->open_output_stream = framefix_open_output_stream;
    device->close_output_stream = framefix_close_output_stream;
    device->common.close = framefix_device_close;
    pthread_mutex_unlock(&g_device_lock);

    *hardware_device = &device->common;
    ALOGI("primary audio device wrapped; non-HDMI paths remain pass-through");
    return 0;
}

static struct hw_module_methods_t framefix_module_methods = {
    .open = framefix_module_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "TB132FU HDMI frame-clock audio wrapper",
        .author = "Pure Pixel",
        .methods = &framefix_module_methods,
    },
};
