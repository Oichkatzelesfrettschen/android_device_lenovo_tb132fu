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

BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_USERIMAGES_USE_EXT4 := true

# Leave room for filesystem metadata and incremental source growth while the
# images are dynamically sized inside the stock main_a group.
BOARD_PRODUCTIMAGE_PARTITION_RESERVED_SIZE := 268435456
BOARD_SYSTEMIMAGE_PARTITION_RESERVED_SIZE := 268435456
BOARD_SYSTEM_EXTIMAGE_PARTITION_RESERVED_SIZE := 134217728

# The retained Android 12 vendor supplies its own policy and HAL manifest.
PRODUCT_FULL_TREBLE := true
TARGET_USES_PREBUILT_VENDOR_SEPOLICY := true

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

# Stock reference facts used by validation scripts; these do not authorize
# emitting or flashing those partitions.
TB132FU_STOCK_VENDOR_PARTITION_SIZE := 694665216
TB132FU_STOCK_MAIN_GROUP_SIZE := 12882804736
