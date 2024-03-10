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

#include <arpa/inet.h>

#include "ConntrackMgr.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

ConntrackMgr::ConntrackMgr()
{
	for (int i = 0; i < CT_MAX; i++) {
		ctInst[i].ctFd = -1;
		ctInst[i].ctHandle = NULL;
		ctInst[i].ctFilter = NULL;
		ctInst[i].filterAttached = false;
	}
}

ConntrackMgr::~ConntrackMgr()
{
	resetValues();

	for (int i = 0; i < CT_MAX; i++) {
		if (ctInst[i].ctHandle) {
			nfct_callback_unregister(ctInst[i].ctHandle);
			nfct_close2(ctInst[i].ctHandle, true);
		}
	}

	sInstance = NULL;
}

void ConntrackMgr::resetValues()
{
	detachFilters(false);

	if (ctUpstream) {
		nfct_destroy(ctUpstream);
		ctUpstream = NULL;
	}

	pthread_mutex_lock(&mCallbackLock);
	mLocalAddrInfo.clear();
	for (int i = 0; i < HW_REG_NAT_LOCAL_ADDR_MAX; i++)
		mLocalAddrIndexOccupied[i] = false;
	mLocalPortAddrTable.clear();
	pthread_mutex_unlock(&mCallbackLock);
}

ConntrackMgr *ConntrackMgr::getInstance()
{
	if (sInstance)
		return sInstance;

	sInstance = new ConntrackMgr();
	if (!sInstance)
		return NULL;

	return sInstance;
}

bool ConntrackMgr::setConntrackFd(enum ctFamily family, int fd)
{
	unsigned int subscriptions[CT_MAX] = {
		(NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY),
		(NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY),
	};

	ctInst[family].ctFd = fd;
	ctInst[family].ctHandle = nfct_open2(CONNTRACK, subscriptions[family], ctInst[family].ctFd);
	if (!ctInst[family].ctHandle) {
		HALLOGE("failed to open nfct");
		return false;
	}

	return true;
}

void ConntrackMgr::setOffloadControl(OffloadControl *control)
{
	mControl = control;
}

void ConntrackMgr::setUpstreamIpv4Addr(string &addr)
{
	if (addr.empty()) {
		if (ctUpstream) {
			nfct_destroy(ctUpstream);
			ctUpstream = NULL;
		}
		return;
	}

	if (!ctUpstream)
		ctUpstream = nfct_new();

	nfct_set_attr_u32(ctUpstream, ATTR_REPL_IPV4_DST, inet_addr(addr.c_str()));
}

bool ConntrackMgr::addLocalPrefixFilterAttr(string &addr, uint8_t netmask)
{
	struct nfct_filter_ipv4 filter_ipv4 = {
		.addr = ntohl(inet_addr(addr.c_str())),
		.mask = (0xFFFFFFFF << (32 - netmask)),
	};

	for (int i = 0; i < CT_MAX; i++) {
		/* Error will be returend when multiple set */
		nfct_filter_set_logic(ctInst[i].ctFilter,
			NFCT_FILTER_SRC_IPV4, NFCT_FILTER_LOGIC_NEGATIVE);
		nfct_filter_add_attr(ctInst[i].ctFilter, NFCT_FILTER_SRC_IPV4, &filter_ipv4);

		nfct_filter_set_logic(ctInst[i].ctFilter,
			NFCT_FILTER_DST_IPV4, NFCT_FILTER_LOGIC_NEGATIVE);
		nfct_filter_add_attr(ctInst[i].ctFilter, NFCT_FILTER_DST_IPV4, &filter_ipv4);
	}

	return true;
}

void ConntrackMgr::setLocalDevAddr(bool valid, uint32_t addr, uint8_t *devAddr)
{
	if (!addr)
		return;

	pthread_mutex_lock(&mCallbackLock);
	mLocalAddrInfo[addr].validDevAddr = valid;
	if (valid && devAddr)
		memcpy(mLocalAddrInfo[addr].devAddr, devAddr, ETH_ALEN);
	pthread_mutex_unlock(&mCallbackLock);
}

