# Copyright (C) 2026
# SPDX-License-Identifier: Apache-2.0

DEVICE_PATH := device/lenovo/tb132fu

# Architecture: MediaTek MT6893 / Kompanio 1300T with a 64-bit system and
# retained 32-bit vendor ABI.
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := generic

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic

# The bootloader, DTBO, vendor, ODM, firmware, and calibration partitions are
# external preserved inputs. Kernel/boot packaging remains in the separately
# audited TB132FU kernel pipeline.
TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true
TARGET_NO_RECOVERY := true

# Lenovo's public firmware packages populate only slot A. Use the classic
# recovery updater and device releasetools so an OTA never targets slot B.
AB_OTA_UPDATER := false
TARGET_RELEASETOOLS_EXTENSIONS := $(DEVICE_PATH)

BOARD_BOOTIMAGE_PARTITION_SIZE := 67108864
BOARD_DTBOIMG_PARTITION_SIZE := 8388608
BOARD_FLASH_BLOCK_SIZE := 4096

# Stock virtual-A/B dynamic-partition geometry. Initial Android 17 builds emit
# device-owned system, system_ext, and product images but never a replacement
# vendor image or a flashable super image.
BOARD_SUPER_PARTITION_SIZE := 12884901888
BOARD_SUPER_PARTITION_GROUPS := tb132fu_dynamic_partitions
BOARD_TB132FU_DYNAMIC_PARTITIONS_SIZE := 12882804736
BOARD_TB132FU_DYNAMIC_PARTITIONS_PARTITION_LIST := \
    product \
    system \
    system_ext

TARGET_COPY_OUT_PRODUCT := product
TARGET_COPY_OUT_SYSTEM_EXT := system_ext

# The vendor partition is retained from Lenovo rather than emitted by this
# build, but its copy-out root must still be declared.  Without this assignment,
# the build defaults vendor content to system/vendor and creates /vendor as a
# symlink, which prevents first-stage init from preserving the real vendor
# mount while switching to the system-as-root image.
TARGET_COPY_OUT_VENDOR := vendor

# Lenovo's fstab mounts metadata during first-stage init.  Declare it so the
# system-as-root image contains /metadata and SwitchRoot can move the live
# metadata mount into the new root.
BOARD_USES_METADATA_PARTITION := true

BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_USERIMAGES_USE_EXT4 := true

# Pin product and system_ext to the exact currently booted logical extents.
# SHA-256 hashtrees otherwise make their dynamically sized images 40 KiB and
# 24 KiB larger, respectively, which would make fastbootd mutate super
# metadata during a routine framework update. The image builders allocate the
# unused capacity inside these fixed ceilings as filesystem free space.
BOARD_PRODUCTIMAGE_PARTITION_SIZE := 3489660928
# Pin system to the allocation proven from the tablet's current super metadata.
# This makes an oversized image fail at build time instead of relying on
# fastbootd to resize a logical partition implicitly.  The temporary static
# apexd probe still leaves roughly 255 MiB free in the fixed-size filesystem.
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1317494784
BOARD_SYSTEM_EXTIMAGE_PARTITION_SIZE := 536870912

# The retained Android 12 vendor supplies its own version-31 CIL policy and HAL
# manifest. Android 17 supplies the versioned platform mapping; no vendor policy
# is compiled or installed from this device tree.
PRODUCT_FULL_TREBLE := true

# Device-owned system_ext policy labels only the Goodix compatibility node
# used by the native DT2W setting. Retained Lenovo vendor policy is untouched.
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += \
    $(DEVICE_PATH)/sepolicy/system_ext/private

# The owner-build wrapper product (device/lenovo/tb132fu-avf) contributes system_ext policy only
# where it is staged. The NPU scheduling HAL policy joins the build only with
# TB132FU_NPU_SERVICE=true, the same switch that adds the HAL module to the product.
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += \
    $(wildcard device/lenovo/tb132fu-avf/sepolicy/system_ext/private)
ifeq ($(TB132FU_NPU_SERVICE),true)
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += \
    $(wildcard device/lenovo/tb132fu-avf/npu/sepolicy/system_ext/private)
endif

# Acknowledge the exact optional HAL instances provided by Lenovo's retained
# Android 12 vendor partition. Platform FCM5 requirements remain enabled and
# are validated separately against the real 4.19 kernel configuration.
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE += \
    $(DEVICE_PATH)/framework_compatibility_matrix.xml

# Use normal AVB footers for the three framework-owned logical partitions.
# Flashing vbmeta/super is intentionally outside the initial bring-up path.
BOARD_AVB_ENABLE := true
BOARD_AVB_VBMETA_SYSTEM := system system_ext product
BOARD_AVB_VBMETA_SYSTEM_KEY_PATH := external/avb/test/data/testkey_rsa2048.pem
BOARD_AVB_VBMETA_SYSTEM_ALGORITHM := SHA256_RSA2048
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX := 0
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX_LOCATION := 1
BOARD_AVB_SYSTEM_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256
BOARD_AVB_SYSTEM_EXT_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256
BOARD_AVB_PRODUCT_ADD_HASHTREE_FOOTER_ARGS += --hash_algorithm sha256

# Stock reference facts used by validation scripts; these do not authorize
# emitting or flashing those partitions.
TB132FU_STOCK_VENDOR_PARTITION_SIZE := 694665216
TB132FU_STOCK_MAIN_GROUP_SIZE := 12882804736
