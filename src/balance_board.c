/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *	This file:
 *		Bertho Stultiens	< para >
 *
 *	Copyright 2009
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *	@brief Balance Board expansion device.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef WIN32
	#include <Winsock2.h>
#endif

#include "definitions.h"
#include "wiiuse_internal.h"
#include "dynamics.h"
#include "events.h"
#include "balance_board.h"

/**
 *	@brief Handle the handshake data from the guitar.
 *
 *	@param cc		A pointer to a classic_ctrl_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */
int balance_board_handshake(struct wiimote_t* wm, struct balance_board_t* bb, byte* data, unsigned short len) {
	int i;

	bb->tr = 0.0;
	bb->br = 0.0;
	bb->tl = 0.0;
	bb->br = 0.0;

	if (len < 0xe0)
		return 0;

	if (data[0xdc] != 0xa4) {
		/* decrypt data */
		for (i = 0; i < len; ++i)
			data[i] = (data[i] ^ 0x17) + 0x17;
	}

	/* See: http://wiibrew.org/wiki/Wii_Balance_Board */
	/* Unknown data at 0xa40020..23 (mine says: 0x44 0x69 0x00 0x00) */
	bb->cal_0.tr = (data[0x04] << 8) + data[0x05];
	bb->cal_0.br = (data[0x06] << 8) + data[0x07];
	bb->cal_0.tl = (data[0x08] << 8) + data[0x09];
	bb->cal_0.bl = (data[0x0a] << 8) + data[0x0b];
	bb->cal_17.tr = (data[0x0c] << 8) + data[0x0d];
	bb->cal_17.br = (data[0x0e] << 8) + data[0x0f];
	bb->cal_17.tl = (data[0x10] << 8) + data[0x11];
	bb->cal_17.bl = (data[0x12] << 8) + data[0x13];
	bb->cal_34.tr = (data[0x14] << 8) + data[0x15];
	bb->cal_34.br = (data[0x16] << 8) + data[0x17];
	bb->cal_34.tl = (data[0x18] << 8) + data[0x19];
	bb->cal_34.bl = (data[0x1a] << 8) + data[0x1b];
	/* Unknown data at 0xa4003c..3f (mine says: 0x4c 0x81 0x59 0x95) */

	WIIUSE_DEBUG("calibration  0: %04x %04x %04x %04x\n", bb->cal_0.tr, bb->cal_0.br, bb->cal_0.tl, bb->cal_0.br);
	WIIUSE_DEBUG("calibration 17: %04x %04x %04x %04x\n", bb->cal_17.tr, bb->cal_17.br, bb->cal_17.tl, bb->cal_17.br);
	WIIUSE_DEBUG("calibration 34: %04x %04x %04x %04x\n", bb->cal_34.tr, bb->cal_34.br, bb->cal_34.tl, bb->cal_34.br);

	/* handshake done */
	wm->exp.type = EXP_BALANCE_BOARD;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	return 1;
}


/**
 *	@brief The balance board disconnected.
 *
 *	@param cc		A pointer to a balance_board_t structure.
 */
void balance_board_disconnected(struct balance_board_t* bb) {
	memset(bb, 0, sizeof(*bb));
}



/**
 *	@brief Handle balance board event.
 *
 *	@param cc		A pointer to a balance board_t structure.
 *	@param msg		The message specified in the event packet.
 */
#define set_bbval(x)	do {\
		if(bb->raw.x < bb->cal_17.x) \
			bb->x = 17.0 * (float)(bb->raw.x - bb->cal_0.x) / (float)(bb->cal_17.x - bb->cal_0.x); \
		else \
			bb->x = 17.0 * (1.0 + (float)(bb->raw.x - bb->cal_17.x) / (float)(bb->cal_34.x - bb->cal_17.x)); \
	} while(0)
void balance_board_event(struct balance_board_t* bb, byte* msg) {
	bb->raw.tr = (msg[0] << 8) + msg[1];
	bb->raw.br = (msg[2] << 8) + msg[3];
	bb->raw.tl = (msg[4] << 8) + msg[5];
	bb->raw.bl = (msg[6] << 8) + msg[7];
	set_bbval(tr);
	set_bbval(br);
	set_bbval(tl);
	set_bbval(bl);
}

