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

#ifndef SRC_VIRTUALCHANNEL_H_
#define SRC_VIRTUALCHANNEL_H_

extern "C" {
#include "../lib/osdlp/include/osdlp.h"
}
#include <memory>
#include <vector>
#include <deque>

#include "mission_config.h"

class virtual_channel
{
public:
	typedef std::shared_ptr<virtual_channel> sptr;

	struct map {
		uint16_t mapid;
		tc_bypass_t bypass;
		tc_ctrl_t ctrl;
		std::vector<uint8_t> data;
	};

	static sptr
	make_shared(struct tm_transfer_frame tm_transfer_frame,
	            struct tc_transfer_frame tc_transfer_frame,
	            uint16_t vcid);

	static sptr
	make_shared(struct tm_transfer_frame tm_transfer_frame,
	            uint16_t vcid);

	static sptr
	make_shared(struct tc_transfer_frame tc_transfer_frame,
	            uint16_t vcid);


	//virtual_channel() = delete;
	//virtual_channel(const virtual_channel &) = delete;
	~virtual_channel();

	uint16_t
	get_vcid();

	void
	add_map(uint16_t mapid, tc_bypass_t bp, tc_ctrl_t ctrl,
	        std::vector<uint8_t> data);

	struct tm_transfer_frame *
	get_tm_config();

	struct tc_transfer_frame *
	get_tc_config();

	std::vector<struct map> *
	get_maps();

	std::deque<struct local_queue_item> *
	get_sent_queue();

	std::deque<std::vector<uint8_t>> *
	                              get_rx_queue();

	std::deque<std::vector<uint8_t>> *
	                              get_tx_queue();

	std::vector<struct tc_transfer_frame> *
	get_wait_queue();

protected:
	virtual_channel(struct tm_transfer_frame tm_transfer_frame,
	                struct tc_transfer_frame tc_transfer_frame,
	                uint16_t vcid);

	virtual_channel(struct tm_transfer_frame tm_transfer_frame,
	                uint16_t vcid);

	virtual_channel(struct tc_transfer_frame tc_transfer_frame,
	                uint16_t vcid);
private:
	struct tm_transfer_frame d_tm_config;
	struct tc_transfer_frame d_tc_config;
	uint16_t d_vcid;
	std::vector<struct map>d_mapids;
	std::deque<struct local_queue_item> d_sent_queue;
	std::deque<std::vector<uint8_t>> d_rx_queue;
	std::deque<std::vector<uint8_t>> d_tx_queue;
	std::vector<struct tc_transfer_frame> d_wait_queue;
};

#endif /* SRC_VIRTUALCHANNEL_H_ */
