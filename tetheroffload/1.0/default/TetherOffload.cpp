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
#include "TetherOffload.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

TetherOffload::TetherOffload()
{
}

TetherOffload::~TetherOffload()
{
}

int TetherOffload::openDevice()
{
	static int selected_dev = -1;
	int dev_min, dev_max;
	int fd = -1;

	if (selected_dev < 0) {
		dev_min = 0;
		dev_max = ARRAY_LENGTH(DEVICE_OFFLOAD);
	} else {
		dev_min = selected_dev;
		dev_max = selected_dev + 1;
	}

	for (int i = dev_min; i < dev_max; i++) {
		if ((fd = open(DEVICE_OFFLOAD[i], O_RDWR | O_CLOEXEC)) >= 0) {
			selected_dev = i;
			break;
		}
	}

	return fd;
}

int TetherOffload::ioctlOffload(int cmd, void *arg)
{
	int ret = 0, fd = -1;
	void *param = arg;

	if ((fd = openDevice()) < 0) {
		HALLOGE("failed to open device");
		return -1;
	}

	if (ioctl(fd, cmd, param) < 0) {
		ret = -1;
	}

	close(fd);
	return ret;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
