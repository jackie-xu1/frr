/*
 * Copyright (C) 2019  NetDEF, Inc.
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

#include <lib/version.h>
#include "getopt.h"
#include "thread.h"
#include "command.h"
#include "log.h"
#include "memory.h"
#include "privs.h"
#include "sigevent.h"
#include "libfrr.h"
#include "vrf.h"

#include "pathd.h"
#include "path_nb.h"
#include "path_zebra.h"

char backup_config_file[256];

bool mpls_enabled;

zebra_capabilities_t _caps_p[] = {};

struct zebra_privs_t pathd_privs = {
#if defined(FRR_USER) && defined(FRR_GROUP)
	.user = FRR_USER,
	.group = FRR_GROUP,
#endif
#if defined(VTY_GROUP)
	.vty_group = VTY_GROUP,
#endif
	.caps_p = _caps_p,
	.cap_num_p = array_size(_caps_p),
	.cap_num_i = 0};

struct option longopts[] = {{0}};

/* Master of threads. */
struct thread_master *master;

static struct frr_daemon_info pathd_di;

/* SIGHUP handler. */
static void sighup(void)
{
	zlog_info("SIGHUP received");

	/* Reload config file. */
	vty_read_config(NULL, pathd_di.config_file, config_default);
}

/* SIGINT / SIGTERM handler. */
static void sigint(void)
{
	zlog_notice("Terminating on signal");

	exit(0);
}

/* SIGUSR1 handler. */
static void sigusr1(void)
{
	zlog_rotate();
}

struct quagga_signal_t path_signals[] = {
	{
		.signal = SIGHUP,
		.handler = &sighup,
	},
	{
		.signal = SIGUSR1,
		.handler = &sigusr1,
	},
	{
		.signal = SIGINT,
		.handler = &sigint,
	},
	{
		.signal = SIGTERM,
		.handler = &sigint,
	},
};

static const struct frr_yang_module_info *pathd_yang_modules[] = {
	&frr_interface_info,
	&frr_pathd_info,
};

#define PATH_VTY_PORT 2621

FRR_DAEMON_INFO(pathd, PATH, .vty_port = PATH_VTY_PORT,

		.proghelp = "Implementation of PATH.",

		.signals = path_signals, .n_signals = array_size(path_signals),

		.privs = &pathd_privs, .yang_modules = pathd_yang_modules,
		.n_yang_modules = array_size(pathd_yang_modules), )

int main(int argc, char **argv, char **envp)
{
	frr_preinit(&pathd_di, argc, argv);
	frr_opt_add("", longopts, "");

	while (1) {
		int opt;

		opt = frr_getopt(argc, argv, NULL);

		if (opt == EOF)
			break;

		switch (opt) {
		case 0:
			break;
		default:
			frr_help_exit(1);
			break;
		}
	}

	master = frr_init();

	path_zebra_init(master);
	path_cli_init();

	frr_config_fork();
	frr_run(master);

	/* Not reached. */
	return 0;
}
