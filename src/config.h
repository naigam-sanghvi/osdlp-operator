/*
 * conf.h
 *
 *  Created on: May 19, 2020
 *      Author: vardakis
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
