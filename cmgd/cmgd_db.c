/*
 * CMGD Databases
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

#include <pthread.h>

#include "thread.h"
#include "sockunion.h"
#include "prefix.h"
#include "network.h"
#include "lib/libfrr.h"
#include "lib/thread.h"
#include "cmgd/cmgd.h"
#include "cmgd/cmgd_memory.h"
#include "lib/cmgd_pb.h"
#include "lib/vty.h"
#include "cmgd/cmgd_db.h"
#include "libyang/libyang.h"

#ifdef REDIRECT_DEBUG_TO_STDERR
#define CMGD_DB_DBG(fmt, ...)				\
	fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#define CMGD_DB_ERR(fmt, ...)				\
	fprintf(stderr, "%s: ERROR, " fmt "\n", __func__, ##__VA_ARGS__)
#else /* REDIRECT_DEBUG_TO_STDERR */
#define CMGD_DB_DBG(fmt, ...)				\
	zlog_err("%s: " fmt , __func__, ##__VA_ARGS__)
#define CMGD_DB_ERR(fmt, ...)				\
	zlog_err("%s: ERROR: " fmt , __func__, ##__VA_ARGS__)
#endif /* REDIRECT_DEBUG_TO_STDERR */

typedef struct cmgd_db_ctxt_ {
        cmgd_database_id_t db_id;
	pthread_rwlock_t rw_lock;

	bool config_db;

	union {
		struct nb_config *cfg_root;
		struct lyd_node *dnode_root;
	} root;
} cmgd_db_ctxt_t;

const char *cmgd_db_names[CMGD_DB_MAX_ID+1] = {
	CMGD_DB_NAME_NONE, 		/* CMGD_DB_NONE */
	CMGD_DB_NAME_RUNNING, 		/* CMGD_DB_RUNNING */
	CMGD_DB_NAME_CANDIDATE, 	/* CMGD_DB_RUNNING */
	CMGD_DB_NAME_OPERATION, 	/* CMGD_DB_OPERATIONAL */
	"Unknown/Invalid", 		/* CMGD_DB_ID_MAX */
};

static struct cmgd_master *cmgd_db_cm = NULL;
static cmgd_db_ctxt_t running, candidate, oper;

extern struct nb_config *running_config;

int cmgd_db_init(struct cmgd_master *cm)
{
	if (cmgd_db_cm || cm->running_db || cm->candidate_db || cm->oper_db)
		assert(!"Call cmgd_db_init() only once!");

	// Use Running DB from NB module???
	if (!running_config)
		assert(!"Call cmgd_db_init() after frr_init() only!");
	running.root.cfg_root = running_config;
	// running.root.cfg_root = nb_config_new(NULL);
	running.config_db = true;
	running.db_id = CMGD_DB_RUNNING;

	candidate.root.cfg_root = nb_config_new(NULL);
	candidate.config_db = true;
	candidate.db_id = CMGD_DB_CANDIDATE;

	oper.root.dnode_root = yang_dnode_new(ly_native_ctx, true);
	oper.config_db = false;
	oper.db_id = CMGD_DB_OPERATIONAL;

	cm->running_db = (cmgd_db_hndl_t)&running;
	cm->candidate_db = (cmgd_db_hndl_t)&candidate;
	cm->oper_db = (cmgd_db_hndl_t)&oper;
	cmgd_db_cm = cm;

	return 0;
}

cmgd_db_hndl_t cmgd_db_get_hndl_by_id(
        struct cmgd_master *cm, cmgd_database_id_t db_id)
{
	switch (db_id) {
	case CMGD_DB_CANDIDATE:
		return (cm->candidate_db);
	case CMGD_DB_RUNNING:
		return (cm->running_db);
	case CMGD_DB_OPERATIONAL:
		return (cm->oper_db);
	default:
		break;
	}

	return 0;
}

bool cmgd_db_is_config(cmgd_db_hndl_t db_hndl)
{
	cmgd_db_ctxt_t *db_ctxt;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return false;

	return db_ctxt->config_db;
}

int cmgd_db_read_lock(cmgd_db_hndl_t db_hndl)
{
	cmgd_db_ctxt_t *db_ctxt;
	int lock_status;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return -1;

	lock_status = pthread_rwlock_tryrdlock(&db_ctxt->rw_lock);
	return lock_status;

}

int cmgd_db_write_lock(cmgd_db_hndl_t db_hndl)
{
	cmgd_db_ctxt_t *db_ctxt;
	int lock_status;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return -1;

	lock_status = pthread_rwlock_trywrlock(&db_ctxt->rw_lock);
	return lock_status;
}

int cmgd_db_unlock(cmgd_db_hndl_t db_hndl)
{
	cmgd_db_ctxt_t *db_ctxt;
	int lock_status;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return -1;

	lock_status =  pthread_rwlock_unlock(&db_ctxt->rw_lock);
	return lock_status;
}

int cmgd_db_merge_dbs(
        cmgd_db_hndl_t src_db, cmgd_db_hndl_t dst_db)
{
	cmgd_db_ctxt_t *src, *dst;

	src = (cmgd_db_ctxt_t *)src_db;
	dst = (cmgd_db_ctxt_t *)dst_db;
	if (!src || !dst)
		return -1;

	return 0;
}

int cmgd_db_copy_dbs(
        cmgd_db_hndl_t src_db, cmgd_db_hndl_t dst_db)
{
	cmgd_db_ctxt_t *src, *dst;

	src = (cmgd_db_ctxt_t *)src_db;
	dst = (cmgd_db_ctxt_t *)dst_db;
	if (!src || !dst)
		return -1;

	return 0;
}

struct nb_config *cmgd_db_get_nb_config(cmgd_db_hndl_t db_hndl)
{
	cmgd_db_ctxt_t *db_ctxt;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return NULL;

	return (db_ctxt->config_db ? db_ctxt->root.cfg_root : NULL);
}

static int cmgd_walk_db_nodes(cmgd_db_ctxt_t *db_ctxt, 
	char *base_xpath, cmgd_db_node_iter_fn iter_fn,
	void *ctxt, char *xpaths[], struct lyd_node *dnodes[],
	struct nb_node *nbnodes[], int *num_nodes,
	bool childs_as_well)
{
	uint32_t indx;
	struct ly_set *set = NULL;
	char *xpath;
	size_t xp_len; 
	int ret, num_left, num_found;

	if (!num_nodes)
		return -1;

	if (!*num_nodes)
		return 0;

	xp_len = 0;
	cmgd_xpath_append_trail_wildcard(base_xpath, &xp_len);
	ret = lyd_find_xpath(db_ctxt->config_db ?
			db_ctxt->root.cfg_root->dnode :
			db_ctxt->root.dnode_root, base_xpath,
			&set);
	cmgd_xpath_remove_trail_wildcard(base_xpath, &xp_len);

	if (LY_SUCCESS != ret) {
		return -1;
	}

	num_left = *num_nodes;
	CMGD_DB_DBG(" -- START: *num_nodes:%d, num_left:%d",
		*num_nodes, num_left);
	*num_nodes = 0;
	for(indx = 0; indx < set->count; indx++) {
		if (dnodes) {
			dnodes[*num_nodes] = set->dnodes[indx];
			assert(dnodes[*num_nodes]->schema &&
				dnodes[*num_nodes]->schema->priv);
		}
		if (nbnodes)
			nbnodes[*num_nodes] =
				(struct nb_node *) dnodes[indx]->schema->priv;

		xpath = NULL;
		if (xpaths) {
			if (xpaths[*num_nodes]) {
				xpath = lyd_path(dnodes[*num_nodes],
						LYD_PATH_STD,
						xpaths[*num_nodes],
						CMGD_MAX_XPATH_LEN);
			} else {
				xpath = lyd_path(dnodes[*num_nodes],
						LYD_PATH_STD, NULL, 0);
				xpaths[*num_nodes] = xpath;
			}
			CMGD_DB_DBG(" -- XPATH: %s", xpath);
		}

		if (iter_fn) {
			(*iter_fn)((cmgd_db_hndl_t) db_ctxt,
				xpath, dnodes[*num_nodes],
				nbnodes ? nbnodes[*num_nodes] : NULL, 
				ctxt);
		}		

		(*num_nodes)++;
		num_left--;

		if (!childs_as_well)
			continue;

		if (set->count > 1) {
			num_found = num_left;
			if (cmgd_walk_db_nodes(db_ctxt, xpath, iter_fn,
				ctxt, xpaths ? &xpaths[*num_nodes] : NULL,
				&dnodes[*num_nodes],
				nbnodes ? &nbnodes[*num_nodes] : NULL,
				&num_found, childs_as_well) != 0) {
				break;
			}
			num_left -= num_found;
			(*num_nodes) += num_found;
		}
	}

	ly_set_free(set, NULL);

	CMGD_DB_DBG(" -- END: *num_nodes:%d, num_left:%d",
		*num_nodes, num_left);

	return 0;
}

int cmgd_db_lookup_data_nodes(
        cmgd_db_hndl_t db_hndl, const char *xpath, char *dxpaths[],
        struct lyd_node *dnodes[], struct nb_node *nbnodes[],
	int *num_nodes, bool get_childs_as_well)
{
	cmgd_db_ctxt_t *db_ctxt;
	char base_xpath[CMGD_MAX_XPATH_LEN];

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt || !num_nodes)
		return -1;

	if (xpath[0] == '.' && xpath[1] == '/')
		xpath += 2;

	strncpy(base_xpath, xpath, sizeof(base_xpath));

	return (cmgd_walk_db_nodes(db_ctxt, base_xpath, NULL, NULL,
			dxpaths, dnodes, nbnodes, num_nodes,
			get_childs_as_well));
}

int cmgd_db_delete_data_nodes(
        cmgd_db_hndl_t db_hndl, const char *xpath)
{
	cmgd_db_ctxt_t *db_ctxt;
	struct nb_node *nb_node;
	struct lyd_node *dnode, *dep_dnode;
	char dep_xpath[XPATH_MAXLEN];

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return -1;

	nb_node = nb_node_find(xpath);

	dnode = yang_dnode_get(db_ctxt->config_db ?
			db_ctxt->root.cfg_root->dnode :
			db_ctxt->root.dnode_root, xpath);

	if (!dnode)
		/*
			* Return a special error code so the caller can choose
			* whether to ignore it or not.
			*/
		return NB_ERR_NOT_FOUND;
	/* destroy dependant */
	if (nb_node->dep_cbs.get_dependant_xpath) {
		nb_node->dep_cbs.get_dependant_xpath(dnode, dep_xpath);

		dep_dnode = yang_dnode_get(db_ctxt->config_db ?
				db_ctxt->root.cfg_root->dnode :
				db_ctxt->root.dnode_root, dep_xpath);
		if (dep_dnode)
			lyd_free_tree(dep_dnode);
	}
	lyd_free_tree(dnode);

	return 0;
}

int cmgd_db_iter_data(
        cmgd_db_hndl_t db_hndl, char *base_xpath,
        cmgd_db_node_iter_fn iter_fn, void *ctxt)
{
	cmgd_db_ctxt_t *db_ctxt;
	int indx, ret, max_iter;
	char *xpaths[CMGD_MAX_NUM_DBNODES_PER_BATCH] = { 0 };
	struct lyd_node *dnodes[CMGD_MAX_NUM_DBNODES_PER_BATCH] = {0};
	struct nb_node *nbnodes[CMGD_MAX_NUM_DBNODES_PER_BATCH] = {0};

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return -1;

	max_iter = array_size(dnodes);
	ret = cmgd_walk_db_nodes(db_ctxt, base_xpath, iter_fn, ctxt,
			xpaths, dnodes, nbnodes, &max_iter, true);

	for (indx = 0; indx < max_iter; indx++) {
		if (xpaths[indx]) {
			free(xpaths[indx]);
		}
	}

	return ret;
}

int cmgd_db_hndl_send_get_data_req(
        cmgd_db_hndl_t db_hndl, cmgd_database_id_t db_id,
        cmgd_yang_getdata_req_t *data_req, int num_reqs)
{
	cmgd_db_ctxt_t *db_ctxt;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt)
		return -1;

	return 0;
}

void cmgd_db_status_write_one(
        struct vty *vty, cmgd_db_hndl_t db_hndl)
{
	cmgd_db_ctxt_t *db_ctxt;

	db_ctxt = (cmgd_db_ctxt_t *)db_hndl;
	if (!db_ctxt) {
		vty_out(vty, "    >>>>> Database Not Initialized!\n");
		return;
	}
	
	vty_out(vty, "  DB: %s\n", cmgd_db_id2name(db_ctxt->db_id));
	vty_out(vty, "    DB-Hndl: \t\t\t0x%p\n", db_ctxt);
	vty_out(vty, "    Config: \t\t\t%s\n", db_ctxt->config_db ? "True" : "False");
}

void cmgd_db_status_write(struct vty *vty)
{
	cmgd_db_ctxt_t *db_ctxt;

	vty_out(vty, "CMGD Databases\n");

	cmgd_db_status_write_one(vty, cmgd_db_cm->running_db);

	cmgd_db_status_write_one(vty, cmgd_db_cm->candidate_db);

	cmgd_db_status_write_one(vty, cmgd_db_cm->oper_db);
}
