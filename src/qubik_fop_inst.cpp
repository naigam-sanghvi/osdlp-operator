/*
 * fop_inst.cpp
 *
 *  Created on: May 27, 2020
 *      Author: vardakis
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include<arpa/inet.h>
#include "queue_api.h"
#include "virtualchannel.h"
#include "config.h"
#include "qubik_fop_inst.h"
extern "C" {
#include "../lib/osdlp/include/osdlp.h"
#include <ncurses.h>
#include <unistd.h>
}

WINDOW *status;

qubik_fop_inst::qubik_fop_inst()
{
}

qubik_fop_inst::~qubik_fop_inst()
{
}

void
update_status(bool on)
{
	wclear(status);
	if (on) {
		wattron(status, COLOR_PAIR(2));
		mvwprintw(status, 0, 0, "  CONNECTED  ");
	} else {
		wattron(status, COLOR_PAIR(1));
		mvwprintw(status, 0, 0, "NOT_CONNECTED");
	}
	wrefresh(status);
}

void
qubik_fop_inst::fop_transmitter()
{
	link_state_t st = NOT_CONNECTED;
	WINDOW *local_win;

	initscr();
	start_color();
	curs_set(0);
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	int height = LINES;
	int width = COLS / 2 - 7;
	int starty = 0;
	int startx = 0;
	local_win = newwin(height, width, starty, startx);
	status = newwin(1, 14, 0, COLS / 2 - 7);
	wattron(status, COLOR_PAIR(1));
	mvwprintw(status, 0, 0, "NOT_CONNECTED");
	box(local_win, 0, 0);
	wrefresh(local_win);
	wrefresh(status);

	uint8_t tx_buffer[TC_MAX_FRAME_LEN];
	int sockfd;
	struct sockaddr_in servaddr;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(get_mission_params().out_port);
	servaddr.sin_addr.s_addr = inet_addr(OUTPUT_ADDR);

	while (1) {
		virtual_channel::sptr vc = get_vc_tc(1);
		struct tc_transfer_frame *tr = &vc->get_tc_config();
		if (st == NOT_CONNECTED) {
			wclear(local_win);
			mvwaddstr(local_win, 1, 1, "Choose initialization method: \n"
			          " 0. Exit \n"
			          " 1. Initialize with CLCW\n"
			          " 2. Initialize no CLCW\n"
			          " 3. Initialize with Set V(R)\n"
			          " 4. Initialize with Unlock : ");
			box(local_win, 0, 0);
			wrefresh(local_win);
		}
		int input = 0;
		wscanw(local_win, "%d", &input);
		switch (input) {
			case 1:
				osdlp_initiate_with_clcw(tr);
				break;
			case 2:
				osdlp_initiate_no_clcw(tr);
				break;
			case 3:
				osdlp_initiate_with_setvr(tr, 0);
				break;
			case 4:
				osdlp_initiate_with_unlock(tr);
				break;
		}
		uint8_t *pt = &vc->get_tx_queue().front()[0];
		int n, len;
		sendto(sockfd, pt, vc->get_tc_config().primary_hdr.frame_len + 1,
		       MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	}
	endwin();
}

void
qubik_fop_inst::fop_receiver()
{
	initscr();
	struct clcw_frame clcw;
	WINDOW *local_win;
	int height = LINES;
	int width = COLS / 2 - 7;
	int starty = 0;
	int startx = COLS / 2 + 7;
	local_win = newwin(height, width, starty, startx);
	box(local_win, 0, 0);
	wrefresh(local_win);

	int sockfd;
	uint8_t rx_buffer[TM_FRAME_LEN];
	struct sockaddr_in servaddr;
	socklen_t len;

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		std::cerr << "Bind failed" << std::endl;
	}

	memset(&servaddr, 0, sizeof(servaddr));


	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(get_mission_params().in_port);

	if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr))
	    < 0) {
		std::cerr << "Bind failed" << std::endl;
	}

	int n;
	len = sizeof(servaddr);
	uint8_t ocf[4];
	while (1) {
		n = recvfrom(sockfd, (char *) rx_buffer, TM_FRAME_LEN,
		             MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
		if (n > 0) {
			volatile int ret = osdlp_tm_receive(rx_buffer);
			struct tm_transfer_frame *tm = get_last_tm();
			memcpy(ocf, tm->ocf, 4 * sizeof(uint8_t));

//			ocf[0] = tm->ocf[0];
//			ocf[1] = tm->ocf[1];
//			ocf[2] = tm->ocf[2];
//			ocf[3] = tm->ocf[3];
			osdlp_clcw_unpack(&clcw, ocf);
			wprintw(local_win, " Control Word Type: %d\n"
			        " CLCW Version Number : %d\n"
			        " Status field: %d \n"
			        " COP in Effect: %d\n"
			        " VCID: %d\n"
			        " RSVD Spare: %d\n"
			        " No RF Available FLAG: %d\n"
			        " No Bit Lock FLAG: %d\n"
			        " Lockout FLAG: %d\n"
			        " Wait FLAG: %d\n"
			        " Retransmit FLAG: %d\n"
			        " Farm-B Counter: %d\n"
			        " RSVD Spare: %d\n"
			        " Report Value: %d\n", clcw.ctrl_word_type,
			        clcw.clcw_version_num, clcw.status_field,
			        clcw.cop_in_effect, clcw.vcid, clcw.rsvd_spare1,
			        clcw.rf_avail, clcw.bit_lock, clcw.lockout, clcw.wait,
			        clcw.rt, clcw.farm_b_counter, clcw.rsvd_spare2,
			        clcw.report_value);
			box(local_win, 0, 0);

			wrefresh(local_win);
			update_status(1);

			virtual_channel::sptr vc = get_vc_tc(clcw.vcid);
			struct tc_transfer_frame tr = vc->get_tc_config();

			osdlp_handle_clcw(&tr, ocf);
		}
	}
}

int
timer_start(uint16_t vcid)
{
	return 0;
}

int
timer_cancel(uint16_t vcid)
{
	return 0;
}

