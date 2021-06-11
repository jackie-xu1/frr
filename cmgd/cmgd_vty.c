/*
 * CMGD VTY Interface
 * Copyright (C) 2021  Vmware, Inc.
 *		       Pushpasis Sarkar
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <zebra.h>

#include "command.h"
#include "lib/json.h"
#include "lib_errors.h"
#include "lib/libfrr.h"
#include "lib/zclient.h"
#include "prefix.h"
#include "plist.h"
#include "buffer.h"
#include "linklist.h"
#include "stream.h"
#include "thread.h"
#include "log.h"
#include "memory.h"
#include "lib_vty.h"
#include "hash.h"
#include "queue.h"
#include "filter.h"
#include "frrstr.h"

#define INCLUDE_CMGD_CMD_DEFINITIONS

#include "cmgd/cmgd_vty.h"

#include "staticd/static_vty.c"

void cmgd_enqueue_nb_commands(struct vty *vty, const char *xpath,
				enum nb_operation operation,
				const char *value)
{
	zlog_err("%s, cmd: '%s', xpath: '%s' ", __func__, vty->buf, xpath);

}

int cmgd_apply_nb_commands(struct vty *vty, const char *xpath_base_fmt,
				...)
{
	zlog_err("%s, cmd: '%s'", __func__, vty->buf);
	return 0;
}

int cmgd_hndl_bknd_cmd(const struct cmd_element *cmd, struct vty *vty,
			int argc, struct cmd_token *argv[])
{
	vty_out(vty, "%s: %s, got the command '%s'", 
		frr_get_progname(), __func__, vty->buf);
	return 0;
}

void cmgd_vty_init(void)
{
	// cmd_variable_handler_register(bgp_var_neighbor);
	// cmd_variable_handler_register(bgp_var_peergroup);

	// cmgd_init_bcknd_cmd();
	static_vty_init();
}
