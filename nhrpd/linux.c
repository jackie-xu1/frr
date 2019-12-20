/* NHRP daemon Linux specific glue
 * Copyright (c) 2014-2015 Timo Teräs
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include <errno.h>
#include "zebra.h"
#include <linux/if_packet.h>

#include "nhrp_protocol.h"
#include "os.h"

#ifndef HAVE_STRLCPY
size_t strlcpy(char *__restrict dest,
	       const char *__restrict src, size_t destsize);
#endif

int os_socket(struct nhrp_vrf *nhrp_vrf)
{
	if (nhrp_vrf->nhrp_socket_fd < 0) {
		frr_with_privs(&nhrpd_privs) {
			nhrp_vrf->nhrp_socket_fd =
				vrf_socket(PF_PACKET, SOCK_DGRAM,
					   htons(ETH_P_NHRP),
					   nhrp_vrf->vrf_id, NULL);
		}
	}
	return nhrp_vrf->nhrp_socket_fd;
}

int os_sendmsg(const uint8_t *buf, size_t len, int ifindex, const uint8_t *addr,
	       size_t addrlen, uint16_t protocol, int fd)
{
	struct sockaddr_ll lladdr;
	struct iovec iov = {
		.iov_base = (void *)buf, .iov_len = len,
	};
	struct msghdr msg = {
		.msg_name = &lladdr,
		.msg_namelen = sizeof(lladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	int status;

	if (addrlen > sizeof(lladdr.sll_addr))
		return -1;

	memset(&lladdr, 0, sizeof(lladdr));
	lladdr.sll_family = AF_PACKET;
	lladdr.sll_protocol = htons(protocol);
	lladdr.sll_ifindex = ifindex;
	lladdr.sll_halen = addrlen;
	memcpy(lladdr.sll_addr, addr, addrlen);

	if (fd < 0)
		return -1;

	status = sendmsg(fd, &msg, 0);
	if (status < 0)
		return -errno;

	return status;
}

int os_recvmsg(uint8_t *buf, size_t *len, int *ifindex, uint8_t *addr,
	       size_t *addrlen, int fd)
{
	struct sockaddr_ll lladdr;
	struct iovec iov = {
		.iov_base = buf, .iov_len = *len,
	};
	struct msghdr msg = {
		.msg_name = &lladdr,
		.msg_namelen = sizeof(lladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	int r;

	r = recvmsg(fd, &msg, MSG_DONTWAIT);
	if (r < 0)
		return r;

	*len = r;
	*ifindex = lladdr.sll_ifindex;

	if (*addrlen <= (size_t)lladdr.sll_addr) {
		if (memcmp(lladdr.sll_addr, "\x00\x00\x00\x00", 4) != 0) {
			memcpy(addr, lladdr.sll_addr, lladdr.sll_halen);
			*addrlen = lladdr.sll_halen;
		} else {
			*addrlen = 0;
		}
	}

	return 0;
}

static int linux_configure_arp(const char *iface, int on,
			       struct nhrp_vrf *nhrp_vrf)
{
	struct ifreq ifr;

	strlcpy(ifr.ifr_name, iface, IFNAMSIZ);
	if (ioctl(nhrp_vrf->nhrp_socket_fd, SIOCGIFFLAGS, &ifr))
		return -1;

	if (on)
		ifr.ifr_flags &= ~IFF_NOARP;
	else
		ifr.ifr_flags |= IFF_NOARP;

	if (ioctl(nhrp_vrf->nhrp_socket_fd, SIOCSIFFLAGS, &ifr))
		return -1;

	return 0;
}

int os_configure_dmvpn(struct interface *ifp, int af,
		       struct nhrp_vrf *nhrp_vrf)
{
	int ret = 0;

	switch (af) {
	case AF_INET:
		nhrp_send_zebra_interface_redirect(ifp, af);
		break;
	}
	ret |= linux_configure_arp(ifp->name, 1, nhrp_vrf);

	return ret;
}
