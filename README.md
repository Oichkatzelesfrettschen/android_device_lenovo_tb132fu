# Lenovo TB132FU Android 17 device product

This is the hardware-complete native Android 17 device product for the Wi-Fi
Lenovo Tab P11 Pro (2nd Gen), model TB132FU. It replaces `system`,
`system_ext`, and `product` while retaining the accepted non-GKI boot pipeline
and stock vendor/ODM/firmware/calibration partitions.

The published product is intentionally plain `aosp_tb132fu`. It contains no
Google application packages, private signing keys, or private release inputs.
Those may be composed in a separate local product layer without changing this
device-owned hardware tree.

Supported lunch targets are `aosp_tb132fu-cp2a-userdebug` and
`aosp_tb132fu-cp2a-user`. The open tree keeps AOSP-sized logical-image ceilings;
larger private package compositions must define and validate their own measured
partition ceilings rather than silently resizing the tablet's saved super
geometry.

## Required kernel

Pure Pixel 17 R6 and this Android 17 device product use the separately
maintained TB132FU Kernel #52. R6 retains the physically qualified R5 kernel
implementation unchanged:

- release source: [`tb132fu-purepixel17-r6-k52`](https://github.com/error311/tb132fu-kernel/tree/tb132fu-purepixel17-r6-k52)
- qualified kernel-source commit: `6eadba53b576`
- kernel release: `4.19.325-cip135-st19+ #52`

Kernel #52 retains the Kernel #17 hardware, charging, autofocus, and ABI
baseline plus the physically qualified K51 display/audio implementation. It
restores the TB132FU DisplayPort transmitter, produces correct-pitch 48 kHz
HDMI stereo with independent left/right channels, rejects the unsafe 4K30
path, and offers 4K60 only when the trained sink/link reports both DSC and FEC.
Its `-st19` suffix is the imported CIP staging level; `#52` is the independent
TB132FU build counter.

This device tree intentionally sets `TARGET_NO_KERNEL := true`: it builds the
framework-owned Android images but does not assemble `boot.img`. A flashable
TB132FU boot image also requires the matching kernel configuration, DTB,
ramdisk, Android boot-header geometry, and AVB metadata. Follow the kernel
repository's build documentation and the matching ROM release process rather
than flashing a standalone `Image` or `Image.gz`.

The `user` variant also sets `ro.adb.secure=1`; USB debugging remains a user
choice, and enabling it on the unlocked tablet still requires an authorized
host key.

The device product expects the exact stock Lenovo pen service and two pen
notification sounds as hash-pinned, locally supplied proprietary inputs. They
are intentionally ignored by Git; see `prebuilt/README.md` for the required
identities. The product does not inherit the old GSI product or TrebleDroid to
obtain those hardware components.

The panel policy exposes Standard (60 Hz), Fluid (content-aware 60–120 Hz), and
Force 120 Hz modes. Fluid keeps the framework default render ceiling at 120 Hz
so SurfaceFlinger's touch/content policy can select both physical modes; the
user's peak and minimum settings provide the Standard and Force 120 clamps.

Pure Pixel also integrates LineageOS Aperture and Updater plus Evolution-X
FaceUnlock. R6 retains Aperture source commit `de12d69e0ffc` and labels it Camera
in the launcher. The updater is adapted to generate and install signed full
recovery packages on active slot A; it does not use Android's A/B payload
engine, prepare slot B, or replace retained Lenovo partitions. Face Unlock is
exposed as a convenience biometric and is not a substitute for a PIN or a
hardware-backed strong biometric.

Double-tap-to-wake is bridged from the persistent TB132FU setting to the stock
Goodix `/proc/gesture_control` node by init. Device policy gives that exact proc
node its own SELinux type instead of granting init access to generic proc files.

This is a native TB132FU product definition; it does not depend on PHH,
TrebleDroid, or `gsi_release.mk`. It emits only the framework-owned `system`,
`system_ext`, and `product` images. Lenovo's existing vendor, ODM, bootloader,
firmware, persist, NVRAM/NVDATA, and calibration partitions remain preserved
device inputs and are outside this repository's build and flashing scope.

## Source and release boundary

This repository contains the open TB132FU device integration only. Pure Pixel
17 R6 additionally uses Android platform compatibility topics, the separately
published TB132FU kernel, locally supplied proprietary Lenovo payloads, and a
private optional Google/Pixel package composition. The device tree alone is
therefore useful for native ROM bring-up and collaboration, but is not a
complete byte-for-byte Pure Pixel build manifest.

R6 retains the complete R5 Bluetooth, charging, HDMI stereo, external-display,
camera, vendor-hardening, landscape startup, and AOD baseline. Its open device
additions include the adaptive Pure Pixel light/dark wallpaper, standard live
wallpaper feature declaration, fresh-user pen-IME opt-in policy, Android's
Multiple users UI, Pixel model-service association declarations, and an
explicit consumer ADB-watchdog override. The matching Google packages, boot
animation, and hash-pinned proprietary audio delegate are not distributed
here.

The R6 release package keeps the R3 installer geometry hotfix and uses one
standard installer for both `p11_pro_2gen` and `p11_pro_2gen_ROW_WIFI` when the
tablet is active on slot A. The separately guarded ROW_WIFI migration remains
only for an unlocked stock `p11_pro_2gen_ROW_WIFI` tablet that starts on slot
B; it preserves B and writes the complete matched image set only to A. R6 also
accepts the known official `TB132FU`, `TB132FU_PRC`, and `TB132FU_ROW`
fastbootd identities, retries a missed USB transport transition exactly once,
and can safely resume from either supported fastboot transport. Product,
unlock, slot, logical-partition, image-hash, and geometry checks remain
fail-closed before writes.

The R6 release branch is `purepixel17-tb132fu-r6`; R5, R4, R3, R2, and R1
release refs remain available for previous releases. Source files in this
repository are provided under Apache-2.0 unless an individual file states
otherwise. No Lenovo or Google proprietary payload is licensed or distributed
by this repository.
