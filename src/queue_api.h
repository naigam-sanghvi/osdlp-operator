/*
 * queue_api.h
 *
 *  Created on: May 19, 2020
 *      Author: vardakis
 */

#ifndef INCLUDE_QUEUE_API_H_
#define INCLUDE_QUEUE_API_H_

#include <vector>
#include <string>

#include <stddef.h>
#include <stdint.h>
#include "mission_config.h"
#include "virtualchannel.h"

extern "C" {
#include "../lib/osdlp/include/osdlp.h"
}

std::vector<virtual_channel::sptr> &
get_tc_configs();

std::vector<virtual_channel::sptr> &
get_tm_configs();

struct mission_params
get_mission_params();

struct tm_transfer_frame
get_last_tm();

int
init_structs(std::string path);

#endif /* INCLUDE_QUEUE_API_H_ */
