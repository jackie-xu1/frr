/* mgmt memory type declarations
 * Copyright (C) 2021  Vmware, Inc.
 *		       Pushpasis Sarkar <spushpasis@vmware.com>
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

#ifndef _FRR_MGMTD_MEMORY_H
#define _FRR_MGMTD_MEMORY_H

#include "memory.h"

DECLARE_MGROUP(MGMTD);
DECLARE_MTYPE(MGMTD);
DECLARE_MTYPE(MGMTD_BE_ADPATER);
DECLARE_MTYPE(MGMTD_FE_ADPATER);
DECLARE_MTYPE(MGMTD_FE_SESSION);
DECLARE_MTYPE(MGMTD_TXN);
DECLARE_MTYPE(MGMTD_TXN_REQ);
DECLARE_MTYPE(MGMTD_TXN_SETCFG_REQ);
DECLARE_MTYPE(MGMTD_TXN_COMMCFG_REQ);
DECLARE_MTYPE(MGMTD_TXN_GETDATA_REQ);
DECLARE_MTYPE(MGMTD_TXN_GETDATA_REPLY);
DECLARE_MTYPE(MGMTD_TXN_CFG_BATCH);
DECLARE_MTYPE(MGMTD_TXN_DATA_BATCH);
DECLARE_MTYPE(MGMTD_BE_ADAPTER_MSG_BUF);
DECLARE_MTYPE(MGMTD_CMT_INFO);
DECLARE_MTYPE(MGMTD_XPATH_MAP);
#endif /* _FRR_MGMTD_MEMORY_H */
