# Lenovo TB132FU Android 17 device product

This is the device-owned native AOSP scaffold for the Wi-Fi Lenovo Tab P11 Pro
(2nd Gen), model TB132FU. Initial builds replace `system`, `system_ext`, and
`product` while retaining the accepted non-GKI boot pipeline and stock
vendor/ODM/firmware/calibration partitions.

The initial product is intentionally plain `aosp_tb132fu`. Pixel-like features
and final branding are gated on a stable hardware-complete AOSP 17 boot.

The device product retains the exact stock Lenovo pen service as a hash-pinned,
locally supplied proprietary prebuilt. It does not inherit the old GSI product
or TrebleDroid to obtain that hardware component.

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
