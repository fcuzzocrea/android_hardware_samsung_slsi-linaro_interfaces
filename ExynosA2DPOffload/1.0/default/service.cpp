/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0-service"

#include <android/log.h>
#include <hidl/HidlTransportSupport.h>
#include <binder/ProcessState.h>

#include "ExynosA2DPOffload.h"

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

// Generated HIDL files
using vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffload;
using vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::implementation::ExynosA2DPOffload;

using android::status_t;
using android::OK;

int main() {
    ALOGD("ExynosA2DPOffload HIDL start");
    android::ProcessState::initWithDriver("/dev/vndbinder");

    android::sp<IExynosA2DPOffload> service = new ExynosA2DPOffload();

    configureRpcThreadpool(1, true /*callerWillJoin*/);
    android::status_t status = service->registerAsService();
    if (status == OK) {
        ALOGI("ExynosA2DPOffload HAL service Ready!!!");
        joinRpcThreadpool();
    } else
        ALOGE("Cannot register ExynosA2DPOffload HAL service");

    return 1;
}
