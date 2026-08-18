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

The `user` variant also sets `ro.adb.secure=1`; USB debugging remains a user
choice, and enabling it on the unlocked tablet still requires an authorized
host key.

The device product expects the exact stock Lenovo pen service as a hash-pinned,
locally supplied proprietary prebuilt. It is intentionally ignored by Git;
see `prebuilt/README.md` for the required identity. The product does not inherit
the old GSI product or TrebleDroid to obtain that hardware component.

The panel policy exposes Standard (60 Hz), Fluid (content-aware 60–120 Hz), and
Force 120 Hz modes. Fluid keeps the framework default render ceiling at 120 Hz
so SurfaceFlinger's touch/content policy can select both physical modes; the
user's peak and minimum settings provide the Standard and Force 120 clamps.

Double-tap-to-wake is bridged from the persistent TB132FU setting to the stock
Goodix `/proc/gesture_control` node by init. Device policy gives that exact proc
node its own SELinux type instead of granting init access to generic proc files.

It must not inherit PHH, TrebleDroid, `gsi_release.mk`, or another device tree.
Do not build or flash a replacement vendor, ODM, super, bootloader, modem,
persist, NVRAM/NVDATA, or calibration image from this tree.
