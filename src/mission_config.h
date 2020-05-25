/*
 * mission_config.h
 *
 *  Created on: May 23, 2020
 *      Author: sleepwalker
 */

#ifndef SRC_MISSION_CONFIG_H_
#define SRC_MISSION_CONFIG_H_


#define OUTPUT_PORT  6666
#define INPUT_PORT   6667
#define OUTPUT_ADDR  "127.0.0.1"

#define TC_MAX_SDU_LEN 1024
#define TC_MAX_FRAME_LEN 128
#define TM_MAX_SDU_LEN 1024
#define TM_FRAME_LEN 128
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
};


#endif /* SRC_MISSION_CONFIG_H_ */
