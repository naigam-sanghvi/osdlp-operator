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
#include <unistd.h>
}

std::vector<unsigned long int> thread_handle;

uint8_t tx_buf[28];

void
forward_frames(int sockfd, struct sockaddr_in servaddr)
{
	virtual_channel::sptr vc = get_vc_tc(1);
	struct tc_transfer_frame *tr = &vc->get_tc_config();
	uint8_t *pt;
	int n, len;
	while (1) {
		if (vc->get_tx_queue().size() > 0) {
			pt = &vc->get_tx_queue().front()[0];
			memcpy(tx_buf, pt, vc->get_tc_config().primary_hdr.frame_len + 1);
			sendto(sockfd, tx_buf, 28,
			       MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
			vc->get_tx_queue().pop_front();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

qubik_fop_inst::qubik_fop_inst()
{
}

qubik_fop_inst::~qubik_fop_inst()
{
}

void
qubik_fop_inst::fop_transmitter()
{
	link_state_t st = NOT_CONNECTED;

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

	std::thread tim(timer);
	tim.detach();

	std::thread t(forward_frames, sockfd, servaddr);
	thread_handle.push_back(t.native_handle());
	t.detach();
	int input;

	for (size_t i = 0; i < 28; i++)
		tx_buf[i] = rand() % 256;
	while (1) {
		virtual_channel::sptr vc = get_vc_tc(1);
		struct tc_transfer_frame *tr = &vc->get_tc_config();
		input = 0;
		std::cin >> input;

		switch (input) {
			case 0:
				pthread_cancel(thread_handle.front());
				exit(0);
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
	}
}

void
qubik_fop_inst::fop_receiver()
{
	struct clcw_frame clcw;

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

			ocf[0] = tm->ocf[0];
			ocf[1] = tm->ocf[1];
			ocf[2] = tm->ocf[2];
			ocf[3] = tm->ocf[3];
			osdlp_clcw_unpack(&clcw, ocf);

			virtual_channel::sptr vc = get_vc_tc(clcw.vcid);
			struct tc_transfer_frame tr = vc->get_tc_config();

			osdlp_handle_clcw(&tr, ocf);
		}
	}
}








