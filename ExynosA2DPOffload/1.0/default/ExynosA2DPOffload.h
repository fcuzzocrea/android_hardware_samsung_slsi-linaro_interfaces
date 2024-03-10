/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef VENDOR_SAMSUNG_SLSI_HARDWARE_EXYNOSA2DPOFFLOAD_V1_0_EXYNOSA2DPOFFLOAD_H
#define VENDOR_SAMSUNG_SLSI_HARDWARE_EXYNOSA2DPOFFLOAD_V1_0_EXYNOSA2DPOFFLOAD_H

#include <vendor/samsung_slsi/hardware/ExynosA2DPOffload/1.0/IExynosA2DPOffload.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace ExynosA2DPOffload {
namespace V1_0 {
namespace implementation {

using ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffload;
using ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status;
using ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::CodecInfo;
using ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::A2DPData;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

typedef enum {
    WAIT_FOR_NORMAL = 0,
    WAIT_FOR_CODEC,
} wait_type_t;

#define WAIT_SEC_FOR_NORMAL_ACK 6   // 6 seconds
#define WAIT_SEC_FOR_CODEC_ACK  1   // 1 second

#define MAX_SIZE_CODEC_CONFIGURATION    22  // 22 Bytes = SBC Condec Info Size 18Bytes + Type 4Bytes


struct ExynosA2DPOffload : public IExynosA2DPOffload {
    ExynosA2DPOffload();

    // Methods from ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffload follow.
    /* Call by Bluetooth */
    Return<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status> initialize(
        const sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC>& hostIf) override;
    Return<void> a2dp_start_ack(::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status) override;
    Return<void> a2dp_suspend_ack(::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status) override;
    Return<void> a2dp_stop_ack(::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status) override;
    Return<void> a2dp_codec_config_ack(::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status,
        const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::CodecInfo& codecInfo) override;
    Return<void> a2dp_send_datas_to_audio(
        const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::A2DPData& datas) override;
    Return<void> close() override;

    /* Call by Primary AudioHAL */
    Return<int32_t> a2dp_open_stream() override;
    Return<int32_t> a2dp_close_stream() override;
    Return<int32_t> a2dp_start_stream() override;
    Return<int32_t> a2dp_stop_stream() override;
    Return<int32_t> a2dp_suspend_stream() override;
    Return<int32_t> a2dp_get_codec_config(const hidl_memory& codecInfo) override;
    Return<int32_t> a2dp_clear_suspend_flag() override;
    Return<int32_t> a2dp_check_ready() override;

  private:
    // Mutex Lock & Condition
    pthread_mutex_t lock;
    pthread_cond_t  cond;

    // BT A2DP IPC Function Pointers
    sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC> mIPC;

    // Acknowledgement Status
    ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status returnStatus;

    // BT A2DP CODEC Configuration
    uint8_t codecConfig[MAX_SIZE_CODEC_CONFIGURATION];

    // Internal functions
    virtual ~ExynosA2DPOffload();
    virtual int handleAcknowledgement(wait_type_t type);
};

extern "C" IExynosA2DPOffload* HIDL_FETCH_IExynosA2DPOffload(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace ExynosA2DPOffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor

#endif  // VENDOR_SAMSUNG_SLSI_HARDWARE_EXYNOSA2DPOFFLOAD_V1_0_EXYNOSA2DPOFFLOAD_H