void ConntrackMgr::addCommonIgnoredFilterAttr()
{
	struct nfct_filter_ipv4 filter_ipv4;

	filter_ipv4.addr = 0xFFFFFFFF;
	filter_ipv4.mask = 0xFFFFFFFF;

	nfct_filter_set_logic(ctInst[CT_UDP].ctFilter,
		NFCT_FILTER_DST_IPV4, NFCT_FILTER_LOGIC_NEGATIVE);
	nfct_filter_add_attr(ctInst[CT_UDP].ctFilter, NFCT_FILTER_DST_IPV4, &filter_ipv4);
}


bool ConntrackMgr::attachUdpFilter()
{
	struct conntrackInst *inst = &ctInst[CT_UDP];

	if (nfct_filter_set_logic(inst->ctFilter,
			NFCT_FILTER_L4PROTO, NFCT_FILTER_LOGIC_POSITIVE) == -1) {
		HALLOGE("failed to set UDP filter logic");
		return false;
	}
	nfct_filter_add_attr_u32(inst->ctFilter, NFCT_FILTER_L4PROTO, IPPROTO_UDP);

	if (nfct_filter_attach(nfct_fd(inst->ctHandle), inst->ctFilter) != 0) {
		HALLOGE("failed to attach UDP filter");
		return false;
	}

	inst->filterAttached = true;
	return true;
}

bool ConntrackMgr::attachTcpFilter()
{
	struct nfct_filter_proto filter_proto;
	struct conntrackInst *inst = &ctInst[CT_TCP];

	if (nfct_filter_set_logic(inst->ctFilter,
			NFCT_FILTER_L4PROTO, NFCT_FILTER_LOGIC_POSITIVE) == -1) {
		HALLOGE("failed to set TCP filter logic");
		return false;
	}
	nfct_filter_add_attr_u32(inst->ctFilter, NFCT_FILTER_L4PROTO, IPPROTO_TCP);

	if (nfct_filter_set_logic(inst->ctFilter,
			NFCT_FILTER_L4PROTO_STATE, NFCT_FILTER_LOGIC_POSITIVE) == -1) {
		HALLOGE("failed to set ESTABLISHED filter logic");
		return false;
	}
	filter_proto.proto = IPPROTO_TCP;
	filter_proto.state = TCP_CONNTRACK_ESTABLISHED;
	nfct_filter_add_attr(inst->ctFilter, NFCT_FILTER_L4PROTO_STATE, &filter_proto);

	if (nfct_filter_set_logic(inst->ctFilter,
			NFCT_FILTER_L4PROTO_STATE, NFCT_FILTER_LOGIC_POSITIVE) == -1) {
		HALLOGE("failed to set FIN_WAIT filter logic");
		return false;
	}
	filter_proto.proto = IPPROTO_TCP;
	filter_proto.state = TCP_CONNTRACK_FIN_WAIT;
	nfct_filter_add_attr(inst->ctFilter, NFCT_FILTER_L4PROTO_STATE, &filter_proto);

	if (nfct_filter_attach(nfct_fd(inst->ctHandle), inst->ctFilter) != 0) {
		HALLOGE("failed to attach TCP filter");
		return false;
	}

	inst->filterAttached = true;
	return true;
}

bool ConntrackMgr::detachFilters(bool createAgain)
{
	for (int i = 0; i < CT_MAX; i++) {
		ctInst[i].filterAttached = false;
		if (ctInst[i].ctHandle)
			nfct_filter_detach(nfct_fd(ctInst[i].ctHandle));
		if (ctInst[i].ctFilter) {
			nfct_filter_destroy(ctInst[i].ctFilter);
			ctInst[i].ctFilter = NULL;
		}
	}

	if (!createAgain)
		return true;

	for (int i = 0; i < CT_MAX; i++) {
		ctInst[i].ctFilter = nfct_filter_create();
		if (!ctInst[i].ctFilter) {
			HALLOGI("failed to create filter family: %d", i);
			return false;
		}
	}

	return true;
}

