/*
 *  Open Space Data Link Protocol
 *
 *  Copyright (C) 2020 Libre Space Foundation (https://libre.space)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "queue_api.h"
#include "config.h"

#include <deque>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <iostream>

std::vector<virtual_channel::sptr> vc_tm_configs;
std::vector<virtual_channel::sptr> vc_tc_configs;
struct mission_params m_params;
uint8_t mc_count;


std::vector<uint8_t>
pkt(TC_MAX_FRAME_LEN);

std::deque<unsigned long int> thread_vec;

std::map<uint16_t, bool> timer_start;
std::map<uint16_t, bool> timer_running;
std::map<uint16_t, bool> cancel_timer;

uint8_t tc_util[TC_MAX_SDU_LEN];
uint8_t tm_util[TM_MAX_SDU_LEN];

struct tm_transfer_frame *last_tm;
struct tc_transfer_frame *last_tc;

std::timed_mutex mtx;

std::unique_lock<std::timed_mutex>
lock(mtx, std::defer_lock);

std::unique_lock<std::timed_mutex> *
get_lock()
{
	return &lock;
}

struct tm_transfer_frame *
get_last_tm()
{
	return last_tm;
}

struct tc_transfer_frame *
get_last_tc()
{
	return last_tc;
}

struct mission_params get_mission_params()
{
	return m_params;
}

std::vector<virtual_channel::sptr> *
get_tc_configs()
{
	return &vc_tc_configs;
}

std::vector<virtual_channel::sptr> *
get_tm_configs()
{
	return &vc_tm_configs;
}

int
init_structs(std::string path)
{
	load_config(path, &vc_tm_configs, &vc_tc_configs, &m_params, &mc_count,
	            tc_util, tm_util);
	return 0;
}

virtual_channel::sptr
get_vc_tm(uint16_t id)
{
	for (virtual_channel::sptr vc : vc_tm_configs) {
		if (vc->get_vcid() == id) {
			return vc;
		}
	}
	return NULL;
}

virtual_channel::sptr
get_vc_tc(uint16_t id)
{
	for (virtual_channel::sptr vc : vc_tc_configs) {
		if (vc->get_vcid() == id) {
			return vc;
		}
	}
	return NULL;
}

int
osdlp_tm_rx_queue_dequeue(uint8_t *buffer, uint16_t *length, uint16_t vcid)
{
	virtual_channel::sptr vc = get_vc_tm(vcid);
	if (!vc)
		return -1;
	uint16_t len = (vc->get_rx_queue()->front()[0] << 8) |
	               vc->get_rx_queue()->front()[1];
	*length = len;

	memcpy(buffer, &vc->get_rx_queue()->front().data()[2], len * sizeof(uint8_t));
	vc->get_rx_queue()->pop_front();
	return 0;
}

bool
osdlp_tm_rx_queue_empty(uint16_t vcid)
{
	virtual_channel::sptr vc = get_vc_tm(vcid);
	if (!vc)
		return false;
	if (vc->get_rx_queue()->size() == 0) {
		return true;
	} else {
		return false;
	}
}

void
timer(uint16_t vcid)
{
	std::chrono::steady_clock::time_point begin, end;
	virtual_channel::sptr vc;
	struct tc_transfer_frame *tr;
	while (1) {
		if (timer_running[vcid]) {
			if (timer_start[vcid]) {
				begin = std::chrono::steady_clock::now();
				timer_start[vcid] = false;
			}
			if (cancel_timer[vcid]) {
				timer_running[vcid] = false;
				cancel_timer[vcid] = false;
			} else {
				vc = get_vc_tc(vcid);
				tr = vc->get_tc_config();
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				end = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::seconds>(
				            end - begin).count() >= tr->cop_cfg.fop.t1_init) {
					if (!lock.try_lock_for(std::chrono::milliseconds(2000))) {
						continue;
					}
					timer_running[vcid] = false;
					osdlp_handle_timer_expired(tr);
					lock.unlock();
					if (tr->cop_cfg.fop.signal >= 5
					    && tr->cop_cfg.fop.signal <= 12) {
						std::cout << "Alert! Probably retrnamission limit reached. Alert code : " <<
						          std::to_string(tr->cop_cfg.fop.signal) << std::endl;
					}
				}

			}
		} else if (timer_start[vcid]) {
			begin = std::chrono::steady_clock::now();
			timer_running[vcid] = true;
			timer_start[vcid] = false;
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
}

extern "C" {

	int osdlp_tc_wait_queue_enqueue(void *tc_tf, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		vc->get_wait_queue()->push_back(*vc->get_tc_config());
		return 0;
	}

	int osdlp_tc_wait_queue_dequeue(void *tc_tf, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		memcpy((struct tc_transfer_frame *) tc_tf, &vc->get_wait_queue()->back(),
		       sizeof(struct tc_transfer_frame));
		vc->get_wait_queue()->pop_back();
		return 0;
	}

	bool osdlp_tc_wait_queue_empty(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return false;
		if (vc->get_wait_queue()->size() == 0) {
			return true;
		} else {
			return false;
		}
	}

	int osdlp_tc_wait_queue_clear(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		vc->get_wait_queue()->clear();
		return 0;
	}

	int osdlp_tc_sent_queue_clear(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		vc->get_sent_queue()->clear();

		return 0;
	}

	uint16_t osdlp_tc_sent_queue_size(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		return vc->get_sent_queue()->size();
	}

	int osdlp_tc_sent_queue_dequeue(struct queue_item *qi, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		struct local_queue_item new_item;
		vc->get_sent_queue()->pop_front();
		return 0;
	}

	int osdlp_tc_sent_queue_enqueue(struct queue_item *qi, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		uint16_t frame_len = (((qi->fdu[2] & 0x03) << 8) | qi->fdu[3]);
		struct local_queue_item new_item;
		memcpy(new_item.fdu, qi->fdu, (frame_len + 1) * sizeof(uint8_t));
		new_item.rt_flag = qi->rt_flag;
		new_item.seq_num = qi->seq_num;
		new_item.type = qi->type;
		vc->get_sent_queue()->push_back(new_item);
		return 0;
	}

	bool osdlp_tc_sent_queue_empty(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return false;
		if (vc->get_sent_queue()->size() == 0) {
			return true;
		} else {
			return false;
		}
	}

	bool osdlp_tc_sent_queue_full(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return false;
		if (vc->get_sent_queue()->size() == m_params.tc_sent_queue_max_cap) {
			return true;
		} else {
			return false;
		}
	}

	int osdlp_tc_sent_queue_head(struct queue_item *qi, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		struct local_queue_item item;
		if (vc->get_sent_queue()->size() > 0) {
			item = vc->get_sent_queue()->front();
			qi->fdu = item.fdu; // TODO Check if this stands
			qi->rt_flag = item.rt_flag;
			qi->seq_num = item.seq_num;
			qi->type = item.type;
			return 0;
		} else {
			return -1;
		}
	}

	struct tc_transfer_frame *
	osdlp_tc_get_tx_config(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return NULL;

		return vc->get_tc_config();
	}

	bool osdlp_tc_rx_queue_full(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return false;
		if (vc->get_rx_queue()->size() == m_params.tc_rx_queue_max_cap) {
			return true;
		} else {
			return false;
		}
	}

	bool osdlp_tc_tx_queue_full()
	{
		return false;
//	if(vc->get_tx_queue().size() == TC_TX_QUEUE_MAX_CAP){
//		return true;
//	}
//	else{
//		return false;
//	}
	}

	int osdlp_tc_tx_queue_enqueue(uint8_t *buffer, uint16_t vcid)
	{
		pkt.clear();

		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		volatile uint16_t length = (((buffer[2] & 0x03) << 8) | buffer[3]) + 1;

		for (int i = 0; i < length; i++) {
			pkt.push_back(buffer[i]);
		}

		int ret = 0;
		if (vc->get_tx_queue()->size() < m_params.tc_tx_queue_max_cap) {
			vc->get_tx_queue()->push_back(pkt);
			ret = 0;
		} else {
			ret = -1;
		}
		return ret;
	}

	int osdlp_tc_rx_queue_enqueue(uint8_t *buffer, uint32_t length, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		if (vc->get_rx_queue()->size() < m_params.tc_rx_queue_max_cap) {
			std::vector<uint8_t> pkt;
			pkt.insert(pkt.end(), buffer, &buffer[length]);
			vc->get_rx_queue()->push_back(pkt);
			return 0;
		} else {
			return -1;
		}
	}

	int osdlp_tc_rx_queue_enqueue_now(uint8_t *buffer, uint32_t length,
	                                  uint8_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		if (vc->get_rx_queue()->size() < m_params.tc_rx_queue_max_cap) {
			std::vector<uint8_t> pkt;
			pkt.insert(pkt.end(), buffer, &buffer[length]);
			vc->get_rx_queue()->push_back(pkt);
			return 0;
		} else {
			vc->get_rx_queue()->pop_back();
			std::vector<uint8_t> pkt;
			pkt.insert(pkt.end(), buffer, &buffer[length]);
			vc->get_rx_queue()->push_back(pkt);
			return 0;
		}
	}

	int osdlp_tc_get_rx_config(struct tc_transfer_frame **tf, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		*tf = vc->get_tc_config();
		last_tc = vc->get_tc_config();
		return 0;
	}

	int osdlp_cancel_lower_ops()
	{
		for (virtual_channel::sptr vc : vc_tc_configs) {
			vc->get_tx_queue()->clear();
		}
		return 0;
	}

	int osdlp_mark_ad_as_rt(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		for (int i = 0; i < vc->get_sent_queue()->size(); i++) {
			if ((*vc->get_sent_queue())[i].type == TYPE_A) {
				(*vc->get_sent_queue())[i].rt_flag = RT_FLAG_ON;
			}
		}
		return 0;
	}

	int osdlp_get_first_ad_rt_frame(struct queue_item *qi, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		for (int i = 0; i < vc->get_sent_queue()->size(); i++) {
			if ((*vc->get_sent_queue())[i].type == TYPE_A
			    && (*vc->get_sent_queue())[i].rt_flag == RT_FLAG_ON) {
				qi->fdu = (*vc->get_sent_queue())[i].fdu;
				qi->rt_flag = (*vc->get_sent_queue())[i].rt_flag;
				qi->seq_num = (*vc->get_sent_queue())[i].seq_num;
				qi->type = (*vc->get_sent_queue())[i].type;
				return 0;
			}
		}
		return -1;
	}

	int osdlp_reset_rt_frame(struct queue_item *qi, uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		for (int i = 0; i < vc->get_sent_queue()->size(); i++) {
			if ((*vc->get_sent_queue())[i].seq_num == qi->seq_num) {
				(*vc->get_sent_queue())[i].rt_flag = RT_FLAG_OFF;
				return 0;
			}
		}
		return -1;
	}

	int osdlp_mark_bc_as_rt(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		if (vc->get_sent_queue()->front().type == TYPE_B) {
			vc->get_sent_queue()->front().rt_flag = RT_FLAG_ON;
		}
		return 0;
	}

	int osdlp_tc_rx_queue_clear(uint16_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tc(vcid);
		if (!vc)
			return -1;
		vc->get_rx_queue()->clear();
		return 0;
	}

	bool osdlp_tm_tx_queue_empty(uint8_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tm(vcid);
		if (!vc)
			return -1;
		if (vc->get_tx_queue()->size() == 0) {
			return true;
		} else {
			return false;
		}
	}

	int osdlp_tm_tx_queue_back(uint8_t **pkt, uint8_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tm(vcid);
		if (!vc)
			return -1;
		if (vc->get_tx_queue()->size() == 0) {
			return -1;
		}
		*pkt = vc->get_tx_queue()->back().data();
		return 0;
	}

	int osdlp_tm_tx_queue_enqueue(uint8_t *pkt, uint8_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tm(vcid);
		if (!vc)
			return -1;
		std::vector<uint8_t> vec;
		uint16_t len = pkt[0] << 8 | pkt[1];
		vec.insert(vec.end(), pkt, &pkt[len + 2]);
		vc->get_tx_queue()->push_back(vec);
		return 0;
	}

	int osdlp_tm_rx_queue_enqueue(uint8_t *pkt, uint8_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tm(vcid);
		if (!vc)
			return -1;
		std::vector<uint8_t> vec;
		uint16_t len = pkt[0] << 8 | pkt[1];
		vec.insert(vec.end(), pkt, &pkt[len + 2]);
		vc->get_rx_queue()->push_back(vec);
		return 0;
	}

	int osdlp_tm_get_packet_len(uint16_t *length, uint8_t *pkt, uint16_t mem_len)
	{
		if (mem_len >= 5) {
			if (((pkt[0] << 8) | pkt[1]) <= m_params.tm_max_sdu_len) {
				*length = ((pkt[0] << 8) | pkt[1]) + sizeof(uint16_t); // Add the length bytes
				return 0;
			} else {
				return -1;
			}
		} else {
			return -1;
		}
	}

	void osdlp_tm_tx_commit_back(uint8_t vcid)
	{
		return;
	}

	int osdlp_tm_get_rx_config(struct tm_transfer_frame **tm, uint8_t vcid)
	{
		virtual_channel::sptr vc = get_vc_tm(vcid);
		if (!vc)
			return -1;
		*tm = vc->get_tm_config();
		last_tm = vc->get_tm_config();
		return 0;
	}

	int osdlp_timer_start(uint16_t vcid)
	{
		timer_start[vcid] = true;
		return 0;
	}

	int osdlp_timer_cancel(uint16_t vcid)
	{
		cancel_timer[vcid] = true;
		return 0;
	}
}
