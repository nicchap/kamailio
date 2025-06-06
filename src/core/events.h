/**
 *
 * Copyright (C) 2009 SIP-Router.org
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SR_EVENTS_H_
#define _SR_EVENTS_H_

#include "parser/msg_parser.h"

#define SREV_NET_DATA_IN 1
#define SREV_NET_DATA_OUT 2
#define SREV_CORE_STATS 3
#define SREV_CFG_RUN_ACTION 4
#define SREV_PKG_UPDATE_STATS 5
#define SREV_RCV_NOSIP 6
#define SREV_NET_DGRAM_IN 7
#define SREV_TCP_HTTP_100C 8
#define SREV_TCP_MSRP_FRAME 9
#define SREV_TCP_WS_FRAME_IN 10
#define SREV_TCP_WS_FRAME_OUT 11
#define SREV_STUN_IN 12
#define SREV_TCP_CLOSED 13
#define SREV_NET_DATA_RECV 14
#define SREV_NET_DATA_SENT 15
#define SREV_SIP_REPLY_OUT 16
#define SREV_TCP_WS_CLOSE 17

#define SREV_CB_LIST_SIZE 8

typedef struct sr_event_param
{
	void *data;
	str obuf;
	receive_info_t *rcv;
	dest_info_t *dst;
	sip_msg_t *req;
	sip_msg_t *rpl;
	int rplcode;
	int mode;
} sr_event_param_t;

typedef int (*sr_event_cb_f)(sr_event_param_t *evp);

typedef struct sr_event_cb
{
	sr_event_cb_f net_data_in[SREV_CB_LIST_SIZE];
	sr_event_cb_f net_data_out[SREV_CB_LIST_SIZE];
	sr_event_cb_f core_stats;
	sr_event_cb_f run_action;
	sr_event_cb_f pkg_update_stats;
	sr_event_cb_f net_dgram_in;
	sr_event_cb_f tcp_http_100c;
	sr_event_cb_f tcp_msrp_frame;
	sr_event_cb_f tcp_ws_frame_in;
	sr_event_cb_f tcp_ws_frame_out;
	sr_event_cb_f stun_in;
	sr_event_cb_f rcv_nosip;
	sr_event_cb_f tcp_closed[SREV_CB_LIST_SIZE];
	sr_event_cb_f net_data_recv;
	sr_event_cb_f net_data_sent;
	sr_event_cb_f sip_reply_out[SREV_CB_LIST_SIZE];
	sr_event_cb_f tcp_ws_close[SREV_CB_LIST_SIZE];
} sr_event_cb_t;

void sr_event_cb_init(void);
int sr_event_register_cb(int type, sr_event_cb_f f);
int sr_event_exec(int type, sr_event_param_t *evp);
int sr_event_enabled(int type);


/* shortcut types for core event routes */
/* initial parsing error in message receive function */
#define SR_CORE_ERT_RECEIVE_PARSE_ERROR 1

void sr_core_ert_init(void);
void sr_core_ert_run(sip_msg_t *msg, int e);
int sr_core_ert_run_xname(char *evname);

typedef void (*sr_corecb_void_f)(void);
typedef struct sr_corecb
{
	sr_corecb_void_f app_ready;
	sr_corecb_void_f app_shutdown;
} sr_corecb_t;

sr_corecb_t *sr_corecb_get(void);

#define sr_corecb_void_exec(fname)            \
	do {                                      \
		sr_corecb_t *__cbp = sr_corecb_get(); \
		if(__cbp && __cbp->fname) {           \
			__cbp->fname();                   \
		}                                     \
	} while(0);

#endif
