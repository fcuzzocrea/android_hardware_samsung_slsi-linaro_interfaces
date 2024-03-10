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


LOCAL_PATH := $(call my-dir)

#
# Service
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ExynosA2DPOffload.cpp \
    service.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libhidlbase \
    libhidlmemory \
    libhidltransport \
    liblog \
    libbinder \
    android.hidl.allocator@1.0 \
    android.hidl.memory@1.0 \
    vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0

LOCAL_CFLAGS := -Wall -Werror

# Can not switch to Android.bp until AUDIOSERVER_MULTILIB
# is deprecated as build config variable are not supported
ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_MULTILIB := 32
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
endif

LOCAL_MODULE := vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0-service
LOCAL_INIT_RC := vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0-service.rc
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)

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
    vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0

LOCAL_CFLAGS := -Wall -Werror

LOCAL_MODULE := vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)