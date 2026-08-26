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

Pure Pixel 17 R4 and this Android 17 device product use the separately
maintained TB132FU Kernel #17:

- release source: [`tb132fu-purepixel17-r4-k17`](https://github.com/error311/tb132fu-kernel/tree/tb132fu-purepixel17-r4-k17)
- release-source commit: `71b4644268320`
- kernel release: `4.19.325-cip135-st19+`

Kernel #17 retains the Kernel #14 hardware and ABI baseline, including BQ27541
battery-health reporting. It restores the GT9768 rear-camera autofocus path,
reports negotiated charging capability, and enforces the native 80% charge
limit across both the main charger and independent SC8551 PPS charge pump.

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

Pure Pixel also integrates the open LineageOS Updater and Evolution-X FaceUnlock
projects listed by the platform-patches manifest. The updater is adapted to
generate and install signed full recovery packages on the active slot A. It
does not use Android's A/B payload engine, prepare slot B, or replace retained
Lenovo partitions. Face unlock is exposed as a convenience biometric and is
not a substitute for a PIN or a hardware-backed strong biometric.

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
17 R4 additionally uses Android platform compatibility topics, the separately
published TB132FU kernel, locally supplied proprietary Lenovo payloads, and a
private optional Google/Pixel package composition. The device tree alone is
therefore useful for native ROM bring-up and collaboration, but is not a
complete byte-for-byte Pure Pixel build manifest.

R4 retains the landscape startup and power-button AOD reveal behavior from R3.
It also adds the qualified Bluetooth-audio compatibility bridge, native charge
limit integration, and reversible controls for obsolete vendor services. The
matching Google boot animation is not distributed here.

The R4 release branch is `purepixel17-tb132fu-r4`; R3, R2, and R1 tags remain
available for previous releases. Source files in this repository are provided
under Apache-2.0 unless an individual file states otherwise. No Lenovo or
Google proprietary payload is licensed or distributed by this repository.
