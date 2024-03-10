/*
 * Copyright (C) 2020 Samsung Electronics
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

#pragma once

#include "OffloadIoctl.h"
#include "OffloadConfig.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

using android::hardware::tetheroffload::control::V1_0::IOffloadControl;
using android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using android::hardware::tetheroffload::control::V1_0::OffloadCallbackEvent;

enum offload_event_num {
	/* OEM defined event */
	INTERNAL_OFFLOAD_STOPPED	= 5000,
};

struct offload_event {
	int32_t event_num;
} __packed;

struct offloadThread {
	bool created;
	pthread_t threadId;
};

class OffloadControl : public TetherOffload, public IOffloadControl {
public:
	OffloadControl(OffloadConfig *config);
	virtual ~OffloadControl();

	/* IOffloadControl */
	Return<void> initOffload(const ::android::sp<ITetheringOffloadCallback>& cb,
		initOffload_cb hidl_cb) override;
	Return<void> stopOffload(stopOffload_cb hidl_cb) override;
	Return<void> setLocalPrefixes(const hidl_vec<hidl_string>& prefixes,
		setLocalPrefixes_cb hidl_cb) override;
	Return<void> getForwardedStats(const hidl_string& upstream,
		getForwardedStats_cb hidl_cb) override;
	Return<void> setDataLimit(const hidl_string& upstream, uint64_t limit,
		setDataLimit_cb hidl_cb) override;
	Return<void> setUpstreamParameters(const hidl_string& iface,
		const hidl_string& v4Addr, const hidl_string& v4Gw,
		const hidl_vec<hidl_string>& v6Gws, setUpstreamParameters_cb hidl_cb) override;
	Return<void> addDownstream(const hidl_string& iface, const hidl_string& prefix,
		addDownstream_cb hidl_cb) override;
	Return<void> removeDownstream(const hidl_string& iface,
		const hidl_string& prefix, removeDownstream_cb hidl_cb) override;

	static void threadResetNoti(enum threadType type);
	virtual int getDownstreamDstRing(uint32_t v4addr);
	virtual bool hwCapaMatched(enum hw_capabilities_mask mask);

protected:
	virtual bool isInitialized();
	virtual int checkInterfaceStat(string name);

private:
	static const int DEV_POLL_TIMEOUT = -1;
	static inline pthread_mutex_t mThreadLock = PTHREAD_MUTEX_INITIALIZER;
	static inline offloadThread mThreads[THREAD_MAX];

	sp<ITetheringOffloadCallback> mCtrlCb;
	upstreamInfo mUpstream;
	map<string, downstreamInfo> mDownstreams;
	OffloadConfig *mConfig;
	struct hw_info mHwInfo;

	void resetValues() override;

	int parseAddress(const string &in, string &out, string &netmask);

	void startThread(enum threadType type);
	static void *eventThread(void *arg);
	static void *conntrackThread(void *arg);
	int callbackEvent(int32_t event);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
