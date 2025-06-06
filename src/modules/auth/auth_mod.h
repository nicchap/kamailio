/*
 * Digest Authentication Module
 *
 * Copyright (C) 2001-2003 FhG Fokus
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

#ifndef AUTH_MOD_H
#define AUTH_MOD_H

#include "../../core/str.h"
#include "../../modules/sl/sl.h"
#include "../../core/parser/msg_parser.h" /* struct sip_msg */
#include "../../core/parser/digest/digest.h"
#include "nonce.h" /* auth_extra_checks & AUTH_CHECK flags */
#include "rfc2617.h"

/*
 * Module parameters variables
 */
extern str secret1;			 /* secret phrase used to generate nonce */
extern str secret2;			 /* secret phrase used to generate nonce */
extern int nonce_expire;	 /* nonce expire interval */
extern int protect_contacts; /* Enable/disable contact hashing in nonce */
extern avp_ident_t challenge_avpid;
extern str proxy_challenge_header;
extern str www_challenge_header;
extern struct qp auth_qop;
extern str auth_algorithm;
extern int
		add_authinfo_hdr; /* should an Authentication-Info header be added on 200 OK responses? */

extern int hash_hex_len;
extern calc_HA1_t calc_HA1;
extern calc_response_t calc_response;

#endif /* AUTH_MOD_H */
