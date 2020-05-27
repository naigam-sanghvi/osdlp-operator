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

struct tm_transfer_frame *
get_last_tm();

struct tc_transfer_frame *
get_last_tc();

int
init_structs(std::string path);

virtual_channel::sptr
get_vc_tc(uint16_t id);

virtual_channel::sptr
get_vc_tm(uint16_t id);

#endif /* INCLUDE_QUEUE_API_H_ */
