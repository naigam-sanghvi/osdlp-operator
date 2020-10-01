/*
 * qubik_farm_inst.cpp
 *
 *  Created on: May 28, 2020
 *      Author: vardakis
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "queue_api.h"
#include "virtualchannel.h"
#include "qubik_farm_inst.h"
extern "C" {
#include "../lib/osdlp/include/osdlp.h"
}

qubik_farm_inst::qubik_farm_inst()
{
}

qubik_farm_inst::~qubik_farm_inst()
{
}


void
qubik_farm_inst::farm_transmitter()
{}

void
qubik_farm_inst::farm_receiver()
{
	struct mission_params m = get_mission_params();
	uint8_t ocf[4];
	struct clcw_frame clcw;
	int sockfd;
	uint8_t rx_buffer[TC_MAX_FRAME_LEN];
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		std::cerr << "Bind failed" << std::endl;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));


	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(m.in_port);

	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = inet_addr(OUTPUT_ADDR);
	cliaddr.sin_port = htons(m.out_port);

	if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr))
	    < 0) {
		std::cerr << "Bind failed" << std::endl;
	}

	int n;
	uint8_t vcid;
	struct tm_transfer_frame *tm;
	struct tc_transfer_frame *tc;
	len = sizeof(servaddr);
	int ret;
	while (1) {
		n = recvfrom(sockfd, (char *) rx_buffer, TC_MAX_FRAME_LEN,
		             MSG_WAITALL, (struct sockaddr *) &servaddr, &len);

		ret = osdlp_tc_receive(rx_buffer, TC_MAX_FRAME_LEN);
		tc = get_last_tc();
		tm = get_vc_tm(tc->primary_hdr.vcid)->get_tm_config();
		osdlp_prepare_clcw(tc, tm->ocf);

		osdlp_tm_transmit_idle_fdu(tm, tc->mission.clcw.vcid);
		sendto(sockfd, tm->mission.util.buffer, TM_FRAME_LEN,
		       MSG_CONFIRM, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
	}
}
