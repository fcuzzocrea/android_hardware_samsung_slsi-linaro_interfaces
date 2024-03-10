/*
 * Copyright (C) 2021 Samsung Electronics
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

#include <arpa/inet.h>
#include <inttypes.h>
#include <pthread.h>
#include <poll.h>
#include <net/if.h>
#include "OffloadIoctlV1_1.h"
#include "OffloadControlV1_1.h"
#include "ConntrackMgr.h"
#include "NetlinkMgr.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_1 {
namespace implementation {

Return<void> OffloadControl::setDataWarningAndLimit(const hidl_string& upstream,
	uint64_t warningBytes, uint64_t limitBytes, setDataWarningAndLimit_cb hidl_cb)
{
	struct forward_limit limit;

	if (!isInitialized()) {
		hidl_cb(false, "not initialized");
		return Void();
	}

	if (upstream.empty()) {
		hidl_cb(false, "upstream is not set");
		return Void();
	}

	if (checkInterfaceStat(upstream) < 0) {
		HALLOGI("failed to get upstream stat %s", upstream.c_str());
		hidl_cb(false, "upstream stat failed");
		return Void();
	}

	strlcpy(limit.iface, upstream.c_str(), IFNAMSIZ);
	limit.data_warning = warningBytes;
	limit.data_limit = limitBytes;

	if (ioctlOffload(OFFLOAD_IOCTL_SET_DATA_WARNING_LIMIT, &limit) < 0) {
		hidl_cb(false, "can't set data warning/limit");
		return Void();
	}

	HALLOGI("%" PRIu64 "/%" PRIu64 " bytes warning/limit applied to %s", warningBytes, limitBytes,
			upstream.c_str());

	hidl_cb(true, NULL);
	return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
