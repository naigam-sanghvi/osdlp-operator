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

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#include <libconfig.h++>

#include <vector>
#include "virtualchannel.h"
#include "mission_config.h"

int
load_config(const std::string path,
            std::vector<virtual_channel::sptr> *vc_tm_configs,
            std::vector<virtual_channel::sptr> *vc_tc_configs,
            struct mission_params *m_params,
            uint8_t *mc_count,
            uint8_t *tc_util,
            uint8_t *tm_util);

#endif /* SRC_CONFIG_H_ */
