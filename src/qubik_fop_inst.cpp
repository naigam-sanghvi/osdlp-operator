/*
 * fop_inst.cpp
 *
 *  Created on: May 27, 2020
 *      Author: vardakis
 */

#include <cstdio>
#include <fstream>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <iosfwd>
#include <ctype.h>
#include <sstream>
#include <iomanip>

#include "queue_api.h"
#include "virtualchannel.h"
#include "config.h"
#include "qubik_fop_inst.h"

extern "C" {
#include "../lib/osdlp/include/osdlp.h"
#include <unistd.h>
}

std::vector<unsigned long int> thread_handle;
logger::sptr log_udp;
uint8_t tx_buf[TC_MAX_FRAME_LEN];
int active_vcid = -1;

qubik_fop_inst::link_state_t st = qubik_fop_inst::NOT_CONNECTED;

void
qubik_fop_inst::forward_frames(int sockfd, struct sockaddr_in servaddr)
{
	virtual_channel::sptr vc = get_vc_tc(VCID);
	uint8_t *pt;
	int n, len;
	while (1) {
		for (virtual_channel::sptr vc : *get_tc_configs()) {
			while (vc->get_tx_queue()->size() > 0) {
				memset(tx_buf, 0x33, TC_MAX_FRAME_LEN);
				if (!get_lock()->try_lock_for(std::chrono::milliseconds(2000))) {
					continue;
				}
				pt = &vc->get_tx_queue()->front()[0];
				memcpy(tx_buf, pt,
				       vc->get_tc_config()->primary_hdr.frame_len + 1);
				vc->get_tx_queue()->pop_front();
				get_lock()->unlock();
				log_udp->log_output(
				        "VCID: " + std::to_string(vc->get_vcid())
				        + " TX: Frame sent \n", true);
				sendto(sockfd, tx_buf, TC_MAX_FRAME_LEN,
				       MSG_CONFIRM, (const struct sockaddr *) &servaddr,
				       sizeof(servaddr));
			}

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

logger::sptr
get_logger()
{
	return log_udp;
}

void
qubik_fop_inst::fop_transmitter()
{
	log_udp = logger::make_shared(get_mission_params().sec_out_port);
	int sockfd;
	struct sockaddr_in servaddr;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(get_mission_params().out_port);
	servaddr.sin_addr.s_addr = inet_addr(get_mission_params().host_ip.c_str());

	for (virtual_channel::sptr vc : *get_tc_configs()) {
		std::thread tim(timer, vc->get_vcid());
		tim.detach();
	}

	std::thread t(forward_frames, sockfd, servaddr);
	thread_handle.push_back(t.native_handle());
	t.detach();

	std::string in;
	int input;
	int v_in;
	std::string hex_data;
	std::vector<uint8_t> cmd;
	bool map_found;

	for (size_t i = 0; i < 28; i++)
		tx_buf[i] = rand() % 256;

	struct virtual_channel::map m;
	virtual_channel::sptr vc;
	struct tc_transfer_frame *tr;
	while (1) {
		if (active_vcid == -1) {
			std::cout << "Insert VCID " << std::endl;
			for (virtual_channel::sptr vc : *get_tc_configs()) {
				std::cout << "* " << std::to_string(vc->get_vcid()) << " "
				          << vc->get_tc_name() << std::endl;
			}
			std::cin >> input;
			std::cout << std::endl;
			vc = get_vc_tc(input);
			if (vc == NULL) {
				std::cout << "Invalid VCID " << std::endl;
				continue;
			} else {
				std::cout << "VCID " << std::to_string(input) << " selected"
				          << std::endl;
			}
		}
		tr = vc->get_tc_config();
		active_vcid = input;
		input = 0;
		switch (tr->cop_cfg.fop.state) {
			case FOP_STATE_INIT:
				std::cout
				                << "Insert initialization option : \n"
				                "0. Return to VCID selection menu. \n"
				                "1. Initiate with CLCW (Just wait until a packet arrives) \n"
				                "2. Initiate no CLCW (Don't expect anything. We begin. \n"
				                "3. Initiate with Set V(R) (Sends a command. Good for ping) \n"
				                "4. Initiate with Unlock. (Another command. Also good for ping) \n "
				                << std::endl;
				std::cin >> input;
				std::cout << std::endl;
				if (input == 3) {
					std::cout << "Insert new V(R) : ";
					std::cin >> v_in;
					if (v_in < 0 || v_in > 255) {
						std::cout << "Invalid V(R) \n";
						continue;
					}
				}
				if (!get_lock()->try_lock_for(std::chrono::milliseconds(2000))) {
					continue;
				}
				switch (input) {
					case 0:
						active_vcid = -1;
						break;
					case 1:
						osdlp_initiate_with_clcw(tr);
						break;
					case 2:
						osdlp_initiate_no_clcw(tr);
						break;
					case 3:
						osdlp_initiate_with_setvr(tr, v_in);
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
					std::cout << " *   0. Return to VCID selection menu."
					          << std::endl;
					for (struct virtual_channel::map m : *vc->get_maps()) {
						std::cout << " *   " << std::to_string(m.mapid) << " "
						          << m.name << std::endl;
					}
					std::cout
					                << " *  's' [osdlp] Set new V(S) (Local frame sequence number)"
					                << std::endl;
					std::cout
					                << " *  't' [osdlp] Terminate local osdlp service (Reset)"
					                << std::endl;
					std::cout << " *  'r' [osdlp] Resume" << std::endl;
					std::cin >> in;
					if (!isdigit(in[0])) {
						if (in[0] == 's') {
							std::cout << "Insert new V(S) : ";
							std::cin >> input;
							input = std::atoi(in.data());
							if (input < 0 || input > 255) {
								std::cout << "Invalid V(S) \n";
								break;
							}
						}
						if (!get_lock()->try_lock_for(
						            std::chrono::milliseconds(2000))) {
							continue;
						}
						switch (in[0]) {
							case 's':
								osdlp_set_new_vs(tr, input);
								break;
							case 't':
								osdlp_terminate_ad(tr);
								break;
							case 'r':
								osdlp_resume_ad(tr);
								break;
						}
						get_lock()->unlock();
						continue;
					}
					input = std::atoi(in.data());
					if (input == 0) {
						active_vcid = -1;
						break;
					}
					for (virtual_channel::map vm : *vc->get_maps()) {
						if (vm.mapid == input) {
							m = vm;
							map_found = true;
							break;
						}
					}
					if (map_found == false) {
						std::cout << "Wrong MAP id";
						continue;
					}
					map_found = false;
					std::cout << std::endl;
					if (m.data.size() > 0) {
						if (!get_lock()->try_lock_for(
						            std::chrono::milliseconds(2000))) {
							continue;
						}
						if (m.bypass == 0) {
							osdlp_prepare_typea_data_frame(tr, m.data.data(),
							                               m.data.size(), m.mapid);
						} else if (m.bypass == 1) {
							osdlp_prepare_typeb_data_frame(tr, m.data.data(),
							                               m.data.size(), m.mapid);
						}
						osdlp_tc_transmit(tr, m.data.data(), m.data.size());
						get_lock()->unlock();
						std::cout << "Command sent" << std::endl;
					} else {
						std::cout
						                << "Insert command in hex format x01x02 etc or 0 for return"
						                << std::endl;
						std::cin >> hex_data;
						cmd.clear();
						if (hex_data == "0") {
							active_vcid = -1;
							break;
						}
						int ret = tokenize(hex_data, &cmd);
						if (ret) {
							std::cout << "Invalid data. Returning to main menu..."
							          << std::endl;
							active_vcid = -1;
							break;
						}
						if (!get_lock()->try_lock_for(
						            std::chrono::milliseconds(2000))) {
							continue;
						}
						if (m.bypass == 0) {
							osdlp_prepare_typea_data_frame(tr, cmd.data(),
							                               cmd.size(), m.mapid);
						} else if (m.bypass == 1) {
							osdlp_prepare_typeb_data_frame(tr, cmd.data(),
							                               cmd.size(), m.mapid);
						}
						osdlp_tc_transmit(tr, cmd.data(), cmd.size());
						get_lock()->unlock();
					}

				} else {
					std::cout
					                << "Add some MAPs to transmit TCs. Exiting gracefully "
					                << std::endl;
					active_vcid = -1;
				}
				break;
			case FOP_STATE_INIT_BC:
			case FOP_STATE_INIT_NO_BC:
			case FOP_STATE_RT_NO_WAIT:
			case FOP_STATE_RT_WAIT:
				if (tr->cop_cfg.fop.state == FOP_STATE_INIT_BC
				    || tr->cop_cfg.fop.state == FOP_STATE_INIT_NO_BC) {
					std::cout << "Waiting for response to arrive... " << std::endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				} else if (tr->cop_cfg.fop.state == FOP_STATE_RT_NO_WAIT) {
					std::cout << "Retransmitting... Returning to main menu "
					          << std::endl;
				} else if (tr->cop_cfg.fop.state == FOP_STATE_RT_WAIT) {
					std::cout
					                << "Retransmitting... Spacecraft sent WAIT. Returning to main menu "
					                << std::endl;
				}
				std::cout
				                << " *  's' [osdlp] Set new V(S) (Local frame sequence number)"
				                << std::endl;
				std::cout << " *  't' [osdlp] Terminate local osdlp service (Reset)"
				          << std::endl;
				std::cout << " *  'r' [osdlp] Resume" << std::endl;
				std::cout << " *  '0' [osdlp] Wait. Continue to main menu" << std::endl;
				std::cin >> in;
				if (in[0] == 's') {
					std::cout << "Insert new V(S) : ";
					std::cin >> input;
					input = std::atoi(in.data());
					if (input < 0 || input > 255) {
						std::cout << "Invalid V(S) \n";
						break;
					}
				}
				if (!get_lock()->try_lock_for(
				            std::chrono::milliseconds(2000))) {
					continue;
				}
				switch (in[0]) {
					case 's':
						osdlp_set_new_vs(tr, input);
						break;
					case 't':
						osdlp_terminate_ad(tr);
						break;
					case 'r':
						osdlp_resume_ad(tr);
						break;
					case '0':
						active_vcid = -1;
						break;
				}
				get_lock()->unlock();
				continue;
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
	std::string clcw_out;
	std::stringstream hexStringStream;
	uint8_t rx_payload[1024];
	uint16_t rx_length;
	while (1) {
		n = recvfrom(sockfd, (char *) rx_buffer, TM_FRAME_LEN,
		             MSG_WAITALL, (struct sockaddr *) &servaddr, &len);

		if (n > 0) {
			log_udp->log_output("RX: Received frame.\n", true);
			int ret = osdlp_tm_receive(rx_buffer);
			if (ret < 0) {
				log_udp->log_output(
				        "RX: OSDLP Error, Code :" + std::to_string(ret)
				        + " \n", true);
				continue;
			}
			struct tm_transfer_frame *tm = get_last_tm();
			if (ret == 3) {
				log_udp->log_output("RX: Only Idle Data \n", true);
			} else {
				if (!osdlp_tm_rx_queue_empty(tm->primary_hdr.vcid)) {
					osdlp_tm_rx_queue_dequeue(rx_payload, &rx_length, tm->primary_hdr.vcid);
					for (int i = 0; i < rx_length; i++) {
						hexStringStream << "0x" << std::hex << std::setw(2) << std::setfill('0')
						                << (uint32_t(rx_payload[i]) & 0xFF) << " ";
					}
				}
				log_udp->log_output("RX: VCID = " + std::to_string(tm->primary_hdr.vcid) +
				                    "\n Payload : " + hexStringStream.str() + "\n", true);
				hexStringStream.str(std::string());
			}
			if (!tm->primary_hdr.ocf == TM_OCF_PRESENT) {
				continue;
			}

			memcpy(ocf, tm->ocf, 4 * sizeof(uint8_t));

			ocf[0] = tm->ocf[0];
			ocf[1] = tm->ocf[1];
			ocf[2] = tm->ocf[2];
			ocf[3] = tm->ocf[3];
			osdlp_clcw_unpack(&clcw, ocf);
			virtual_channel::sptr vc = get_vc_tc(clcw.vcid);
			if (vc == NULL) {
				log_udp->log_output("RX: Wrong VCID\n", true);
				continue;
			}
			struct tc_transfer_frame *tr = vc->get_tc_config();
			while (!get_lock()->try_lock_for(std::chrono::milliseconds(2000))) {
				continue;
			}
			osdlp_handle_clcw(tr, ocf);
			get_lock()->unlock();
			clcw_out = "Control Word Type: "
			           + std::to_string(clcw.ctrl_word_type)
			           + " \nCLCW Version Number : "
			           + std::to_string(clcw.clcw_version_num)
			           + " \nStatus field: " + std::to_string(clcw.status_field)
			           + " \nCOP in Effect: " + std::to_string(clcw.cop_in_effect)
			           + " \nVCID: " + std::to_string(clcw.vcid)
			           + " \nRSVD Spare: " + std::to_string(clcw.rsvd_spare1)
			           + " \nNo RF Available FLAG:" + std::to_string(clcw.rf_avail)
			           + " \nNo Bit Lock FLAG: " + std::to_string(clcw.bit_lock)
			           + " \nLockout FLAG: " + std::to_string(clcw.lockout)
			           + " \nWait FLAG: " + std::to_string(clcw.wait)
			           + " \nRetransmit FLAG: " + std::to_string(clcw.rt)
			           + " \nFarm-B Counter: "
			           + std::to_string(clcw.farm_b_counter) + " \nRSVD Spare: "
			           + std::to_string(clcw.rsvd_spare2) + " \nReport Value: "
			           + std::to_string(clcw.report_value) + "\n";
			log_udp->log_output(clcw_out, true);
			log_udp->log_output("\n\n", false);

		}
	}
}

