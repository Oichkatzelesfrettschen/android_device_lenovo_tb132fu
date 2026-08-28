/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Verify Lenovo's retained 32-bit MT6893 primary audio HAL and the immutable
 * PCM16-corrected system copy, then bind-mount the corrected copy read-only.
 * Refuse every unknown input and leave the stock vendor path untouched on any
 * mismatch or mount failure.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#define HAL_SIZE 2592096
#define BUILD_ID_OFFSET 0x19c

struct instruction {
    off_t offset;
    unsigned char stock[4];
    unsigned char pcm16[4];
};

static const unsigned char expected_build_id[16] = {
    0x9e, 0x8a, 0x45, 0x6a, 0x98, 0x89, 0x2d, 0x2f,
    0xbe, 0xfb, 0x92, 0xfc, 0x12, 0x50, 0x0b, 0x9d,
};

/*
 * AUX_DIGITAL (0x400) uses AudioALSAPlaybackHandlerTVOut on TB132FU. Keep the
 * similarly named HDMI::open() sites pristine; dev06 changed those sites and
 * therefore never changed the live PCM21 S24_LE contract.
 */
static const struct instruction required_unchanged[] = {
    { 0x11ec4c, { 0x04, 0x30, 0xa0, 0xe3 },
                  { 0x04, 0x30, 0xa0, 0xe3 } },
    { 0x11ec88, { 0x04, 0x10, 0xa0, 0xe3 },
                  { 0x04, 0x10, 0xa0, 0xe3 } },
};

static const struct instruction instructions[] = {
    /* AudioALSAPlaybackHandlerTVOut::open(): stream/DP audio_format_t. */
    { 0x11fc58, { 0x04, 0x20, 0xa0, 0xe3 },
                  { 0x01, 0x20, 0xa0, 0xe3 } },
    /* TVOut::open(): audio_format_t passed to the TinyALSA converter. */
    { 0x11fcd4, { 0x04, 0x10, 0xa0, 0xe3 },
                  { 0x01, 0x10, 0xa0, 0xe3 } },
};

static int pread_all(int fd, unsigned char *data, size_t size, off_t offset)
{
    size_t done = 0;

    while (done < size) {
        ssize_t count = pread(fd, data + done, size - done,
                              offset + (off_t)done);

        if (count < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (count == 0) {
            errno = EIO;
            return -1;
        }
        done += (size_t)count;
    }
    return 0;
}

static int verify_hal(const char *path, int expect_pcm16, struct stat *st)
{
    unsigned char observed[sizeof(expected_build_id)];
    size_t i;
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    int result = -1;

    if (fd < 0 || fstat(fd, st) != 0 || !S_ISREG(st->st_mode) ||
        st->st_size != HAL_SIZE) {
        fprintf(stderr, "refusing unexpected HDMI HAL %s: %s\n", path,
                fd < 0 ? strerror(errno) : "wrong type or size");
        goto out;
    }

    if (pread_all(fd, observed, sizeof(observed), BUILD_ID_OFFSET) != 0 ||
        memcmp(observed, expected_build_id, sizeof(observed)) != 0) {
        fprintf(stderr, "refusing HDMI HAL with unexpected GNU build ID: %s\n",
                path);
        goto out;
    }

    for (i = 0; i < sizeof(required_unchanged) /
                    sizeof(required_unchanged[0]); ++i) {
        const unsigned char *expected = expect_pcm16
                ? required_unchanged[i].pcm16
                : required_unchanged[i].stock;

        if (pread_all(fd, observed, 4, required_unchanged[i].offset) != 0 ||
            memcmp(observed, expected, 4) != 0) {
            fprintf(stderr,
                    "refusing changed wrong-handler instruction at 0x%lx in %s\n",
                    (unsigned long)required_unchanged[i].offset, path);
            goto out;
        }
    }

    for (i = 0; i < sizeof(instructions) / sizeof(instructions[0]); ++i) {
        const unsigned char *expected = expect_pcm16 ? instructions[i].pcm16
                                                     : instructions[i].stock;

        if (pread_all(fd, observed, 4, instructions[i].offset) != 0 ||
            memcmp(observed, expected, 4) != 0) {
            fprintf(stderr, "refusing unexpected instruction at 0x%lx in %s\n",
                    (unsigned long)instructions[i].offset, path);
            goto out;
        }
    }

    result = 0;

out:
    if (fd >= 0)
        close(fd);
    return result;
}

int main(int argc, char **argv)
{
    struct stat stock_st;
    struct stat pcm16_st;
    struct stat mounted_st;
    struct statvfs mounted_vfs;
    const char *stock_path;
    const char *pcm16_path;
    int verify_only = 0;

    if (argc == 4 && strcmp(argv[1], "--verify-only") == 0) {
        verify_only = 1;
        stock_path = argv[2];
        pcm16_path = argv[3];
    } else if (argc == 3) {
        stock_path = argv[1];
        pcm16_path = argv[2];
    } else {
        fprintf(stderr, "usage: %s [--verify-only] STOCK_HAL PCM16_HAL\n",
                argv[0]);
        return 2;
    }

    if (verify_hal(stock_path, 0, &stock_st) != 0 ||
        verify_hal(pcm16_path, 1, &pcm16_st) != 0)
        return 1;

    if (stock_st.st_dev == pcm16_st.st_dev && stock_st.st_ino == pcm16_st.st_ino) {
        fprintf(stderr, "refusing identical stock and PCM16 HAL inodes\n");
        return 1;
    }

    if (verify_only) {
        printf("verified TB132FU stock and HDMI PCM16 HALs\n");
        return 0;
    }

    if (mount(pcm16_path, stock_path, NULL, MS_BIND, NULL) != 0) {
        fprintf(stderr, "failed to bind verified HDMI HAL: %s\n",
                strerror(errno));
        return 1;
    }

    /*
     * Do not bind-remount here. After the first bind the target inode has the
     * system_lib_file label, and Android's executable-type mounton neverallow
     * intentionally rejects a second mount operation on it. The immutable
     * AVB-backed system filesystem is already read-only. Prove that property,
     * and prove the target now resolves to the exact verified source inode.
     */
    if (verify_hal(stock_path, 1, &mounted_st) != 0 ||
        mounted_st.st_dev != pcm16_st.st_dev ||
        mounted_st.st_ino != pcm16_st.st_ino ||
        statvfs(stock_path, &mounted_vfs) != 0 ||
        !(mounted_vfs.f_flag & ST_RDONLY)) {
        umount2(stock_path, MNT_DETACH);
        fprintf(stderr,
                "refusing HDMI HAL bind without verified read-only source identity\n");
        return 1;
    }

    printf("mounted verified TB132FU HDMI PCM16 HAL\n");
    return 0;
}
