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

#ifndef VENDOR_SAMSUNG_SLSI_HARDWARE_EXYNOSA2DPOFFLOAD_V1_0_EXYNOSA2DPOFFLOAD_H
#define VENDOR_SAMSUNG_SLSI_HARDWARE_EXYNOSA2DPOFFLOAD_V1_0_EXYNOSA2DPOFFLOAD_H

#include <vendor/samsung_slsi/hardware/ExynosA2DPOffload/2.0/IExynosA2DPOffload.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioHost.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioOffload.h>
#include <android/hardware/bluetooth/a2dp/1.0/types.h>

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#define WAIT_SEC_FOR_NORMAL_ACK 6   // 6 seconds
#define WAIT_SEC_FOR_CODEC_ACK  1   // 1 second

#define MAX_SIZE_CODEC_CONFIGURATION 22  // 22 Bytes = SBC Condec Info Size 18Bytes + Type 4Bytes


namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace ExynosA2DPOffload {
namespace V2_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost;

/** These Configurations from A2DP IPC Library */
typedef struct {
    uint32_t codec_type;
    uint32_t subband;       /* 4, 8 */
    uint32_t blk_len;       /* 4, 8, 12, 16 */
    uint32_t bitrate;       /* 320kbps to 512kbps */
    uint16_t sampling_rate; /*44.1khz,48khz*/
    uint8_t  channels;      /*0(Mono),1(Dual_mono),2(Stereo),3(JS)*/
    uint8_t  alloc;         /*0(Loudness),1(SNR)*/
    uint8_t  min_bitpool;   /* 2 */
    uint8_t  max_bitpool;   /*53(44.1khz),51 (48khz) */
} audio_sbc_encoder_config;

typedef struct {
    uint32_t codec_type;
    uint32_t bitrate;
    uint16_t sampling_rate;
    uint8_t  channels;
} audio_aptx_encoder_config;

typedef enum {
    WAIT_FOR_NORMAL = 0,
    WAIT_FOR_CODEC,
} wait_type_t;


struct ExynosA2DPOffload : public IExynosA2DPOffload {
    ExynosA2DPOffload();

    // Methods from ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffload follow.
    /* Call by Bluetooth */
    Return<::android::hardware::bluetooth::a2dp::V1_0::Status> session_start(
        const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf,
        const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration& codecConfig);
    Return<void> a2dp_start_ack(::android::hardware::bluetooth::a2dp::V1_0::Status status) override;
    Return<void> a2dp_suspend_ack(::android::hardware::bluetooth::a2dp::V1_0::Status status) override;
    Return<void> a2dp_stop_ack(int32_t status) override;
    Return<void> a2dp_codec_config_ack(
        ::android::hardware::bluetooth::a2dp::V1_0::Status status,
        const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V2_0::CodecInfo& codec_info) override;
    Return<void> a2dp_send_datas_to_audio(
        const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V2_0::DYN_Param& data) override;
    Return<void> close() override;

    /* Call by Primary AudioHAL */
    Return<int32_t> a2dp_open_stream() override;
    Return<int32_t> a2dp_close_stream() override;
    Return<int32_t> a2dp_start_stream() override;
    Return<int32_t> a2dp_stop_stream() override;
    Return<int32_t> a2dp_suspend_stream() override;
    Return<int32_t> a2dp_get_codec_config(const hidl_memory& codec_info) override;
    Return<int32_t> a2dp_clear_suspend_flag() override;
    Return<int32_t> a2dp_check_ready() override;

    private:
        // Mutex Lock & Condition
        pthread_mutex_t lock;
        pthread_cond_t  cond;

        // Acknowledgement Status
        ::android::hardware::bluetooth::a2dp::V1_0::Status returnStatus;

        // Internal functions
        virtual ~ExynosA2DPOffload() override;
        virtual int handleAcknowledgement(wait_type_t type);

        uint8_t codecConf[MAX_SIZE_CODEC_CONFIGURATION];
        ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration codecInfo;

        sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost> host_if;
};

extern "C" IExynosA2DPOffload* HIDL_FETCH_IExynosA2DPOffload(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace ExynosA2DPOffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor

#endif  // VENDOR_SAMSUNG_SLSI_HARDWARE_EXYNOSA2DPOFFLOAD_V1_0_EXYNOSA2DPOFFLOAD_H
