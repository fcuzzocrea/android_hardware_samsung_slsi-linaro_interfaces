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
#include <inttypes.h>
#include <pthread.h>
#include <poll.h>
#include <net/if.h>
#include <netutils/ifc.h>
#include "OffloadIoctl.h"
#include "OffloadControlV1_0.h"
#include "ConntrackMgr.h"
#include "NetlinkMgr.h"

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace tetheroffload {
namespace V1_0 {
namespace implementation {

OffloadControl::OffloadControl(OffloadConfig *config)
{
	resetValues();
	mConfig = config;
}

OffloadControl::~OffloadControl()
{
	resetValues();
}

void OffloadControl::resetValues()
{
	mCtrlCb.clear();
	mUpstream.iface.clear();
	mDownstreams.clear();
	memset(&mHwInfo, 0x0, sizeof(struct hw_info));
}

void OffloadControl::startThread(enum threadType type)
{
	int ret = -1;
	pthread_t tid;

	pthread_mutex_lock(&mThreadLock);
	if (mThreads[type].created)
		goto exit;

	switch(type) {
	case THREAD_EVENT:
		ret = pthread_create(&tid, 0, eventThread, this);
		break;
	case THREAD_CONNTRACK_UDP:
		if (ConntrackMgr::getInstance()->setConntrackFd(CT_UDP, mConfig->getUdpConntrackFd()))
			ret = pthread_create(&tid, 0, ConntrackMgr::conntrackUdpThread, NULL);
		break;
	case THREAD_CONNTRACK_TCP:
		if (ConntrackMgr::getInstance()->setConntrackFd(CT_TCP, mConfig->getTcpConntrackFd()))
			ret = pthread_create(&tid, 0, ConntrackMgr::conntrackTcpThread, NULL);
		break;
	case THREAD_NETLINK:
		ret = pthread_create(&tid, 0, NetlinkMgr::netlinkThread, NULL);
		break;
	default:
		break;
	}

	if(ret != 0) {
		HALLOGE("failed to create thread type: %d", type);
		goto exit;
	}

	pthread_detach(tid);
	mThreads[type].created = true;
	mThreads[type].threadId = tid;

exit:
	pthread_mutex_unlock(&mThreadLock);
}

void OffloadControl::threadResetNoti(enum threadType type)
{
	pthread_mutex_lock(&mThreadLock);
	mThreads[type].created = false;
	pthread_mutex_unlock(&mThreadLock);
}

void *OffloadControl::eventThread(void *arg)
{
	int fd = -1;
	struct pollfd pfd;
	struct offload_event event;

	HALLOGI("+++");

	if ((fd = openDevice()) < 0) {
		HALLOGE("failed to open device");
		return nullptr;
	}

	pfd.fd = fd;
	pfd.events = POLLIN | POLLHUP;

	do {
		pfd.revents = 0;
		poll(&pfd, 1, DEV_POLL_TIMEOUT);

		if (pfd.revents & POLLIN) {
			if (read(fd, &event, sizeof(struct offload_event)) == 0)
				continue;

			if (((OffloadControl *)arg)->callbackEvent(event.event_num) < 0)
				break;
		}
	} while (1);

	close(fd);

	threadResetNoti(THREAD_EVENT);
	HALLOGI("---");

	return nullptr;
}

bool OffloadControl::isInitialized() {
	return mCtrlCb.get() != nullptr;
}

int OffloadControl::parseAddress(const string &in, string &out, string &netmask) {
	int af = AF_UNSPEC;
	size_t pos;

	/* address family */
	pos = in.find(":");
	if (pos != string::npos)
		af = AF_INET6;
	else
		af = AF_INET;

	/* extract address and netmask */
	pos = in.find("/");
	if (pos != string::npos && (pos >= 1 && pos < in.size())) {
		out = in.substr(0, pos);
		netmask = in.substr(pos + 1);
	} else if (pos == string::npos) {
		out = in;
		netmask = (af == AF_INET6 ? "128" : "32");
	} else {
		return -1;
	}

	/* validate address */
	if (af == AF_INET6) {
		struct sockaddr_in6 sa;

		if (inet_pton(AF_INET6, out.c_str(), &(sa.sin6_addr)) <= 0)
			return -1;
	}
	else {
		struct sockaddr_in sa;

		if (inet_pton(AF_INET, out.c_str(), &(sa.sin_addr)) <= 0)
			return -1;
	}

	return af;
}

int OffloadControl::checkInterfaceStat(string name)
{
	int ifindex;
	int ret;

	if (regex_match(name.c_str(), regex(UPSTREAM_IFACE_VTS_PATTERN)))
		return 0;

	ret = ifc_init();
	if (ret) {
		HALLOGE("failed to init. iface: %s", name.c_str());
		goto out;
	}

	ret = ifc_get_ifindex(name.c_str(), &ifindex);
	if(ret)
		HALLOGE("failed to get ifindex. iface: %s", name.c_str());

out:
	ifc_close();

	return ret ? -1 : 0;
}

int OffloadControl::callbackEvent(int32_t event)
{
	HALLOGI("received event %d", event);

	if (event == INTERNAL_OFFLOAD_STOPPED) {
		HALLOGI("hw sent stop event");
		return 0;
	}

	if(mCtrlCb.get())
		mCtrlCb->onEvent((OffloadCallbackEvent)event);

	return 0;
}

Return<void> OffloadControl::initOffload(const sp<ITetheringOffloadCallback>& cb,
	initOffload_cb hidl_cb)
{
	if (isInitialized()) {
		hidl_cb(false, "already initialized");
		return Void();
	}

	if (!ConntrackMgr::getInstance()) {
		hidl_cb(false, "can't get conntrack instance");
		return Void();
	}

	if (!NetlinkMgr::getInstance()) {
		hidl_cb(false, "can't get netlink instance");
		return Void();
	}

	if (ioctlOffload(OFFLOAD_IOCTL_GET_HW_INFO, &mHwInfo) < 0) {
		hidl_cb(false, "can't get hw version");
		return Void();
	}

	if (ioctlOffload(OFFLOAD_IOCTL_INIT_OFFLOAD, NULL) < 0) {
		hidl_cb(false, "can't init offload hw");
		return Void();
	}

	mCtrlCb = cb;
	startThread(THREAD_EVENT);
	startThread(THREAD_CONNTRACK_UDP);
	startThread(THREAD_CONNTRACK_TCP);
	startThread(THREAD_NETLINK);

	HALLOGI("init offload done. hw version: 0x%08X, capabilities: 0x%08X",
		mHwInfo.version, mHwInfo.capabilities);

	hidl_cb(true, NULL);
	return Void();
}

Return<void> OffloadControl::stopOffload(stopOffload_cb hidl_cb)
{
	if (!isInitialized()) {
		hidl_cb(false, "not initialized");
		return Void();
	}

	if (ioctlOffload(OFFLOAD_IOCTL_STOP_OFFLOAD, NULL) < 0) {
		hidl_cb(false, "offload hw stop failed");
		return Void();
	}

	/* keep threads and reset values instead of instances deletion */
	resetValues();
	if (ConntrackMgr::getInstance())
		ConntrackMgr::getInstance()->resetValues();
	if (NetlinkMgr::getInstance())
		NetlinkMgr::getInstance()->resetValues();

	HALLOGI("stop offload");

	hidl_cb(true, NULL);
	return Void();
}

Return<void> OffloadControl::setLocalPrefixes(const hidl_vec<hidl_string>& prefixes,
	setLocalPrefixes_cb hidl_cb)
{
	stringstream ssAdd;
	stringstream ssSkip;

	if (!isInitialized()) {
		hidl_cb(false, "not initialized");
		return Void();
	}

	if(prefixes.size() < 1) {
		hidl_cb(false, "no prefix");
		return Void();
	}

	ConntrackMgr::getInstance()->detachFilters(true);

	for (auto prefix : prefixes) {
		string addr, netmask;
		int family = -1;

		if (prefix.empty() || (family = parseAddress(prefix, addr, netmask)) < 0) {
			hidl_cb(false, "prefix parsing error");
			return Void();
		}

		if (family == AF_INET) {
			ConntrackMgr::getInstance()->addLocalPrefixFilterAttr(addr, std::stoi(netmask));
			ssAdd << prefix << " ";
		} else {
			ssSkip << prefix << " ";
		}
	}

	ConntrackMgr::getInstance()->attachFilters();
	HALLOGI("local prefixes added: [%s], skipped: [%s]", ssAdd.str().c_str(), ssSkip.str().c_str());

	hidl_cb(true, NULL);
	return Void();
}

Return<void> OffloadControl::getForwardedStats(const hidl_string& upstream,
	getForwardedStats_cb hidl_cb)
{
	struct forward_stats stats;

	stats.rx_diff = 0;
	stats.tx_diff = 0;

	if (!isInitialized()) {
		HALLOGI("not initialized");
		goto exit;
	}

	strlcpy(stats.iface, upstream.c_str(), IFNAMSIZ);
	if (ioctlOffload(OFFLOAD_IOCTL_GET_FORWD_STATS, &stats) < 0)
		goto exit;

	if (stats.rx_diff > 0 || stats.tx_diff > 0) {
		HALLOGI("%s Current Rx=%" PRIu64 ", Tx=%" PRIu64 " / Total Rx=%" PRIu64 ", Tx=%" PRIu64,
			upstream.c_str(), stats.rx_diff, stats.tx_diff, stats.rx_bytes, stats.tx_bytes);
	}

exit:
	hidl_cb(stats.rx_diff, stats.tx_diff);
	return Void();
}

Return<void> OffloadControl::setDataLimit(const hidl_string& upstream, uint64_t limit,
	setDataLimit_cb hidl_cb)
{
	struct forward_stats stats;

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

	strlcpy(stats.iface, upstream.c_str(), IFNAMSIZ);
	stats.data_limit = limit;
	if (ioctlOffload(OFFLOAD_IOCTL_SET_DATA_LIMIT, &stats) < 0) {
		hidl_cb(false, "can't set data limit");
		return Void();
	}

	HALLOGI("%" PRIu64 " bytes limit applied to %s", limit, upstream.c_str());

	hidl_cb(true, NULL);
	return Void();
}

Return<void> OffloadControl::setUpstreamParameters(const hidl_string& iface,
	const hidl_string& v4Addr, const hidl_string& v4Gw, const hidl_vec<hidl_string>& v6Gws,
	setUpstreamParameters_cb hidl_cb)
{
	struct iface_info param;
	string addr, netmask;
	bool stopOffload = false;

	if (!isInitialized()) {
		hidl_cb(false, "not initialized");
		return Void();
	}

	mUpstream.iface.clear();

	if (v4Addr.empty())
		stopOffload = true;
	if (v4Gw.empty()) {
		/* ipv4 gw could be null in normal */
	}
	if (!v6Gws.size()) {
		/* not consider ipv6 for now */
	}

	/* stop offload */
	if (iface.empty() || stopOffload) {
		param.iface[0] = '\0';
		if (ioctlOffload(OFFLOAD_IOCTL_SET_UPSTRM_PARAM, &param) < 0) {
			hidl_cb(false, "can't set upstream (stop)");
			return Void();
		}

		addr.clear();
		ConntrackMgr::getInstance()->setUpstreamIpv4Addr(addr);
		ConntrackMgr::getInstance()->detachFilters(false);

		if (!iface.empty() && v6Gws.size())
			hidl_cb(true, "stop offload due to ipv6 only");
		else
			hidl_cb(false, "stop offload due to upstream null param");
		return Void();
	}

	if (checkInterfaceStat(iface) < 0) {
		HALLOGI("failed to get upstream stat %s", iface.c_str());
		hidl_cb(false, "upstream stat failed");
		return Void();
	}

	if (!regex_match(iface.c_str(), regex(UPSTREAM_IFACE_PATTERN))) {
		HALLOGI("not supported upstream %s", iface.c_str());
		hidl_cb(false, "not supported upstream");
		return Void();
	}

	mUpstream.iface = iface;
	strlcpy(param.iface, mUpstream.iface.c_str(), IFNAMSIZ);
	if (ioctlOffload(OFFLOAD_IOCTL_SET_UPSTRM_PARAM, &param) < 0) {
		hidl_cb(false, "can't set upstream");
		return Void();
	}

	if (!v4Addr.empty()) {
		if (parseAddress(v4Addr, addr, netmask) != AF_INET) {
			hidl_cb(false, "v4Addr parsing error");
			return Void();
		}
		mUpstream.v4Addr = addr;
		ConntrackMgr::getInstance()->setUpstreamIpv4Addr(addr);
	}

	if (!v4Gw.empty()) {
		if (parseAddress(v4Gw, addr, netmask) != AF_INET) {
			hidl_cb(false, "v4Gw parsing error");
			return Void();
		}
		mUpstream.v4Gw = addr;
	}

	mUpstream.v6Gws.clear();
	for (auto v6gw : v6Gws) {
		if (v6gw.empty() || (parseAddress(v6gw, addr, netmask) != AF_INET6)) {
			hidl_cb(false, "v6Gws parsing error");
			return Void();
		}
		mUpstream.v6Gws.push_back(addr);
	}

	HALLOGI("set upstream iface: %s", iface.c_str());

	hidl_cb(true, NULL);
	return Void();
}

Return<void> OffloadControl::addDownstream(const hidl_string& iface,
	const hidl_string& prefix, addDownstream_cb hidl_cb)
{
	int family = -1;
	string addr, netmask;
	struct iface_info param;

	/* rndis0, 192.168.42.0/24 */
	if (!isInitialized()) {
		hidl_cb(false, "not initialized");
		return Void();
	}

	if (iface.empty()) {
		hidl_cb(false, "iface empty");
		return Void();
	}

	if (checkInterfaceStat(iface) < 0) {
		HALLOGI("failed to get downstream stat %s", iface.c_str());
		hidl_cb(false, "downstream stat failed");
		return Void();
	}

	if ((family = parseAddress(prefix, addr, netmask)) < 0) {
		hidl_cb(false, "prefix parsing error");
		return Void();
	}

	/* IPv4 only for HW NAT */
	if (family == AF_INET) {
		mDownstreams.erase(iface);

		strlcpy(param.iface, iface.c_str(), IFNAMSIZ);
		if (ioctlOffload(OFFLOAD_IOCTL_ADD_DOWNSTREAM, &param) < 0) {
			hidl_cb(false, "can't add a downstream");
			return Void();
		}

		mDownstreams[iface].iface = iface;
		mDownstreams[iface].v4Addr = ntohl(inet_addr(addr.c_str()));
		mDownstreams[iface].v4Mask = std::stoi(netmask);
		mDownstreams[iface].dstRing = param.dst_ring;

		HALLOGI("add downstream %s(%s) at dst: %d",
			iface.c_str(), prefix.c_str(), param.dst_ring);
	} else {
		HALLOGI("skip adding downstream %s, family: %d", iface.c_str(), family);
	}

	hidl_cb(true, NULL);
	return Void();
}

Return<void> OffloadControl::removeDownstream(const hidl_string& iface,
	const hidl_string& prefix, removeDownstream_cb hidl_cb)
{
	int family = -1;
	string addr, netmask;
	struct iface_info param;
	bool ret = true;

	if (!isInitialized()) {
		hidl_cb(false, "not initialized");
		return Void();
	}

	if (iface.empty()) {
		hidl_cb(false, "iface empty");
		return Void();
	}

	if (checkInterfaceStat(iface) < 0) {
		HALLOGI("failed to get downstream stat %s", iface.c_str());
		/* do not return here. give a chance to call ioctl. */
		ret = false;
	}

	if ((family = parseAddress(prefix, addr, netmask)) < 0) {
		hidl_cb(false, "prefix parsing error");
		return Void();
	}

	if (family == AF_INET) {
		if (mDownstreams.find(iface) != mDownstreams.end()) {
			HALLOGI("mDownstreams: %s", iface.c_str());

			ConntrackMgr::getInstance()->removeDownstreamLocalAddr(
				mDownstreams[iface].v4Addr,
				mDownstreams[iface].v4Mask);
			mDownstreams.erase(iface);

			strlcpy(param.iface, iface.c_str(), IFNAMSIZ);
			if (ioctlOffload(OFFLOAD_IOCTL_REMOVE_DOWNSTRM, &param) < 0) {
				hidl_cb(false, "can't remove a downstream");
				return Void();
			}

			HALLOGI("remove downstream %s(%s)", iface.c_str(), prefix.c_str());
		} else {
			hidl_cb(false, "remove downstream not added");
			return Void();
		}
	} else {
		HALLOGI("skip removing downstream %s, family: %d", iface.c_str(), family);
	}

	if (ret)
		hidl_cb(true, NULL);
	else
		hidl_cb(false, "failed to remove downstream");
	return Void();
}

int OffloadControl::getDownstreamDstRing(uint32_t originSrcAddr)
{
	uint32_t maskBits;

	for (auto& [iface, info]: mDownstreams) {
		maskBits = (0xFFFFFFFF << (32 - info.v4Mask));
		if ((info.v4Addr & maskBits) == (originSrcAddr & maskBits))
			return info.dstRing;
	}

	return -1;
}

bool OffloadControl::hwCapaMatched(enum hw_capabilities_mask mask)
{
	if (!mHwInfo.version) {
		HALLOGI("empty hw version");
		return false;
	}

	if (mHwInfo.capabilities & mask)
		return true;

	return false;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
