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
extern "C" {
#include "../lib/osdlp/include/osdlp.h"
}
virtual_channel::sptr
get_vc(uint16_t vcid)
{
	for (virtual_channel::sptr vc : get_tc_configs()) {
		if (vc->get_vcid() == vcid) {
			return vc;
		}
	}
	return NULL;
}


void
transmitter()
{
	printf("Hello TX\n");
	uint8_t tx_buffer[TC_MAX_FRAME_LEN];
	int sockfd;
	const char *hello = "Hello from client\n";
	struct sockaddr_in servaddr;

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(OUTPUT_PORT);
	servaddr.sin_addr.s_addr = inet_addr(OUTPUT_ADDR);

	virtual_channel::sptr vc = get_vc(1);
	struct tc_transfer_frame *tr =  &vc->get_tc_config();
	initiate_with_setvr(&vc->get_tc_config(), 0);
	uint8_t *pt = &vc->get_tx_queue().front()[0];
	int n, len;
	sendto(sockfd, pt, vc->get_tc_config().primary_hdr.frame_len + 1,
	       MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
}

void
receiver()
{
	printf("Hello RX\n");
	int sockfd;
	uint8_t rx_buffer[TM_FRAME_LEN];
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		std::cerr << "Bind failed" << std::endl;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(INPUT_PORT);

	// Bind the socket with the server address
	if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr))
	    < 0) {
		std::cerr << "Bind failed" << std::endl;
	}

	int n;

	len = sizeof(cliaddr);

	n = recvfrom(sockfd, (char *) rx_buffer, TM_FRAME_LEN,
	             MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);

	int ret = tm_receive(rx_buffer);
	struct tm_transfer_frame tm = get_last_tm();
	if (tm.primary_hdr.ocf == TM_OCF_PRESENT) {
		std::cout << tm.ocf << std::endl;
	}
}

int
timer_start(uint16_t vcid)
{
	return 0;
}

int
main(int argc, char *argv[])
{
	std::string path(argv[1]);
	init_structs(path);

	std::thread th1(transmitter);
	std::thread th2(receiver);
	th1.join();
	th2.join();
}
