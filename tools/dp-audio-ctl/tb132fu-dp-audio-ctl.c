/* Device-side diagnostic for the MediaTek DisplayPort audio ioctls. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DRM_IOCTL_BASE 'd'
#define DRM_COMMAND_BASE 0x40
#define DRM_MTK_HDMI_AUDIO_ENABLE 0x3b
#define DRM_MTK_HDMI_AUDIO_CONFIG 0x3c
#define DRM_MTK_HDMI_GET_CAPABILITY 0x3d
#define DRM_IOCTL_MTK_HDMI_AUDIO_ENABLE \
    _IOWR(DRM_IOCTL_BASE, DRM_COMMAND_BASE + DRM_MTK_HDMI_AUDIO_ENABLE, unsigned int)
#define DRM_IOCTL_MTK_HDMI_AUDIO_CONFIG \
    _IOWR(DRM_IOCTL_BASE, DRM_COMMAND_BASE + DRM_MTK_HDMI_AUDIO_CONFIG, unsigned int)
#define DRM_IOCTL_MTK_HDMI_GET_CAPABILITY \
    _IOWR(DRM_IOCTL_BASE, DRM_COMMAND_BASE + DRM_MTK_HDMI_GET_CAPABILITY, unsigned int)

#define DP_CHANNEL_2 (1U << 0)
#define DP_CHANNEL_8 (1U << 6)
#define DP_SAMPLERATE_48 (1U << 2)
#define DP_BITWIDTH_16 (1U << 0)
#define DP_BITWIDTH_24 (1U << 2)
#define DP_CAPABILITY_SAMPLERATE_SFT 8
#define DP_CAPABILITY_BITWIDTH_SFT 16

static unsigned int dp_config(unsigned int channels, unsigned int bitwidth)
{
    return channels |
           (DP_SAMPLERATE_48 << DP_CAPABILITY_SAMPLERATE_SFT) |
           (bitwidth << DP_CAPABILITY_BITWIDTH_SFT);
}

static int run_ioctl(int fd, unsigned long request, unsigned int value,
                     const char *name)
{
    if (ioctl(fd, request, &value) != 0) {
        fprintf(stderr, "%s failed: %s\n", name, strerror(errno));
        return 1;
    }
    printf("%s: 0x%x\n", name, value);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned long request;
    unsigned int value;
    const char *name;
    int fd;
    int result;

    if (argc != 2) {
        fprintf(stderr, "usage: %s {config2|config2_16|config8|enable|disable|cap}\n", argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "config2") == 0) {
        request = DRM_IOCTL_MTK_HDMI_AUDIO_CONFIG;
        value = dp_config(DP_CHANNEL_2, DP_BITWIDTH_24);
        name = "config2";
    } else if (strcmp(argv[1], "config2_16") == 0) {
        request = DRM_IOCTL_MTK_HDMI_AUDIO_CONFIG;
        value = dp_config(DP_CHANNEL_2, DP_BITWIDTH_16);
        name = "config2_16";
    } else if (strcmp(argv[1], "config8") == 0) {
        request = DRM_IOCTL_MTK_HDMI_AUDIO_CONFIG;
        value = dp_config(DP_CHANNEL_8, DP_BITWIDTH_24);
        name = "config8";
    } else if (strcmp(argv[1], "enable") == 0) {
        request = DRM_IOCTL_MTK_HDMI_AUDIO_ENABLE;
        value = 1;
        name = "enable";
    } else if (strcmp(argv[1], "disable") == 0) {
        request = DRM_IOCTL_MTK_HDMI_AUDIO_ENABLE;
        value = 0;
        name = "disable";
    } else if (strcmp(argv[1], "cap") == 0) {
        request = DRM_IOCTL_MTK_HDMI_GET_CAPABILITY;
        value = 0;
        name = "cap";
    } else {
        fprintf(stderr, "unknown command: %s\n", argv[1]);
        return 2;
    }

    fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "open /dev/dri/card0 failed: %s\n", strerror(errno));
        return 1;
    }
    result = run_ioctl(fd, request, value, name);
    close(fd);
    return result;
}
