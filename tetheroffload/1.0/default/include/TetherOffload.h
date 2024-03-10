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

#define LOG_TAG "tetheroffload-service"

#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.0/IOffloadControl.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>
#include <hidl/Status.h>
#include <utils/Log.h>
#include <thread>
#include <string>
#include <regex>
#include <vector>
#include <map>

using android::hardware::hidl_array;
using android::hardware::hidl_memory;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::hidl_handle;
using android::hardware::Return;
using android::hardware::Void;
using android::sp;
using android::status_t;

using std::string;
using std::vector;
using std::map;
using std::regex;
using std::stringstream;
using std::thread;

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

#ifndef TETHER_DEBUG
/* #define TETHER_DEBUG */
#endif

#define HALLOGI(s, args...) \
	ALOGI("tether_offload: %s: " s, __func__, ##args)
#define HALLOGE(s, args...) \
	ALOGE("tether_offload: %s: " s " (%s)", __func__, ##args, strerror(errno))
#ifdef TETHER_DEBUG
#define HALLOGD(s, args...) \
	ALOGI("tether_offload: dbg: %s: " s, __func__, ##args)
#else
#define HALLOGD(s, args...)
#endif

#define ARRAY_LENGTH(arr)	(sizeof(arr)/sizeof(arr[0]))

#define HW_REG_NAT_LOCAL_ADDR_MAX	(16)

/* upstream has been restricted */
#define UPSTREAM_IFACE_VTS_PATTERN	"(rmnet_data\\d)|(dummy\\d)"
#define UPSTREAM_IFACE_PATTERN		"(rmnet[0-7])|" UPSTREAM_IFACE_VTS_PATTERN

/* lower index has higher priority */
static const char *DEVICE_OFFLOAD[] = {
	"/dev/dit2",
};

struct upstreamInfo {
	string iface;
	string v4Addr;
	string v4Gw;
	vector<string> v6Gws;
};

struct downstreamInfo {
	string iface;
	__le32 v4Addr;
	uint8_t v4Mask;
	uint16_t dstRing;
};

struct ConnTrackContext {
	int fd;
	unsigned int groups;
	pthread_t tid;
};

enum threadType {
	THREAD_EVENT,
	THREAD_CONNTRACK_UDP,
	THREAD_CONNTRACK_TCP,
	THREAD_NETLINK,
	THREAD_MAX,
};

enum ctFamily {
	CT_UDP,
	CT_TCP,
	CT_MAX,
};

class TetherOffload {
public:
	TetherOffload();
	virtual ~TetherOffload();

protected:
	static int openDevice();
	static int ioctlOffload(int cmd, void *arg);
	virtual void resetValues() = 0;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
