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

#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack_tcp.h>
#include <linux/netdevice.h>
#include <pthread.h>

#include "TetherOffload.h"
#include "OffloadIoctl.h"
#include "OffloadControlV1_0.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

#define PORT_TABLE_PORT_L(port)	(port & (0x7FF))	/* [10:0] */
#define PORT_TABLE_PORT_H(port)	((0xFF00 & port) >> 8)	/* [15:8] */

struct conntrackInst {
	int ctFd;
	struct nfct_handle *ctHandle;
	struct nfct_filter *ctFilter;
	bool filterAttached;
};

struct localAddrInfo {
	bool validIndex;
	int index;
	bool validDevAddr;
	uint8_t devAddr[ETH_ALEN];
};

struct localPortInfo {
	uint32_t localAddr;
	struct nat_local_port portParam;
};

class ConntrackMgr : public TetherOffload {
public:
	static ConntrackMgr *getInstance();
	~ConntrackMgr();
	void resetValues() override;

	bool setConntrackFd(enum ctFamily family, int fd);
	void setOffloadControl(OffloadControl *control);
	void setUpstreamIpv4Addr(string &addr);
	bool detachFilters(bool createAgain);
	bool attachFilters();
	bool addLocalPrefixFilterAttr(string &addr, uint8_t netmask);
	void setLocalDevAddr(bool valid, uint32_t addr, uint8_t *devAddr);
	void removeDownstreamLocalAddr(uint32_t addr, uint8_t netmask);
	static void *conntrackUdpThread(void *arg);
	static void *conntrackTcpThread(void *arg);

private:
	ConntrackMgr();
	void conntrackMonitor(enum ctFamily family);

	static bool isCallbackReady(enum ctFamily family,
		struct nf_conntrack *ct, int &verdict);
	static int conntrackUdpCallback(enum nf_conntrack_msg_type type,
		struct nf_conntrack *ct, void *data);
	static int conntrackTcpCallback(enum nf_conntrack_msg_type type,
		struct nf_conntrack *ct, void *data);
	static int findNextLocalIndex();
	static void setLocalAddrPort(enum ctFamily family,
		struct nf_conntrack *ct, bool enable);
	static void removeLocalAddr(uint32_t addr);
	static void removeLocalAddrByIndex(int index);

	void addCommonIgnoredFilterAttr();
	bool attachUdpFilter();
	bool attachTcpFilter();

	static inline ConntrackMgr *sInstance = NULL;
	static inline OffloadControl *mControl = NULL;
	static inline struct nf_conntrack *ctUpstream = NULL;

	static inline pthread_mutex_t mCallbackLock = PTHREAD_MUTEX_INITIALIZER;
	static inline map<uint32_t, struct localAddrInfo> mLocalAddrInfo;
	static inline bool mLocalAddrIndexOccupied[HW_REG_NAT_LOCAL_ADDR_MAX];
	/* max: 2048 */
	static inline map<uint16_t, struct localPortInfo> mLocalPortAddrTable;

	struct conntrackInst ctInst[CT_MAX];
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
