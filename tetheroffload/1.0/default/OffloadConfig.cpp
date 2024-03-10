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

#include "OffloadIoctl.h"
#include "OffloadConfig.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

OffloadConfig::OffloadConfig() {}

OffloadConfig::~OffloadConfig() {}

void OffloadConfig::resetValues() {}

Return<void> OffloadConfig::setHandles(const hidl_handle &fd1, const hidl_handle &fd2,
	setHandles_cb hidl_cb)
{
	if (fd1->numFds != 1 || fd2->numFds != 1) {
		hidl_cb(false, "invalid handles");
		return Void();
	}

	/* must contain the handles or these are lost */
	sockHandle[CT_UDP] = fd1;
	sockHandle[CT_TCP] = fd2;

	hidl_cb(true, NULL);
	return Void();
}

int OffloadConfig::getUdpConntrackFd()
{
	return sockHandle[CT_UDP]->data[0];
}

int OffloadConfig::getTcpConntrackFd()
{
	return sockHandle[CT_TCP]->data[0];
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
