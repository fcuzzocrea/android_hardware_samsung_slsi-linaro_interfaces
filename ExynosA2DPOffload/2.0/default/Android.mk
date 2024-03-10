#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(BOARD_USE_BTA2DP_OFFLOAD),true)
LOCAL_PATH := $(call my-dir)

#
# Implementation
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ExynosA2DPOffload.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libhidlbase \
    libhidlmemory \
    libhidltransport \
    liblog \
    android.hidl.allocator@1.0 \
    android.hidl.memory@1.0 \
    android.hardware.bluetooth.a2dp@1.0 \
    vendor.samsung_slsi.hardware.ExynosA2DPOffload@2.0

ifeq ($(BOARD_USE_AUDIOHAL_UNIFIED), true)
LOCAL_SHARED_LIBRARIES += libaudioproxy2
LOCAL_C_INCLUDES := $(TOP)/vendor/samsung_slsi/exynos/audio/hal/sec/impl/proxy
else ifeq ($(BOARD_USE_AUDIOHAL_COMV2), true)
LOCAL_SHARED_LIBRARIES += libaudioproxy2
LOCAL_C_INCLUDES := $(TOP)/hardware/samsung_slsi/exynos/libaudio/audiohal_comv2/proxy
else
LOCAL_SHARED_LIBRARIES += libaudioproxy
ifneq ($(filter exynos9825 ,$(TARGET_SOC)),)
LOCAL_C_INCLUDES := $(TOP)/device/samsung/universal9820/audio/proxy
else
LOCAL_C_INCLUDES := $(TOP)/device/samsung/$(TARGET_DEVICE_NAME)/audio/proxy
endif
endif

LOCAL_HEADER_LIBRARIES := libhardware_headers

LOCAL_CFLAGS := -Wall -Werror

LOCAL_MODULE := vendor.samsung_slsi.hardware.ExynosA2DPOffload@2.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
endif
