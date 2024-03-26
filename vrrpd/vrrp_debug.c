// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * VRRP debugging.
 * Copyright (C) 2019 Cumulus Networks, Inc.
 * Quentin Young
 */
#include <zebra.h>

#include "lib/command.h"
#include "lib/debug.h"
#include "lib/vector.h"

#include "vrrp_debug.h"

/* clang-format off */
struct debug vrrp_dbg_arp = {0, "debug vrrp arp", "VRRP ARP"};
struct debug vrrp_dbg_auto = {0, "debug vrrp autoconfigure", "VRRP autoconfiguration events"};
struct debug vrrp_dbg_ndisc = {0, "debug vrrp ndisc", "VRRP Neighbor Discovery"};
struct debug vrrp_dbg_pkt = {0, "debug vrrp packets", "VRRP packets"};
struct debug vrrp_dbg_proto = {0, "debug vrrp protocol", "VRRP protocol events"};
struct debug vrrp_dbg_sock = {0, "debug vrrp sockets", "VRRP sockets"};
struct debug vrrp_dbg_zebra = {0, "debug vrrp zebra", "VRRP Zebra events"};

struct debug *vrrp_debugs[] = {
	&vrrp_dbg_arp,
	&vrrp_dbg_auto,
	&vrrp_dbg_ndisc,
	&vrrp_dbg_pkt,
	&vrrp_dbg_proto,
	&vrrp_dbg_sock,
	&vrrp_dbg_zebra
};

const char *vrrp_debugs_conflines[] = {
	"debug vrrp arp",
	"debug vrrp autoconfigure",
	"debug vrrp ndisc",
	"debug vrrp packets",
	"debug vrrp protocol",
	"debug vrrp sockets",
	"debug vrrp zebra",
};
/* clang-format on */

static int vrrp_debug_config_write_helper(struct vty *vty, bool config)
{
	uint32_t mode = DEBUG_MODE_ALL;

	if (config)
		mode = DEBUG_MODE_CONF;

	for (unsigned int i = 0; i < array_size(vrrp_debugs); i++)
		if (DEBUG_MODE_CHECK(vrrp_debugs[i], mode))
			vty_out(vty, "%s\n", vrrp_debugs_conflines[i]);

	return 0;
}

int vrrp_debug_status_write(struct vty *vty)
{
	return vrrp_debug_config_write_helper(vty, false);
}

void vrrp_debug_set(struct interface *ifp, uint8_t vrid, int vtynode,
		    bool onoff, bool proto, bool autoconf, bool pkt, bool sock,
		    bool ndisc, bool arp, bool zebra)
{
	uint32_t mode = DEBUG_NODE2MODE(vtynode);

	if (proto)
		DEBUG_MODE_SET(&vrrp_dbg_proto, mode, onoff);
	if (autoconf)
		DEBUG_MODE_SET(&vrrp_dbg_auto, mode, onoff);
	if (pkt)
		DEBUG_MODE_SET(&vrrp_dbg_pkt, mode, onoff);
	if (sock)
		DEBUG_MODE_SET(&vrrp_dbg_sock, mode, onoff);
	if (ndisc)
		DEBUG_MODE_SET(&vrrp_dbg_ndisc, mode, onoff);
	if (arp)
		DEBUG_MODE_SET(&vrrp_dbg_arp, mode, onoff);
	if (zebra)
		DEBUG_MODE_SET(&vrrp_dbg_zebra, mode, onoff);
}

/* ------------------------------------------------------------------------- */

void vrrp_debug_init(void)
{
	debug_install(&vrrp_dbg_arp);
	debug_install(&vrrp_dbg_auto);
	debug_install(&vrrp_dbg_ndisc);
	debug_install(&vrrp_dbg_pkt);
	debug_install(&vrrp_dbg_proto);
	debug_install(&vrrp_dbg_sock);
	debug_install(&vrrp_dbg_zebra);
}
