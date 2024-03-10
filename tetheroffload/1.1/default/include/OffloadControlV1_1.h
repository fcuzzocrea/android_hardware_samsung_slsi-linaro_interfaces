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

#pragma once

#include <android/hardware/tetheroffload/control/1.1/IOffloadControl.h>
#include "OffloadIoctl.h"
#include "OffloadConfig.h"
#include "OffloadControlV1_0.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_1 {
namespace implementation {

using android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using android::hardware::tetheroffload::control::V1_1::IOffloadControl;

using initOffload_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::initOffload_cb;
using stopOffload_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::stopOffload_cb;
using setLocalPrefixes_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::setLocalPrefixes_cb;
using getForwardedStats_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::getForwardedStats_cb;
using setDataLimit_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::setDataLimit_cb;
using setUpstreamParameters_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::setUpstreamParameters_cb;
using addDownstream_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::addDownstream_cb;
using removeDownstream_cbV1_0 = android::hardware::tetheroffload::control::V1_0::IOffloadControl::removeDownstream_cb;

using V1_0::implementation::OffloadConfig;

class OffloadControl : public V1_0::implementation::OffloadControl, public IOffloadControl {
public:
	OffloadControl(OffloadConfig *config) : V1_0::implementation::OffloadControl(config) {}

	Return<void> initOffload(const ::android::sp<ITetheringOffloadCallback>& cb,
			initOffload_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::initOffload(cb, hidl_cb);
	}
	Return<void> stopOffload(stopOffload_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::stopOffload(hidl_cb);
	}
	Return<void> setLocalPrefixes(const hidl_vec<hidl_string>& prefixes,
			setLocalPrefixes_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::setLocalPrefixes(prefixes, hidl_cb);
	}
	Return<void> getForwardedStats(const hidl_string& upstream,
			getForwardedStats_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::getForwardedStats(upstream, hidl_cb);
	}
	Return<void> setDataLimit(const hidl_string& upstream, uint64_t limit,
			setDataLimit_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::setDataLimit(upstream, limit, hidl_cb);
	}
	Return<void> setUpstreamParameters(const hidl_string& iface,
			const hidl_string& v4Addr, const hidl_string& v4Gw,
			const hidl_vec<hidl_string>& v6Gws, setUpstreamParameters_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, hidl_cb);
	}
	Return<void> addDownstream(const hidl_string& iface, const hidl_string& prefix,
			addDownstream_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::addDownstream(iface, prefix, hidl_cb);
	}
	Return<void> removeDownstream(const hidl_string& iface,
			const hidl_string& prefix, removeDownstream_cbV1_0 hidl_cb) override {
		return V1_0::implementation::OffloadControl::removeDownstream(iface, prefix, hidl_cb);
	}
	Return<void> setDataWarningAndLimit(const hidl_string& upstream,
		uint64_t warningBytes, uint64_t limitBytes, setDataWarningAndLimit_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
