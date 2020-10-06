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

#ifndef SRC_MISSION_CONFIG_H_
#define SRC_MISSION_CONFIG_H_

#include <string>

#define OUTPUT_PORT  6666
#define INPUT_PORT   6667
#define OUTPUT_ADDR  "127.0.0.1"

#define TC_MAX_SDU_LEN 1024
#define TC_MAX_FRAME_LEN 128
#define TM_MAX_SDU_LEN 1024
#define TM_FRAME_LEN 124
#define MAX_TM_VCS 8

#define __cplusplus 201703L

extern "C" {
#include "../lib/osdlp/include/osdlp.h"
}

struct local_queue_item {
	uint8_t fdu[TC_MAX_FRAME_LEN];
	uint8_t rt_flag;
	uint8_t seq_num;
	tc_bypass_t type;
};

struct mission_params {
	size_t tc_max_sdu_len;
	size_t tc_max_frame_len;
	size_t tm_max_sdu_len;
	size_t tm_frame_len;
	size_t tc_sent_queue_max_cap;
	size_t tc_tx_queue_max_cap;
	size_t tc_rx_queue_max_cap;
	size_t in_port;
	size_t out_port;
	size_t sec_out_port;
	std::string instance;
};


#endif /* SRC_MISSION_CONFIG_H_ */
