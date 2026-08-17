# Copyright (C) 2026
# SPDX-License-Identifier: Apache-2.0

LOCAL_PATH := device/lenovo/tb132fu

# Compose a normal multi-partition AOSP tablet product. This deliberately does
# not inherit gsi_release.mk, PHH/TrebleDroid, a universal overlay catalog, or a
# foreign-device product. Architecture definitions from core_64_bit are shared
# platform build rules, not GSI device behavior.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/handheld_system.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/handheld_system_ext.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/handheld_product.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/updatable_apex.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/large_screen_common.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AllAudio.mk)

PRODUCT_USE_DYNAMIC_PARTITIONS := true
PRODUCT_USE_DYNAMIC_PARTITION_SIZE := true

# Keep the stock device's 4 KiB ABI. The frozen vendor/ODM stack is not 16 KiB
# compatible.
PRODUCT_MAX_PAGE_SIZE_SUPPORTED := 4096
PRODUCT_NO_BIONIC_PAGE_SIZE_MACRO := true

# The accepted v85/v86 4.19 configs have CONFIG_USERFAULTFD disabled and do
# not provide the MREMAP_DONTUNMAP contract required by ART's UFFD collector.
# Keep the normal concurrent-moving collector instead of relying on the build
# system's unknown-kernel default.
PRODUCT_ENABLE_UFFD_GC := false

# The tablet launched with product API 31 on a board-first API 30 vendor. Keep
# the framework compatibility payloads for that frozen vendor ABI.
PRODUCT_SHIPPING_API_LEVEL := 31
PRODUCT_EXTRA_VNDK_VERSIONS := 31

# Emit only framework-owned logical partitions during bring-up. Boot is built
# by the kernel pipeline; vendor/ODM and all firmware remain byte-preserved.
PRODUCT_BUILD_BOOT_IMAGE := false
PRODUCT_BUILD_CACHE_IMAGE := false
PRODUCT_BUILD_DEBUG_BOOT_IMAGE := false
PRODUCT_BUILD_DEBUG_VENDOR_BOOT_IMAGE := false
PRODUCT_BUILD_ODM_IMAGE := false
PRODUCT_BUILD_ODM_DLKM_IMAGE := false
PRODUCT_BUILD_PRODUCT_IMAGE := true
PRODUCT_BUILD_RAMDISK_IMAGE := false
PRODUCT_BUILD_SUPER_EMPTY_IMAGE := false
PRODUCT_BUILD_SUPER_PARTITION := false
PRODUCT_BUILD_SYSTEM_DLKM_IMAGE := false
PRODUCT_BUILD_SYSTEM_EXT_IMAGE := true
PRODUCT_BUILD_SYSTEM_IMAGE := true
PRODUCT_BUILD_USERDATA_IMAGE := false
PRODUCT_BUILD_VENDOR_BOOT_IMAGE := false
PRODUCT_BUILD_VENDOR_DLKM_IMAGE := false
PRODUCT_BUILD_VENDOR_IMAGE := false

PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := relaxed
PRODUCT_RESTRICT_VENDOR_FILES := all

# Preserve CPU time-in-state accounting on the pre-BTF 4.19 kernel. The
# loader selects this object only on kernels older than 5.10.
PRODUCT_PACKAGES += \
    LenovoPenService \
    timeInStateLegacy.bpf \
    TB132FUFrameworkDisplayPowerOverlay \
    TB132FUSettingsProviderDefaultsOverlay \
    TB132FUSystemUIAodOverlay

PRODUCT_SYSTEM_PROPERTIES += \
    persist.sys.tb132fu.dt2w=1 \
    ro.bpf.platform_kver_override=5.10.0 \
    ro.custom.security_patch=$(PLATFORM_SECURITY_PATCH) \
    ro.surface_flinger.set_touch_timer_ms=1000 \
    ro.vendor.mtk_aod_support=1

# The unlocked boot state otherwise makes adbd treat an absent property as
# authentication disabled. Keep userdebug convenient for controlled bring-up,
# but require host-key authentication whenever a hardened user build enables
# USB debugging explicitly.
ifeq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_SYSTEM_PROPERTIES += ro.adb.secure=1
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/displayconfig/default.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/displayconfig/default.xml \
    $(LOCAL_PATH)/init.tb132fu.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/init.tb132fu.rc \
    $(LOCAL_PATH)/permissions/privapp-permissions-lenovo-penservice.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/privapp-permissions-lenovo-penservice.xml \
    $(LOCAL_PATH)/etc/appcompat/compat_framework_overrides.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/appcompat/compat_framework_overrides.xml \
    $(LOCAL_PATH)/etc/pen/pen_config.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/pen/pen_config.xml \
    $(LOCAL_PATH)/media/audio/ui/pen_mono.wav:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/pen_mono.wav \
    $(LOCAL_PATH)/media/audio/ui/pen_sound.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/pen_sound.ogg \
    $(LOCAL_PATH)/tb132fu-no-telephony.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/tb132fu-no-telephony.xml \
    frameworks/native/data/etc/android.software.freeform_window_management.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/android.software.freeform_window_management.xml

TARGET_SYSTEM_PROP := $(LOCAL_PATH)/system.prop
