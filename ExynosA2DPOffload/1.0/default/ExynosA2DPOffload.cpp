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

#define LOG_TAG "vendor.samsung_slsi.hardware.ExynosA2DPOffload@1.0-impl"
#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <log/log.h>

#include "ExynosA2DPOffload.h"

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>


using android::hidl::allocator::V1_0::IAllocator;
using android::hidl::memory::V1_0::IMemory;
using android::hardware::hidl_memory;

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace ExynosA2DPOffload {
namespace V1_0 {
namespace implementation {


ExynosA2DPOffload::ExynosA2DPOffload()
{
    ALOGI("a2dp_offload-%s: ExynosA2DPOffload is created", __func__);

    pthread_mutex_init(&this->lock, (const pthread_mutexattr_t *) NULL);
    pthread_cond_init (&this->cond, (const pthread_condattr_t *) NULL);

    this->returnStatus = Status::SUCCESS;
    memset((void *)&this->codecConfig[0], 0, MAX_SIZE_CODEC_CONFIGURATION);
}

ExynosA2DPOffload::~ExynosA2DPOffload()
{
    if (this->mIPC != nullptr)
        this->mIPC.clear();

    pthread_cond_destroy (&this->cond);
    pthread_mutex_destroy(&this->lock);

    ALOGI("a2dp_offload-%s: ExynosA2DPOffload is destroyed", __func__);
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


// Methods from ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.

/* Call by Bluetooth */
Return<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status> ExynosA2DPOffload::initialize(
    const sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC>& hostIf)
{
    ALOGI("a2dp_offload-%s: called", __func__);

    // Check Host IPC Interfaces
    if (hostIf == nullptr) {
        ALOGE("a2dp_offload-%s: ExynosA2DPOffloadIPC Interfaces are not specified", __func__);
        return ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status::FAILURE;
    }

    // Set Host IPC Interfaces
    this->mIPC = hostIf;

    return ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status::SUCCESS;
}

Return<void> ExynosA2DPOffload::a2dp_start_ack(
    ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status)
{
    ALOGI("a2dp_offload-%s: called with Status(%hhu)", __func__, status);

    pthread_mutex_lock(&this->lock);
    this->returnStatus = status;
    pthread_mutex_unlock(&this->lock);
    pthread_cond_signal(&this->cond);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_suspend_ack(
    ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status)
{
    ALOGI("a2dp_offload-%s: called with Status(%hhu)", __func__, status);

    pthread_mutex_lock(&this->lock);
    this->returnStatus = status;
    pthread_mutex_unlock(&this->lock);
    pthread_cond_signal(&this->cond);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_stop_ack(
    ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status)
{
    ALOGI("a2dp_offload-%s: called with Status(%hhu)", __func__, status);

    pthread_mutex_lock(&this->lock);
    this->returnStatus = status;
    pthread_mutex_unlock(&this->lock);
    pthread_cond_signal(&this->cond);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_codec_config_ack(
    ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::Status status,
    const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::CodecInfo& codecInfo)
{
    ALOGI("a2dp_offload-%s: called with Status(%hhu)", __func__, status);

    pthread_mutex_lock(&this->lock);
    this->returnStatus = status;

    /*
     * Save A2DP CODEC Configurations
     *
     * CodecInfo type is vector of uint8_t.
     * codecInfo.data() returns const uint8_t* and codecInfo.size() returns size_t
     */
    memcpy((void *)this->codecConfig, (void *)codecInfo.data(), codecInfo.size());

    pthread_mutex_unlock(&this->lock);
    pthread_cond_signal(&this->cond);

    return Void();
}

Return<void> ExynosA2DPOffload::a2dp_send_datas_to_audio(
    const ::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::A2DPData& datas __unused)
{
    ALOGI("a2dp_offload-%s: called", __func__);

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
    ALOGI("a2dp_offload-%s: called", __func__);
    return 0;
}

Return<int32_t> ExynosA2DPOffload::a2dp_close_stream()
{
    ALOGI("a2dp_offload-%s: called", __func__);
    return 0;
}

Return<int32_t> ExynosA2DPOffload::a2dp_start_stream()
{
    ALOGI("a2dp_offload-%s: called", __func__);

    // Get Host IPC Interface
    ExynosA2DPOffload* self = reinterpret_cast<ExynosA2DPOffload*>(this);
    sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC> hostIPC = self->mIPC;
    if (hostIPC.get() == nullptr) return -1;

    // Call IPC to start BT A2DP Stream
    hostIPC->a2dp_start();

    // Handle the Acknowledgement
    return this->handleAcknowledgement(WAIT_FOR_NORMAL);
}

Return<int32_t> ExynosA2DPOffload::a2dp_stop_stream()
{
    ALOGI("a2dp_offload-%s: called", __func__);

    // Get Host IPC Interface
    ExynosA2DPOffload* self = reinterpret_cast<ExynosA2DPOffload*>(this);
    sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC> hostIPC = self->mIPC;
    if (hostIPC.get() == nullptr) return -1;

    // Call IPC to start BT A2DP Stream
    hostIPC->a2dp_stop();

    // Handle the Acknowledgement
    return this->handleAcknowledgement(WAIT_FOR_NORMAL);
}

Return<int32_t> ExynosA2DPOffload::a2dp_suspend_stream()
{
    ALOGI("a2dp_offload-%s: called", __func__);

    // Get Host IPC Interface
    ExynosA2DPOffload* self = reinterpret_cast<ExynosA2DPOffload*>(this);
    sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC> hostIPC = self->mIPC;
    if (hostIPC.get() == nullptr) return -1;

    // Call IPC to start BT A2DP Stream
    hostIPC->a2dp_suspend();

    // Handle the Acknowledgement
    return this->handleAcknowledgement(WAIT_FOR_NORMAL);
}

Return<int32_t> ExynosA2DPOffload::a2dp_get_codec_config(const hidl_memory& codecInfo)
{
    ALOGI("a2dp_offload-%s: called", __func__);

    // Get Host IPC Interface
    ExynosA2DPOffload* self = reinterpret_cast<ExynosA2DPOffload*>(this);
    sp<::vendor::samsung_slsi::hardware::ExynosA2DPOffload::V1_0::IExynosA2DPOffloadIPC> hostIPC = self->mIPC;
    if (hostIPC.get() == nullptr) return -1;

    // Call IPC to get A2DP Codec Configuration
    hostIPC->a2dp_read_output_audio_config();

    // Handle the Acknowledgement
    int32_t ret = this->handleAcknowledgement(WAIT_FOR_CODEC);
    if (ret == 0) {
        // Set the memory to return A2DP Codec Configuration
        sp<IMemory> memory = mapMemory(codecInfo);
        if (memory != nullptr) {
            memory->update();
            memcpy(memory->getPointer(), (const void *)&this->codecConfig[0], MAX_SIZE_CODEC_CONFIGURATION);
            memory->commit();

            // Clear A2DP Codec Configuration
            memset((void *)&this->codecConfig[0], 0, MAX_SIZE_CODEC_CONFIGURATION);
        } else {
            ALOGI("a2dp_offload-%s: failed to map allocated ashmem", __func__);
            ret = -1;
        }
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


// Entry Point
IExynosA2DPOffload* HIDL_FETCH_IExynosA2DPOffload(const char* /* name */) {
    return new ExynosA2DPOffload();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace ExynosA2DPOffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