bool ConntrackMgr::attachFilters()
{
	addCommonIgnoredFilterAttr();

	if (!attachUdpFilter()){
		HALLOGI("failed to attach UDP filter");
		return false;
	}

	if (!attachTcpFilter()) {
		HALLOGI("failed to attach TCP filter");
		return false;
	}

	return true;
}

void *ConntrackMgr::conntrackUdpThread(void *arg)
{
	if (getInstance())
		getInstance()->conntrackMonitor(CT_UDP);
	return nullptr;
}

void *ConntrackMgr::conntrackTcpThread(void *arg)
{
	if (getInstance())
		getInstance()->conntrackMonitor(CT_TCP);
	return nullptr;
}

void ConntrackMgr::conntrackMonitor(enum ctFamily family)
{
	int ret;

	HALLOGI("+++ family: %d", family);

	switch (family) {
	case CT_UDP:
		nfct_callback_register(ctInst[family].ctHandle,
			(nf_conntrack_msg_type)(NFCT_T_NEW | NFCT_T_DESTROY),
			conntrackUdpCallback, NULL);
		break;
	case CT_TCP:
		nfct_callback_register(ctInst[family].ctHandle,
			(nf_conntrack_msg_type)(NFCT_T_UPDATE | NFCT_T_DESTROY),
			conntrackTcpCallback, NULL);
		break;
	default:
		break;
	}

	ret = nfct_catch(ctInst[family].ctHandle);
	if (ret < 0) {
		HALLOGE("nfct catch done ret: %d", ret);
		goto exit;
	}

exit:
	switch (family) {
	case CT_UDP:
		mControl->threadResetNoti(THREAD_CONNTRACK_UDP);
		break;
	case CT_TCP:
		mControl->threadResetNoti(THREAD_CONNTRACK_TCP);
		break;
	default:
		break;
	}

	HALLOGI("--- family: %d", family);

	/* reset values in stop() */
}

bool ConntrackMgr::isCallbackReady(enum ctFamily family,
		struct nf_conntrack *ct, int &verdict)
{
	if (!getInstance()) {
		verdict = NFCT_CB_STOP;
		return false;
	}

	if (!getInstance()->ctInst[family].filterAttached) {
		verdict = NFCT_CB_CONTINUE;
		return false;
	}

	if (!ctUpstream || !nfct_cmp(ctUpstream, ct, NFCT_CMP_REPL | NFCT_CMP_MASK)) {
		verdict = NFCT_CB_CONTINUE;
		return false;
	}

	return true;
}

int ConntrackMgr::conntrackUdpCallback(enum nf_conntrack_msg_type type,
		struct nf_conntrack *ct, void *data)
{
	int verdict;

	if (!isCallbackReady(CT_UDP, ct, verdict))
		return verdict;

#ifdef TETHER_DEBUG
	static char buf[1024];
	nfct_snprintf(buf, sizeof(buf), ct, NFCT_T_UNKNOWN, NFCT_O_DEFAULT, NFCT_OF_SHOW_LAYER3);
	HALLOGD("ct: %s", buf);
#endif

	/* ToDo: need to ignore timeout */
	if (type & NFCT_T_NEW)
		setLocalAddrPort(CT_UDP, ct, true);
	else
		setLocalAddrPort(CT_UDP, ct, false);

	return NFCT_CB_CONTINUE;
}

int ConntrackMgr::conntrackTcpCallback(enum nf_conntrack_msg_type type,
		struct nf_conntrack *ct, void *data)
{
	uint8_t tcp_state;
	int verdict;

	if (!isCallbackReady(CT_TCP, ct, verdict))
		return verdict;

#ifdef TETHER_DEBUG
	static char buf[1024];
	nfct_snprintf(buf, sizeof(buf), ct, NFCT_T_UNKNOWN, NFCT_O_DEFAULT, NFCT_OF_SHOW_LAYER3);
	HALLOGD("ct: %s", buf);
#endif

