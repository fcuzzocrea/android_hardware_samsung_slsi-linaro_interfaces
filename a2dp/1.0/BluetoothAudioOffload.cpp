/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "android.hardware.bluetooth.a2dp-exynos"

#include <log/log.h>

#include "BluetoothAudioOffload.h"

#include <vendor/samsung_slsi/hardware/ExynosA2DPOffload/2.0/IExynosA2DPOffload.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace a2dp {
namespace V1_0 {
namespace implementation {

using ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V2_0::IExynosA2DPOffload;

static android::sp<IExynosA2DPOffload> exynos_a2dp = nullptr;

IBluetoothAudioOffload* HIDL_FETCH_IBluetoothAudioOffload(const char* /* name */) {
    return new BluetoothAudioOffload();
}

// Methods from ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.
Return<::android::hardware::bluetooth::a2dp::V1_0::Status> BluetoothAudioOffload::startSession(
    const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf,
    const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration& codecConfig) {

    ::android::hardware::bluetooth::a2dp::V1_0::Status ret = Status::FAILURE;

    ALOGI("%s: enter", __func__);

    /**
     * Initialize the audio platform if codecConfiguration is supported.
     * Save the the IBluetoothAudioHost interface, so that it can be used
     * later to send stream control commands to the HAL client, based on
     * interaction with Audio framework.
     */

     ALOGI("%s: codec info from BT stack: %d, %d, %hhu, %hhu, %d, %hhu, %hhu, %hhu, %hhu", __func__,
            codecConfig.codecType, codecConfig.sampleRate, codecConfig.bitsPerSample, codecConfig.channelMode,
            codecConfig.encodedAudioBitrate, codecConfig.codecSpecific.sbcData.codecParameters,
            codecConfig.codecSpecific.sbcData.minBitpool, codecConfig.codecSpecific.sbcData.maxBitpool,
            codecConfig.codecSpecific.ldacData.bitrateIndex);

    if (exynos_a2dp == nullptr) {
        exynos_a2dp = IExynosA2DPOffload::getService();
        if (exynos_a2dp == nullptr) {
            ALOGE("%s: fail to get Exynos a2dp hidl impl", __func__);
            return Status::FAILURE;
        }
    }

    this->hostIF = hostIf;

    ret = exynos_a2dp->session_start(hostIf, codecConfig);

    ALOGI("%s: return from exynos a2dp hidl is %hhu", __func__, ret);

    return ret;
}

Return<void> BluetoothAudioOffload::streamStarted(
    ::android::hardware::bluetooth::a2dp::V1_0::Status status)
{
    ALOGI("%s: enter with Status: %hhu", __func__, status);

    /**
     * Streaming on control path has started,
     * HAL server should start the streaming on data path.
     */

    if (exynos_a2dp == nullptr) {
        ALOGE("%s: There is no Exynos a2dp hidl impl", __func__);
        return Void();
    }

    exynos_a2dp->a2dp_start_ack(status);

    return Void();
}

Return<void> BluetoothAudioOffload::streamSuspended(
    ::android::hardware::bluetooth::a2dp::V1_0::Status status)
{
    int32_t ret = 0;

    ALOGI("%s: enter with Status: %hhu", __func__, status);

    /**
     * Streaming on control path has suspend,
     * HAL server should suspend the streaming on data path.
     */

    if (exynos_a2dp == nullptr) {
        ALOGE("%s: There is no Exynos a2dp hidl impl", __func__);
        return Void();
    }

    exynos_a2dp->a2dp_suspend_ack(status);

    return Void();
}

Return<void> BluetoothAudioOffload::endSession() {

    ALOGI("%s: enter", __func__);
    /**
     * Cleanup the audio platform as remote A2DP Sink device is no
     * longer active
     */

    this->hostIF = nullptr;
    exynos_a2dp = nullptr;

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace a2dp
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
