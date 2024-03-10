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

#define LOG_TAG "vendor.samsung_slsi.hardware.ExynosA2DPOffload@2.0-impl"
#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <log/log.h>
#include <system/audio.h>

#include "ExynosA2DPOffload.h"
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioHost.h>
#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioOffload.h>

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include "audio_a2dp_proxy.h"


using android::hidl::allocator::V1_0::IAllocator;
using android::hidl::memory::V1_0::IMemory;
using android::hardware::hidl_memory;

using android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost;
using android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload;
using android::hardware::bluetooth::a2dp::V1_0::Status;
using android::hardware::bluetooth::a2dp::V1_0::CodecType;
using android::hardware::bluetooth::a2dp::V1_0::SampleRate;
using android::hardware::bluetooth::a2dp::V1_0::ChannelMode;


namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace ExynosA2DPOffload {
namespace V2_0 {
namespace implementation {

// Entry Point
IExynosA2DPOffload* HIDL_FETCH_IExynosA2DPOffload(const char* /* name */) {
    return new ExynosA2DPOffload();
}

// Methods from ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.

ExynosA2DPOffload::ExynosA2DPOffload()
{
    ALOGI("%s: enter", __func__);

    pthread_mutex_init(&this->lock, (const pthread_mutexattr_t *) NULL);
    pthread_cond_init (&this->cond, (const pthread_condattr_t *) NULL);
}

ExynosA2DPOffload::~ExynosA2DPOffload()
{
    ALOGI("%s: enter", __func__);

    pthread_cond_destroy (&this->cond);
    pthread_mutex_destroy(&this->lock);
}

/* Call by Bluetooth a2dp hidl */
Return<::android::hardware::bluetooth::a2dp::V1_0::Status> ExynosA2DPOffload::session_start(
    const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf,
    const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration& codecConfig)
{
    ALOGI("%s: enter", __func__);

    switch (codecConfig.codecType) {
        case CodecType::SBC:
        case CodecType::APTX:
            ALOGI("%s: Codec type[%d] is supported one", __func__, codecConfig.codecType);
            break;
        default:
            ALOGI("%s: Codec type[%d] is unsupported one", __func__, codecConfig.codecType);
            return Status::UNSUPPORTED_CODEC_CONFIGURATION;
    }

    this->host_if = hostIf;
    this->codecInfo = codecConfig;

    // Clear A2DP Codec Configuration
    memset((void *)&this->codecConf, 0, MAX_SIZE_CODEC_CONFIGURATION);

    return Status::SUCCESS;
}

int ExynosA2DPOffload::handleAcknowledgement(wait_type_t type)
{
    struct timespec ts;
    int ret_val = 0;

    // Wait for Acknowledgement with Timeout
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        ALOGE("a2dp_offload-%s: clock_gettime failed", __func__);
        return -1;
    }

    if (type == WAIT_FOR_NORMAL)
        ts.tv_sec += WAIT_SEC_FOR_NORMAL_ACK;
    else if (type == WAIT_FOR_CODEC)
        ts.tv_sec += WAIT_SEC_FOR_CODEC_ACK;

    pthread_mutex_lock(&this->lock);
    ret_val = pthread_cond_timedwait(&this->cond, &this->lock, &ts);
    pthread_mutex_unlock(&this->lock);
    if (ret_val == 0) {
        // Something is signalled, check Returned Status
        if (this->returnStatus == Status::SUCCESS) {
            ALOGD("a2dp_offload-%s: Success Acknowledgement, return success", __func__);
            return 0;
        } else if (this->returnStatus == Status::FAILURE) {
            ALOGD("a2dp_offload-%s: Failure Acknowledgement, return failure", __func__);
            return -1;
        } else if (this->returnStatus == Status::PENDING) {
            ALOGD("a2dp_offload-%s: Pending Acknowledgement, wait more...", __func__);

            if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
                ALOGE("a2dp_offload-%s: clock_gettime failed", __func__);
                return -1;
            }

            if (type == WAIT_FOR_NORMAL)
                ts.tv_sec += WAIT_SEC_FOR_NORMAL_ACK;
            else if (type == WAIT_FOR_CODEC)
                ts.tv_sec += WAIT_SEC_FOR_CODEC_ACK;

            pthread_mutex_lock(&this->lock);
            ret_val = pthread_cond_timedwait(&this->cond, &this->lock, &ts);
            pthread_mutex_unlock(&this->lock);
            if (ret_val == 0) {
                // Something is signalled, check Returned Status again
                if (this->returnStatus == Status::SUCCESS) {
                    ALOGD("a2dp_offload-%s: Success Acknowledgement, return success", __func__);
                    return 0;
                } else {
                    ALOGD("a2dp_offload-%s: Failure or Pending Acknowledgement, return failure", __func__);
                    return -1;
                }
            } else if (ret_val == ETIMEDOUT) {
                // Timeout, not defined to act what
                ALOGI("a2dp_offload-%s: timed out to wait", __func__);
                return -1;
            } else {
                ALOGE("a2dp_offload-%s: failed to wait", __func__);
                return -1;
            }
        }
    } else if (ret_val == ETIMEDOUT) {
        // Timeout, not defined to act what
        ALOGI("a2dp_offload-%s: timed out to wait", __func__);
        return -1;
    } else {
        ALOGE("a2dp_offload-%s: failed to wait", __func__);
        return -1;
    }

    return 0;
}

Return<void> ExynosA2DPOffload::a2dp_start_ack(::android::hardware::bluetooth::a2dp::V1_0::Status status)
{
    ALOGI("%s: enter with Status(%hhu)", __func__, status);

    pthread_mutex_lock(&this->lock);

    this->returnStatus = status;

    pthread_mutex_unlock(&this->lock);

    if (status == Status::FAILURE) {
        ALOGI("%s: failure, call BT interface again...", __func__);
        usleep(10000);
        this->host_if->startStream();
    } else {
        pthread_cond_signal(&this->cond);
    }

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_suspend_ack(::android::hardware::bluetooth::a2dp::V1_0::Status status)
{
    ALOGI("%s: enter with Status(%hhu)", __func__, status);

    pthread_mutex_lock(&this->lock);

    this->returnStatus = status;

    pthread_mutex_unlock(&this->lock);

    if (status == Status::FAILURE) {
        ALOGI("%s: failure, call BT interface again...", __func__);
        usleep(10000);
        this->host_if->suspendStream();
    } else {
        pthread_cond_signal(&this->cond);
    }

    pthread_cond_signal(&this->cond);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_stop_ack(int32_t status)
{
    ALOGI("%s: enter with Status(%d)", __func__, status);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_codec_config_ack(
    ::android::hardware::bluetooth::a2dp::V1_0::Status status,
    const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V2_0::CodecInfo& codec_info __unused)
{
    ALOGI("a2dp_offload-%s: called with Status(%hhu)", __func__, status);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_send_datas_to_audio(
    const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V2_0::DYN_Param& data)
{
    ALOGI("%s: enter with bit_rate[%d], peerMtu[%d]", __func__, data.bit_rate, data.peer_mtu);

    proxy_a2dp_set_mixer(data.bit_rate, data.peer_mtu);

    return Void();
}

Return<void> ExynosA2DPOffload::close()
{
    ALOGI("a2dp_offload-%s: called", __func__);

    return Void();
}


/* Call by Primary AudioHAL */
Return<int32_t> ExynosA2DPOffload::a2dp_open_stream()
{
    ALOGI("%s: enter", __func__);
    return 0;
}

Return<int32_t> ExynosA2DPOffload::a2dp_close_stream()
{
    ALOGI("%s: enter", __func__);
    return 0;
}

Return<int32_t> ExynosA2DPOffload::a2dp_start_stream()
{
    ALOGI("%s: enter", __func__);

    this->host_if->startStream();

    // Handle the Acknowledgement
    return this->handleAcknowledgement(WAIT_FOR_NORMAL);;
}

Return<int32_t> ExynosA2DPOffload::a2dp_stop_stream()
{
    ALOGI("%s: enter", __func__);

    this->host_if->stopStream();

    return 0;
}

Return<int32_t> ExynosA2DPOffload::a2dp_suspend_stream()
{
    ALOGI("%s: enter", __func__);

    this->host_if->suspendStream();

    // Handle the Acknowledgement
    return this->handleAcknowledgement(WAIT_FOR_NORMAL);
}

Return<int32_t> ExynosA2DPOffload::a2dp_get_codec_config(const hidl_memory& codec_info)
{
    audio_sbc_encoder_config codec_sbc;
    audio_aptx_encoder_config codec_aptx;
    int32_t ret = 0;

    ALOGI("%s: enter", __func__);

    if (this->codecInfo.codecType == CodecType::SBC) {
        codec_sbc.codec_type = AUDIO_FORMAT_SBC;
        codec_sbc.subband = (this->codecInfo.codecSpecific.sbcData.codecParameters & 0xC); // A2DP_SBC_IE_SUBBAND_MSK 0x0C, b3-b2
        codec_sbc.blk_len = (this->codecInfo.codecSpecific.sbcData.codecParameters & 0xF0); // A2DP_SBC_IE_BLOCKS_MSK 0xF0, b7-b4
        codec_sbc.alloc = (this->codecInfo.codecSpecific.sbcData.codecParameters & 0x3); // A2DP_SBC_IE_ALLOC_MD_MSK 0x03, b1-b0
        codec_sbc.min_bitpool = this->codecInfo.codecSpecific.sbcData.minBitpool;
        codec_sbc.max_bitpool = this->codecInfo.codecSpecific.sbcData.maxBitpool;
        codec_sbc.bitrate = this->codecInfo.encodedAudioBitrate;

        /** Map to hidl type to codec parsing type */
        switch (this->codecInfo.sampleRate) {
            case SampleRate::RATE_48000:
                codec_sbc.sampling_rate = (uint16_t)48000;
                break;
            case SampleRate::RATE_44100:
                codec_sbc.sampling_rate = (uint16_t)44100;
                break;
            default:
                break;
        }

        switch (this->codecInfo.channelMode) {
            case ChannelMode::MONO:
                codec_sbc.channels = 0x1;
                break;
            case ChannelMode::STEREO:
                codec_sbc.channels = 0x2;
                break;
            default:
                break;
        }

        memcpy((void *)this->codecConf, (const void *)&codec_sbc, MAX_SIZE_CODEC_CONFIGURATION);
    } else if (this->codecInfo.codecType == CodecType::APTX) {
        codec_aptx.codec_type = AUDIO_FORMAT_APTX;
        codec_aptx.bitrate = this->codecInfo.encodedAudioBitrate;

        /** Map to hidl type to codec parsing type */
        switch (this->codecInfo.sampleRate) {
            case SampleRate::RATE_48000:
                codec_aptx.sampling_rate = (uint16_t)48000;
                break;
            case SampleRate::RATE_44100:
                codec_aptx.sampling_rate = (uint16_t)44100;
                break;
            default:
                break;
        }

        switch (this->codecInfo.channelMode) {
            case ChannelMode::MONO:
                codec_aptx.channels = 0x1;
                break;
            case ChannelMode::STEREO:
                codec_aptx.channels = 0x2;
                break;
            default:
                break;
        }

        memcpy((void *)this->codecConf, (const void *)&codec_aptx, (MAX_SIZE_CODEC_CONFIGURATION/2));
    } else {
        ALOGE("%s: Codec type[%d] is unsupported one", __func__, this->codecInfo.codecType);
        return -1;
    }

    // Set the memory to return A2DP Codec Configuration
    sp<IMemory> memory = mapMemory(codec_info);
    if (memory != nullptr) {
        memory->update();
        memcpy(memory->getPointer(), (const void *)&this->codecConf, MAX_SIZE_CODEC_CONFIGURATION);
        memory->commit();

        // Clear A2DP Codec Configuration
        memset((void *)&this->codecConf, 0, MAX_SIZE_CODEC_CONFIGURATION);
    } else {
        ALOGI("a2dp_offload-%s: failed to map allocated ashmem", __func__);
        ret = -1;
    }

    return ret;
}

Return<int32_t> ExynosA2DPOffload::a2dp_clear_suspend_flag()
{
    ALOGI("a2dp_offload-%s: called", __func__);
    return 0;
}

Return<int32_t> ExynosA2DPOffload::a2dp_check_ready()
{
    ALOGI("a2dp_offload-%s: called", __func__);
    return 0;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace ExynosA2DPOffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
