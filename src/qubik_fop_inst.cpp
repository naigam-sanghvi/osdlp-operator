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

qubik_fop_inst::link_state_t st = qubik_fop_inst::NOT_CONNECTED;



void
forward_frames(int sockfd, struct sockaddr_in servaddr)
{
	virtual_channel::sptr vc = get_vc_tc(VCID);
	struct tc_transfer_frame *tr = vc->get_tc_config();
	uint8_t *pt;
	int n, len;
	while (1) {
		if (!get_lock()->try_lock_for(std::chrono::milliseconds(500))) {
			continue;
		}
		if (vc->get_tx_queue()->size() > 0) {
			pt = &vc->get_tx_queue()->front()[0];
			memcpy(tx_buf, pt, vc->get_tc_config()->primary_hdr.frame_len + 1);
			std::cout << "TX: Frame sent " << std::endl;
			sendto(sockfd, tx_buf, 28,
			       MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
			vc->get_tx_queue()->pop_front();
		}
		get_lock()->unlock();
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
	struct tc_transfer_frame *tr;
	struct virtual_channel::map m;

	for (size_t i = 0; i < 28; i++)
		tx_buf[i] = rand() % 256;
	while (1) {
		virtual_channel::sptr vc = get_vc_tc(VCID);
		tr = vc->get_tc_config();

		switch (tr->cop_cfg.fop.state) {
			case FOP_STATE_INIT:
				input = 0;
				std::cout << "Insert initialization option : \n"
				          "1. Initiate with CLCW (Just wait until a packet arrives) \n"
				          "2. Initiate no CLCW (Don't expect anything. We begin. \n"
				          "3. Initiate with Set V(R) (Sends a command. Good for ping) \n"
				          "4. Initiate with Unlock. (Another command. Also good for ping) \n " <<
				          std::endl;
				std::cin >> input;
				std::cout << std::endl;
				if (!get_lock()->try_lock_for(std::chrono::milliseconds(500))) {
					continue;
				}
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
				get_lock()->unlock();
				break;
			case FOP_STATE_ACTIVE:
				if (vc->get_maps()->size() > 0) {
					std::cout << "Choose MAP " << std::endl;
					for (struct virtual_channel::map m : *vc->get_maps()) {
						std::cout << " *   " << std::to_string(m.mapid) << std::endl;
					}
					std::cin >> input;
					if (input < 0 || input > vc->get_maps()->size()) {
						std::cout << "Wrong MAP id";
					}
					std::cout << std::endl;
					m = (*vc->get_maps())[input - 1];
					osdlp_prepare_typea_data_frame(tr, m.data.data(), m.data.size());
					osdlp_tc_transmit(tr, m.data.data(), m.data.size());
				} else {
					std::cout << "Add some MAPs to transmit TCs. Exiting gracefully " << std::endl;
					pthread_cancel(thread_handle.front());
					exit(0);
				}
				break;
			case FOP_STATE_INIT_BC:
			case FOP_STATE_INIT_NO_BC:
				std::cout << "Waiting for response to arrive... " << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
			std::cout << "RX: Received frame . Checking CLCW ... " << std::endl;
			volatile int ret = osdlp_tm_receive(rx_buffer);
			if (ret < 0) {
				std::cout << "RX: OSDLP Error, Code : " << ret << std::endl;
				continue;
			}
			if (ret == 3) {
				std::cout << "RX: Only Idle Data " << std::endl;
			}
			struct tm_transfer_frame *tm = get_last_tm();
			memcpy(ocf, tm->ocf, 4 * sizeof(uint8_t));

			ocf[0] = tm->ocf[0];
			ocf[1] = tm->ocf[1];
			ocf[2] = tm->ocf[2];
			ocf[3] = tm->ocf[3];
			osdlp_clcw_unpack(&clcw, ocf);
			while (!get_lock()->try_lock_for(std::chrono::milliseconds(500))) {
				continue;
			}
			virtual_channel::sptr vc = get_vc_tc(clcw.vcid);
			struct tc_transfer_frame *tr = vc->get_tc_config();

			osdlp_handle_clcw(tr, ocf);
			std::cout << " Control Word Type: " << (int)clcw.ctrl_word_type <<
			          " \nCLCW Version Number : " << (int)clcw.clcw_version_num <<
			          " \nStatus field: " << (int)clcw.status_field <<
			          " \nCOP in Effect: " << (int)clcw.cop_in_effect <<
			          " \nVCID: " << (int)clcw.vcid <<
			          " \nRSVD Spare: " << (int)clcw.rsvd_spare1 <<
			          " \nNo RF Available FLAG:" << (int)clcw.rf_avail <<
			          " \nNo Bit Lock FLAG: " << (int)clcw.bit_lock <<
			          " \nLockout FLAG: " << (int)clcw.lockout <<
			          " \nWait FLAG: " << (int)clcw.wait <<
			          " \nRetransmit FLAG: " << (int)clcw.rt <<
			          " \nFarm-B Counter: " << (int)clcw.farm_b_counter <<
			          " \nRSVD Spare: " << (int)clcw.rsvd_spare2 <<
			          " \nReport Value: " << (int)clcw.report_value << std::endl;
			get_lock()->unlock();
		}
	}
}








