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

#include <linux/if.h>
#include <linux/netdevice.h>
#include <sys/ioctl.h>

struct iface_info {
	char iface[IFNAMSIZ];
	uint16_t dst_ring;
} __packed;

struct forward_stats {
	char iface[IFNAMSIZ];
	uint64_t data_limit;
	uint64_t rx_bytes;
	uint64_t tx_bytes;
	uint64_t rx_diff;
	uint64_t tx_diff;
} __packed;

/* hw uses network order for addr */
struct nat_local_addr {
	uint16_t index;
	uint8_t dst_ring;
	__be32 addr;
	union {
		uint8_t dev_addr[ETH_ALEN];
		struct {
			uint32_t dev_addr_l;
			uint16_t dev_addr_h;
		};
	};
} __packed;

/* hw uses network order for port */
struct nat_local_port {
	uint16_t reply_port_dst_l;	/* an index of table */
	union {
		struct {
			uint32_t	enable: 1,
					reply_port_dst_h: 8,
					origin_port_src: 16,
					addr_index: 4,
					dst_ring: 2,
					is_udp: 1;
		};
		uint32_t	hw_val;
	};
} __packed;

struct hw_info {
	uint32_t version;
	uint32_t capabilities;
} __packed;

/* hw capabilities mask
 * refer to dt-bindings/soc/samsung/exynos-dit.h in kernel
 */
enum hw_capabilities_mask {
	DIT_CAP_MASK_NONE		= (0x0),
	DIT_CAP_MASK_PORT_BIG_ENDIAN	= (0x1 << 0),	/* deprecated */
};

#define OFFLOAD_IOC_MAGIC ('D')

#define OFFLOAD_IOCTL_INIT_OFFLOAD		_IO(OFFLOAD_IOC_MAGIC, 0x00)
#define OFFLOAD_IOCTL_STOP_OFFLOAD		_IO(OFFLOAD_IOC_MAGIC, 0x01)
#define OFFLOAD_IOCTL_SET_LOCAL_PRFIX		_IO(OFFLOAD_IOC_MAGIC, 0x02)
#define OFFLOAD_IOCTL_GET_FORWD_STATS		_IOWR(OFFLOAD_IOC_MAGIC, 0x03, struct forward_stats)
#define OFFLOAD_IOCTL_SET_DATA_LIMIT		_IOW(OFFLOAD_IOC_MAGIC, 0x04, struct forward_stats)
#define OFFLOAD_IOCTL_SET_UPSTRM_PARAM		_IOW(OFFLOAD_IOC_MAGIC, 0x05, struct iface_info)
#define OFFLOAD_IOCTL_ADD_DOWNSTREAM		_IOWR(OFFLOAD_IOC_MAGIC, 0x06, struct iface_info)
#define OFFLOAD_IOCTL_REMOVE_DOWNSTRM		_IOW(OFFLOAD_IOC_MAGIC, 0x07, struct iface_info)

#define OFFLOAD_IOCTL_SET_NAT_LOCAL_ADDR	_IOW(OFFLOAD_IOC_MAGIC, 0x20, struct nat_local_addr)
#define OFFLOAD_IOCTL_SET_NAT_LOCAL_PORT	_IOW(OFFLOAD_IOC_MAGIC, 0x21, struct nat_local_port)

/* mandatory */
#define OFFLOAD_IOCTL_GET_HW_INFO		_IOR(OFFLOAD_IOC_MAGIC, 0xE0, struct hw_info)

