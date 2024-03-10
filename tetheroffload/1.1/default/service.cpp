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

#include "OffloadConfig.h"
#include "OffloadControlV1_1.h"
#include "ConntrackMgr.h"
#include "NetlinkMgr.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_1::IOffloadControl;

using vendor::samsung_slsi::hardware::tetheroffload::V1_0::implementation::OffloadConfig;
using vendor::samsung_slsi::hardware::tetheroffload::V1_1::implementation::OffloadControl;
using vendor::samsung_slsi::hardware::tetheroffload::V1_0::implementation::ConntrackMgr;
using vendor::samsung_slsi::hardware::tetheroffload::V1_0::implementation::NetlinkMgr;

int main()
{
	OffloadConfig *config = new OffloadConfig();
	OffloadControl *control = new OffloadControl(config);
	vendor::samsung_slsi::hardware::tetheroffload::V1_0::implementation::OffloadControl *controlV1_0 = control;

	if (ConntrackMgr::getInstance())
		ConntrackMgr::getInstance()->setOffloadControl(controlV1_0);
	if (NetlinkMgr::getInstance())
		NetlinkMgr::getInstance()->setOffloadControl(controlV1_0);

	android::sp<IOffloadConfig> iConfig = config;
	android::sp<IOffloadControl> iControl = control;

	configureRpcThreadpool(4, true /*callerWillJoin*/);

	auto registrar = android::hardware::LazyServiceRegistrar::getInstance();
	if (registrar.registerService(iControl) == android::OK &&
			registrar.registerService(iConfig) == android::OK) {
		HALLOGI("TetherOffload HAL 1.1 Ready");
		joinRpcThreadpool();
	}

	HALLOGI("Cannot register TetherOffload HAL service");
	return 0;
}
