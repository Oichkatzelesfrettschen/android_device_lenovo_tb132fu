# Lenovo TB132FU Android 17 device product

This is the device-owned native AOSP scaffold for the Wi-Fi Lenovo Tab P11 Pro
(2nd Gen), model TB132FU. Initial builds replace `system`, `system_ext`, and
`product` while retaining the accepted non-GKI boot pipeline and stock
vendor/ODM/firmware/calibration partitions.

The initial product is intentionally plain `aosp_tb132fu`. Pixel-like features
and final branding are gated on a stable hardware-complete AOSP 17 boot.

It must not inherit PHH, TrebleDroid, `gsi_release.mk`, or another device tree.
Do not build or flash a replacement vendor, ODM, super, bootloader, modem,
persist, NVRAM/NVDATA, or calibration image from this tree.
