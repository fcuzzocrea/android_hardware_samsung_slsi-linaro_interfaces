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

#include <libnfnetlink/libnfnetlink.h>

#include "TetherOffload.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

class NetlinkMgr : public TetherOffload {
public:
	static NetlinkMgr *getInstance();
	~NetlinkMgr();
	void resetValues() override;

	void setOffloadControl(OffloadControl *control);
	static void *netlinkThread(void *arg);

private:
	NetlinkMgr();

	/* copied from nfnl_catch() due to "struct nfgenmsg".
	 * RTM_NEWNEIGH uses "struct ndmsg" instead.
	 */
	int netlinkCatch(struct nfnl_handle *h);
	void netlinkMonitor();

	static void netlinkSetDevAddr(bool valid, struct nlmsghdr *nlh, struct nfattr *nfa[]);
	static int netlinkCallback(struct nlmsghdr *nlh, struct nfattr *nfa[], void *data);

	static inline NetlinkMgr *sInstance = NULL;
	static inline OffloadControl *mControl = NULL;
	static inline struct ndmsg *ndmCbData = NULL;

	struct nfnl_handle *nfnlh;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
