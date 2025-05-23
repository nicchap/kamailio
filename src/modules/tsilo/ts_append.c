/*
 * Copyright (C) 2014 Federico Cabiddu (federico.cabiddu@gmail.com)
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../core/sr_module.h"
#include "../../core/dprint.h"
#include "../../core/mod_fix.h"
#include "../../core/route.h"
#include "../../core/data_lump.h"
#include "../../core/dset.h"
#include "../../core/script_cb.h"
#include "../../core/parser/msg_parser.h"
#include "../../core/parser/contact/parse_contact.h"
#include "tsilo.h"
#include "ts_hash.h"
#include "ts_append.h"

int ts_append(struct sip_msg *msg, str *ruri, str *contact, char *table)
{
	ts_urecord_t *_r;
	ts_transaction_t *ptr;

	struct sip_uri p_uri;
	struct sip_uri c_uri;
	str *t_uri;

	int res;
	int appended;

	/* parse R-URI */
	if(use_domain) {
		t_uri = ruri;
	} else {
		if(parse_uri(ruri->s, ruri->len, &p_uri) < 0) {
			LM_ERR("failed to parse uri %.*s\n", ruri->len, ruri->s);
			return -1;
		}
		t_uri = &p_uri.user;
	}

	/* parse contact if given */
	if(contact->s != NULL && contact->len != 0) {
		if(parse_uri(contact->s, contact->len, &c_uri) < 0) {
			LM_ERR("failed to parse contact %.*s\n", contact->len, contact->s);
			return -1;
		}
	}

	/* find urecord in TSILO cache */
	lock_entry_by_ruri(t_uri);

	res = get_ts_urecord(t_uri, &_r);

	if(res != 0) {
		LM_DBG("no record for %.*s\n", t_uri->len, t_uri->s);
		unlock_entry_by_ruri(t_uri);
		return -2;
	}

	/* cycle through existing transactions */
	ptr = _r->transactions;

	while(ptr) {
		LM_DBG("transaction %u:%u found for %.*s, going to append branches\n",
				ptr->tindex, ptr->tlabel, t_uri->len, t_uri->s);

		appended = ts_append_to(
				msg, ptr->tindex, ptr->tlabel, table, ruri, contact);
		if(appended > 0)
			update_stat(added_branches, appended);
		ptr = ptr->next;
	}

	unlock_entry_by_ruri(t_uri);

	return 1;
}

int ts_append_to(struct sip_msg *msg, int tindex, int tlabel, char *table,
		str *uri, str *contact)
{
	struct cell *t = 0;
	struct cell *
			orig_t; /* a pointer to an existing transaction or 0 if lookup fails */
	struct sip_msg *orig_msg;
	int ret;
	str stable;
	int orig_branch;

	if(contact->s != NULL && contact->len > 0) {
		LM_DBG("trying to append based on specific contact <%.*s>\n",
				contact->len, contact->s);
	}

	_tmb.get_tb(&orig_t, &orig_branch);

	/* lookup a transaction based on its identifier (hash_index:label) */
	if(_tmb.t_lookup_ident(&t, tindex, tlabel) < 0) {
		LM_ERR("transaction [%u:%u] not found\n", tindex, tlabel);
		ret = -1;
		goto done;
	}

	/* check if the dialog is still in the early stage */
	if(t->flags & T_CANCELED) {
		LM_DBG("transaction [%u:%u] was cancelled\n", tindex, tlabel);
		ret = -2;
		goto done;
	}

	if(t->uas.status >= 200) {
		LM_DBG("transaction [%u:%u] sent out a final response already - %d\n",
				tindex, tlabel, t->uas.status);
		ret = -3;
		goto done;
	}

	/* get original (very first) request of the transaction */
	orig_msg = t->uas.request;
	stable.s = table;
	stable.len = strlen(stable.s);

	if(uri == NULL || uri->s == NULL || uri->len <= 0) {
		ret = _regapi.lookup_to_dset(orig_msg, &stable, NULL);
	} else {
		ret = _regapi.lookup_to_dset(orig_msg, &stable, uri);
	}

	if(ret != 1) {
		LM_ERR("transaction %u:%u: error updating dset (%d)\n", tindex, tlabel,
				ret);
		ret = -4;
		goto done;
	}

	/* if the contact has been given previously
		then do a new append only for the desired location */
	ret = _tmb.t_append_branches(contact);

done:
	/* unref the transaction which had been referred by t_lookup_ident() call.
	 * Restore the original transaction (if any) */
	if(t)
		_tmb.unref_cell(t);
	_tmb.set_tb(orig_t, orig_branch);

	return ret;
}


/**
 *
 */
int ts_append_branches(sip_msg_t *msg, str *ruri)
{
	ts_transaction_t *ptr = NULL;
	ts_urecord_t *_r;
	tm_cell_t *t = NULL;
	tm_cell_t *orig_t = NULL;
	sip_uri_t p_uri;
	str *t_uri = NULL;
	str contact = STR_NULL;
	int orig_branch;
	int res;

	/* parse R-URI */
	if(use_domain) {
		t_uri = ruri;
	} else {
		if(parse_uri(ruri->s, ruri->len, &p_uri) < 0) {
			LM_ERR("failed to parse uri %.*s\n", ruri->len, ruri->s);
			return -1;
		}
		t_uri = &p_uri.user;
	}

	/* find urecord in TSILO cache */
	lock_entry_by_ruri(t_uri);

	res = get_ts_urecord(t_uri, &_r);

	if(res != 0) {
		LM_DBG("no record for %.*s\n", t_uri->len, t_uri->s);
		unlock_entry_by_ruri(t_uri);
		return -2;
	}

	/* cycle through existing transactions */
	for(ptr = _r->transactions; ptr != NULL; ptr = ptr->next) {
		LM_DBG("transaction %u:%u found for %.*s, going to append branches\n",
				ptr->tindex, ptr->tlabel, t_uri->len, t_uri->s);

		_tmb.get_tb(&orig_t, &orig_branch);

		/* lookup a transaction based on its identifier (hash_index:label) */
		if(_tmb.t_lookup_ident(&t, ptr->tindex, ptr->tlabel) < 0 || t == NULL) {
			LM_ERR("transaction [%u:%u] not found\n", ptr->tindex, ptr->tlabel);
			continue;
		}

		/* check if the dialog is still in the early stage */
		if(t->flags & T_CANCELED) {
			LM_DBG("transaction [%u:%u] was cancelled\n", ptr->tindex,
					ptr->tlabel);
			goto done;
		}

		if(t->uas.status >= 200) {
			LM_DBG("transaction [%u:%u] sent out a final response already - "
				   "%d\n",
					ptr->tindex, ptr->tlabel, t->uas.status);
			goto done;
		}

		/* if the contact has been given previously
			then do a new append only for the desired location */
		res = _tmb.t_append_branches(&contact);

		if(res > 0) {
			update_stat(added_branches, res);
		}

	done:
		/* unref the transaction which had been referred by t_lookup_ident() call.
		 * Restore the original transaction (if any) */
		_tmb.unref_cell(t);
		_tmb.set_tb(orig_t, orig_branch);
	}

	unlock_entry_by_ruri(t_uri);

	return 1;
}