	tcp_state = nfct_get_attr_u8(ct, ATTR_TCP_STATE);
	if (tcp_state == TCP_CONNTRACK_ESTABLISHED)
		setLocalAddrPort(CT_TCP, ct, true);
	else if ((type & NFCT_T_DESTROY) || tcp_state == TCP_CONNTRACK_FIN_WAIT)
		setLocalAddrPort(CT_TCP, ct, false);

	return NFCT_CB_CONTINUE;
}

int ConntrackMgr::findNextLocalIndex()
{
	static int index = -1;

	if (++index >= HW_REG_NAT_LOCAL_ADDR_MAX)
		index = 0;

	if (mLocalAddrIndexOccupied[index]) {
		HALLOGI("overwrite local addr index: %d", index);
		removeLocalAddrByIndex(index);
	}

	return index;
}

void ConntrackMgr::removeLocalAddr(uint32_t addr)
{
	struct nat_local_addr addrParam;
	struct nat_local_port portParam;
	int index;

	if (!addr || (mLocalAddrInfo.find(addr) == mLocalAddrInfo.end()))
		return;

	index = mLocalAddrInfo[addr].index;
	HALLOGI("remove local addr 0x%08X at index: %d", addr, index);

	/* clean up port table */
	auto iter = mLocalPortAddrTable.begin();
	while (iter != mLocalPortAddrTable.end()) {
		uint16_t curIndex = iter->first;
		iter++;

		if (mLocalPortAddrTable[curIndex].localAddr != addr)
			continue;

		mLocalPortAddrTable.erase(curIndex);
		portParam.reply_port_dst_l = curIndex;
		portParam.hw_val = 0;
		if (ioctlOffload(OFFLOAD_IOCTL_SET_NAT_LOCAL_PORT, &portParam) < 0)
			HALLOGI("failed to remove port index: %d", portParam.reply_port_dst_l);
	}

	/* clean up local addr */
	mLocalAddrInfo.erase(addr);
	mLocalAddrIndexOccupied[index] = false;

	addrParam.index = index;
	addrParam.addr = 0;
	if (ioctlOffload(OFFLOAD_IOCTL_SET_NAT_LOCAL_ADDR, &addrParam) < 0)
		HALLOGI("failed to remove local addr 0x%08X at index: %d", addr, index);
}

void ConntrackMgr::removeLocalAddrByIndex(int index)
{
	/* get the matched address */
	for (auto& [curAddr, curInfo]: mLocalAddrInfo) {
		if (curInfo.index == index) {
			removeLocalAddr(curAddr);
			break;
		}
	}
}

void ConntrackMgr::removeDownstreamLocalAddr(uint32_t addr, uint8_t netmask)
{
	uint32_t curAddr;
	uint32_t maskBits = (0xFFFFFFFF << (32 - netmask));

	pthread_mutex_lock(&mCallbackLock);
	auto iter = mLocalAddrInfo.begin();
	while (iter != mLocalAddrInfo.end()) {
		curAddr = iter->first;
		iter++;

		if ((curAddr & maskBits) == (addr & maskBits))
			removeLocalAddr(curAddr);
	}
	pthread_mutex_unlock(&mCallbackLock);
}

void ConntrackMgr::setLocalAddrPort(enum ctFamily family,
		struct nf_conntrack *ct, bool enable)
{
	struct nat_local_addr addrParam;
	struct nat_local_port portParam;
	uint16_t replyPortDst;
	uint16_t originPortSrc;
	uint32_t addrh;
	uint16_t index;
	int dstRing;
	int ret = 0;

	/* get addr (IPv4 only) */
	addrParam.addr = nfct_get_attr_u32(ct, ATTR_ORIG_IPV4_SRC);
	addrh = ntohl(addrParam.addr);
	dstRing = mControl->getDownstreamDstRing(addrh);

	if (!addrh || (dstRing < 0))
		return;

