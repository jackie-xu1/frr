/*
 * Zebra dataplane plugin for DPDK based hw offload
 *
 * Copyright (C) 2021 Nvidia
 * Anuradha Karuppiah
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _ZEBRA_DPLANE_DPDK_PRIVATE_H
#define _ZEBRA_DPLANE_DPDK_PRIVATE_H

#include <zebra.h>

#include "zebra_dplane_dpdk.h"


struct zd_dpdk_stat {
	_Atomic uint32_t ignored_updates;

	_Atomic uint32_t rule_adds;
	_Atomic uint32_t rule_dels;
};

struct zd_dpdk_ctx {
	/* Stats */
	struct zd_dpdk_stat stats;
	int dpdk_logtype;
};

#endif
