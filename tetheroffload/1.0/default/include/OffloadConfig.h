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

#include "TetherOffload.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;

class OffloadConfig : public TetherOffload, public IOffloadConfig {
public:
	OffloadConfig();
	~OffloadConfig();

	Return<void> setHandles(const hidl_handle &fd1, const hidl_handle &fd2,
		setHandles_cb hidl_cb) override;
	int getUdpConntrackFd();
	int getTcpConntrackFd();

private:
	void resetValues() override;

	hidl_handle sockHandle[CT_MAX];
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