	/* check ports (little endian as default) */
	replyPortDst = ntohs(nfct_get_attr_u16(ct, ATTR_REPL_PORT_DST));
	originPortSrc = ntohs(nfct_get_attr_u16(ct, ATTR_ORIG_PORT_SRC));
	if (mControl->hwCapaMatched(DIT_CAP_MASK_PORT_BIG_ENDIAN)) {
		replyPortDst = htons(replyPortDst);
		originPortSrc = htons(originPortSrc);
	}

	if (!replyPortDst || !originPortSrc)
			return;

	pthread_mutex_lock(&mCallbackLock);

	/* get addr index */
	if ((mLocalAddrInfo.find(addrh) != mLocalAddrInfo.end()) && mLocalAddrInfo[addrh].validIndex)
		addrParam.index = mLocalAddrInfo[addrh].index;
	else {
		/* do not need to add addr if disable */
		if (!enable) {
			ret = -1;
			goto exit;
		}

		addrParam.index = findNextLocalIndex();
		mLocalAddrInfo[addrh].index = addrParam.index;
		mLocalAddrInfo[addrh].validIndex = true;
		HALLOGI("add local addr 0x%08X to index: %d, dst: %d", addrh, addrParam.index, dstRing);
	}

	portParam.hw_val = 0;
	portParam.enable = (enable ? 1: 0);
	portParam.reply_port_dst_l = PORT_TABLE_PORT_L(replyPortDst);
	portParam.dst_ring = dstRing;
	portParam.origin_port_src = originPortSrc;
	portParam.reply_port_dst_h = PORT_TABLE_PORT_H(replyPortDst);
	portParam.addr_index = addrParam.index;
	portParam.is_udp = (family == CT_UDP ? 1 : 0);

	/* set local addr */
	if (!mLocalAddrIndexOccupied[addrParam.index]) {
		/* do not set local addr if dev addr is not set */
		if (!mLocalAddrInfo[addrh].validDevAddr) {
			ret = -1;
			goto exit;
		}

		addrParam.dst_ring = portParam.dst_ring;
		memcpy(addrParam.dev_addr, mLocalAddrInfo[addrh].devAddr, ETH_ALEN);
		mLocalAddrIndexOccupied[addrParam.index] = true;

		if (ioctlOffload(OFFLOAD_IOCTL_SET_NAT_LOCAL_ADDR, &addrParam) < 0) {
			HALLOGI("failed to set local addr: 0x%08X at index: %d",
				addrh, addrParam.index);
			ret = -1;
			goto exit;
		}
	}

	index = portParam.reply_port_dst_l;
	/* set port table */
	if (!enable) {
		/* nothing to remove */
		if (mLocalPortAddrTable.find(index) == mLocalPortAddrTable.end()) {
			ret = -1;
			goto exit;
		}

		/* do not compare enable bit */
		if ((portParam.hw_val & 0xFFFFFFFE) ==
				(mLocalPortAddrTable[index].portParam.hw_val & 0xFFFFFFFE)) {
			mLocalPortAddrTable.erase(index);
			HALLOGD("port table [%04d] removed", index);
		} else {
			ret = -1;
			goto exit;
		}
	} else {
#ifdef TETHER_DEBUG
		if (mLocalPortAddrTable.find(index) != mLocalPortAddrTable.end())
			HALLOGD("port table [%04d] overwritten. is_udp: %d", index, portParam.is_udp);
		else
			HALLOGD("port table [%04d] added", index);
#endif
		mLocalPortAddrTable[index].localAddr = addrh;
		mLocalPortAddrTable[index].portParam = portParam;
	}

exit:
	pthread_mutex_unlock(&mCallbackLock);
	if (ret)
		return;

	if (ioctlOffload(OFFLOAD_IOCTL_SET_NAT_LOCAL_PORT, &portParam) < 0) {
		HALLOGI("failed to set port rule for origin 0x%08X:%d",
			addrh, portParam.origin_port_src);
		return;
	}
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
