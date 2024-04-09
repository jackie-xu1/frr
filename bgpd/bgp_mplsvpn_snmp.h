// SPDX-License-Identifier: GPL-2.0-or-later
/* MPLS/BGP L3VPN MIB
 * Copyright (C) 2020 Volta Networks Inc
 */

void bgp_mpls_l3vpn_module_init(void);

#define MPLSL3VPNVRFRTECIDRTYPEOTHER 1
#define MPLSL3VPNVRFRTECIDRTYPEREJECT 2
#define MPLSL3VPNVRFRTECIDRTYPELOCAL 3
#define MPLSL3VPNVRFRTECIDRTYPEREMOTE 4
#define MPLSL3VPNVRFRTECIDRTYPEBLACKHOLE 5

#define MPLSVPNVRFRTTYPEIMPORT 1
#define MPLSVPNVRFRTTYPEEXPORT 2
#define MPLSVPNVRFRTTYPEBOTH 3
