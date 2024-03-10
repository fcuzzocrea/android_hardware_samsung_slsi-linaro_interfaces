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

#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <net/if.h>

#include "OffloadIoctl.h"
#include "ConntrackMgr.h"
#include "NetlinkMgr.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

NetlinkMgr::NetlinkMgr()
{
	nfnlh = NULL;
}

NetlinkMgr::~NetlinkMgr()
{
	if (nfnlh)
		nfnl_close(nfnlh);

	sInstance = NULL;
}

void NetlinkMgr::resetValues() {}

NetlinkMgr *NetlinkMgr::getInstance()
{
	if (sInstance)
		return sInstance;

	sInstance = new NetlinkMgr();
	if (!sInstance)
		return NULL;

	return sInstance;
}

void NetlinkMgr::setOffloadControl(OffloadControl *control)
{
	mControl = control;
}

int NetlinkMgr::netlinkCatch(struct nfnl_handle *h)
{
	int ret;
	unsigned char *bufData;
	unsigned char *modData;
	struct nlmsghdr *nlh;
	int payloadLen = 0;

	assert(h);

	while (1) {
		unsigned char buf[NFNL_BUFFSIZE]
			__attribute__ ((aligned));
		unsigned char mod[NFNL_BUFFSIZE]
			__attribute__ ((aligned));

		ret = nfnl_recv(h, buf, sizeof(buf));
		if (ret == -1) {
			/* interrupted syscall must retry */
			if (errno == EINTR)
				continue;
			break;
		}

		/* copy header */
		memcpy(mod, buf, NLMSG_HDRLEN);

		/* store callback data */
		bufData = (unsigned char *) NLMSG_DATA(buf);
		modData = (unsigned char *) NLMSG_DATA(mod);
		ndmCbData = (struct ndmsg *) bufData;

		/* adjust buffer */
		nlh = (struct nlmsghdr *) mod;
		payloadLen = nlh->nlmsg_len - NLMSG_HDRLEN - NLMSG_ALIGN(sizeof(struct ndmsg));
		memcpy(NFM_NFA(modData), bufData + NLMSG_ALIGN(sizeof(struct ndmsg)), payloadLen);
		nlh->nlmsg_len -= (NLMSG_ALIGN(sizeof(struct ndmsg)) -
			NLMSG_ALIGN(sizeof(struct nfgenmsg)));

		ret = nfnl_process(h, mod, ret);
		if (ret <= NFNL_CB_STOP)
			break;
	}

	return ret;
}

void *NetlinkMgr::netlinkThread(void *arg)
{
	if (getInstance())
		getInstance()->netlinkMonitor();
	return nullptr;
}

void NetlinkMgr::netlinkMonitor()
{
	struct nfnl_subsys_handle *nfssh;
	struct nfnl_callback nfcb;
	u_int32_t subscriptions = 0;
	int ret;
	int fd = -1;

	HALLOGI("+++");

	fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (fd < 0)
		goto exit;

	nfnlh = nfnl_open2(fd, false);
	if (!nfnlh)
		goto exit;

	subscriptions = RTMGRP_NEIGH;
	nfssh = nfnl_subsys_open(nfnlh, NFNL_SUBSYS_NONE, RTM_MAX, subscriptions);
	if (!nfssh)
		goto exit;

	nfcb.call = netlinkCallback;
	nfcb.data = (void *)&ndmCbData;
	nfcb.attr_count = NDA_MAX;
	nfnl_callback_register(nfssh, RTM_NEWNEIGH, &nfcb);
	nfnl_callback_register(nfssh, RTM_DELNEIGH, &nfcb);

	ret = netlinkCatch(nfnlh);
	if (ret < 0) {
		HALLOGI("nfnl catch done ret: %d", ret);
		goto exit;
	}

exit:
	mControl->threadResetNoti(THREAD_NETLINK);
	HALLOGI("---");
}

void NetlinkMgr::netlinkSetDevAddr(bool valid, struct nlmsghdr *nlh, struct nfattr *nfa[])
{
	uint32_t ipAddr = 0;
	uint8_t *devAddr = NULL;

	if (nfa[NDA_DST-1] &&
			(NFA_PAYLOAD(nfa[NDA_DST-1]) == sizeof(uint32_t)))
		ipAddr = ntohl(*((uint32_t *)NFA_DATA(nfa[NDA_DST-1])));

	if (nfa[NDA_LLADDR-1] &&
			(NFA_PAYLOAD(nfa[NDA_LLADDR-1]) == ETH_ALEN))
		devAddr = (uint8_t *) NFA_DATA(nfa[NDA_LLADDR-1]);

	if (!ipAddr)
		return;
	if (valid && !devAddr)
		return;

	HALLOGD("neigh: valid: %d, ip: 0x%08X, dev: 0x%02X%02X%02X%02X%02X%02X",
		valid, ipAddr,
		devAddr[0], devAddr[1], devAddr[2], devAddr[3], devAddr[4], devAddr[5]);

	ConntrackMgr::getInstance()->setLocalDevAddr(valid, ipAddr, devAddr);
}

int NetlinkMgr::netlinkCallback(struct nlmsghdr *nlh, struct nfattr *nfa[], void *data)
{
	struct ndmsg *ndm = *((struct ndmsg **) data);

	/* check IPv4 only */
	if (ndm->ndm_family != AF_INET)
		return NFNL_CB_CONTINUE;

	switch (nlh->nlmsg_type) {
	case RTM_NEWNEIGH:
		netlinkSetDevAddr(true, nlh, nfa);
		break;
	case RTM_DELNEIGH:
		netlinkSetDevAddr(false, nlh, nfa);
		break;
	default:
		break;
	};

	return NFNL_CB_CONTINUE;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
