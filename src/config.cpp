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

#include "config.h"
//#include "queue_api.h"
#include <filesystem>
#include <iostream>

int
load_config(const std::string path,
            std::vector<virtual_channel::sptr> *vc_tm_configs,
            std::vector<virtual_channel::sptr> *vc_tc_configs,
            struct mission_params *m_params, uint8_t *mc_count, uint8_t *tc_util,
            uint8_t *tm_util)
{

	std::string instance;
	int scid;
	if (!std::filesystem::exists(path)) {
		throw std::invalid_argument("Configuration file path does not exist");
		return -1;
	}
	libconfig::Config d_cfg;
	try {
		d_cfg.readFile(path.data());
	} catch (const libconfig::FileIOException &fioex) {
		throw std::runtime_error("I/O error while reading file.");
		return -1;
	} catch (const libconfig::ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
		          << " - " << pex.getError() << std::endl;
		return -1;
	}
	try {
		const libconfig::Setting &root = d_cfg.getRoot();

		if (!(root.exists("instance"))) {
			throw std::runtime_error("Please specify which instance of "
			                         "COP to initialize (\"fop\" or \"farm\")");
		}

		root.lookupValue("instance", instance);

		if (instance.compare("farm") != 0 && instance.compare("fop") != 0) {
			throw std::runtime_error("Wrong instance\n");
		}
		m_params->instance = instance;

		if (!(root.exists("mission"))) {
			throw std::runtime_error("Mission params missing");
		}

		const libconfig::Setting &miss = root["mission"];
		int ret;

		if (miss.exists("in_port")) {
			miss.lookupValue("in_port", ret);
			m_params->in_port = (size_t) ret;
		}
		if (miss.exists("out_port")) {
			miss.lookupValue("out_port", ret);
			m_params->out_port = (size_t) ret;
		}
		if (miss.exists("stdout_port")) {
			miss.lookupValue("stdout_port", ret);
			m_params->sec_out_port = (size_t) ret;
		}

		if (miss.exists("tc_max_sdu_len")) {
			miss.lookupValue("tc_max_sdu_len", ret);
			m_params->tc_max_sdu_len = (size_t) ret;
		} else {
#ifdef TC_MAX_SDU_LEN
			m_params->tc_max_sdu_len = TC_MAX_SDU_LEN;
#else
			throw std::runtime_error("Mission params configuration incomplete - "
			                         "tc_max_sdu_len missing");
#endif
		}

		if (miss.exists("tc_max_frame_len")) {
			miss.lookupValue("tc_max_frame_len", ret);
			m_params->tc_max_frame_len = (size_t) ret;
		} else {
#ifdef TC_MAX_FRAME_LEN
			m_params->tc_max_frame_len = TC_MAX_FRAME_LEN;
#else
			throw std::runtime_error("Mission params configuration incomplete - "
			                         "tc_max_frame_len missing");
#endif
		}

		if (miss.exists("tm_max_sdu_len")) {
			miss.lookupValue("tm_max_sdu_len", ret);
			m_params->tm_max_sdu_len = (size_t) ret;
		} else {
#ifdef TM_MAX_SDU_LEN
			m_params->tm_max_sdu_len = TM_MAX_SDU_LEN;
#else
			throw std::runtime_error("Mission params configuration incomplete - "
			                         "tm_max_sdu_len missing");
#endif
		}

		if (miss.exists("tm_frame_len")) {
			miss.lookupValue("tm_frame_len", ret);
			m_params->tm_frame_len = (size_t) ret;
		} else {
#ifdef TM_FRAME_LEN
			m_params->tm_frame_len = TM_FRAME_LEN;
#else
			throw std::runtime_error("Mission params configuration incomplete - "
			                         "tm_frame_len missing");
#endif
		}
		if (miss.exists("tc_sent_queue_max_cap")) {
			miss.lookupValue("tc_sent_queue_max_cap", ret);
			m_params->tc_sent_queue_max_cap = (size_t) ret;
		} else {
			throw std::runtime_error(
			        "Mission params configuration incomplete - "
			        "tc_sent_queue_max_cap missing");
		}

		if (miss.exists("tc_tx_queue_max_cap")) {
			miss.lookupValue("tc_tx_queue_max_cap", ret);
			m_params->tc_tx_queue_max_cap = (size_t) ret;
		} else {
			throw std::runtime_error(
			        "Mission params configuration incomplete - "
			        "tc_tx_queue_max_cap missing");
		}

		if (miss.exists("tc_rx_queue_max_cap")) {
			miss.lookupValue("tc_rx_queue_max_cap", ret);
			m_params->tc_rx_queue_max_cap = (size_t) ret;
		} else {
			throw std::runtime_error(
			        "Mission params configuration incomplete - "
			        "tc_rx_queue_max_cap missing");
		}

		if (miss.exists("scid")) {
			miss.lookupValue("scid", scid);
		} else {
			throw std::runtime_error(
			        "Mission params configuration incomplete - "
			        "spacecraft ID missing");
		}

		if (!(root.exists("tm") && root.exists("tc"))) {
			throw std::runtime_error("Configuration incomplete");
		}

		/* ------------------------ TM -----------------------------------*/
		int tm_vcid, crc, ocf, ocf_type, sec_hdr_on, sync_flag, stuff_state;

		const libconfig::Setting &tm_vcs = root["tm"];
		size_t num_tms = tm_vcs.getLength();

		for (int i = 0; i < num_tms; i++) {
			libconfig::Setting &tm_t = tm_vcs[i];
			if (!(tm_t.exists("vcid"))) {
				std::cerr << "VC ID missing. Ignorring VC..." << std::endl;
				continue;
			}
			struct tm_transfer_frame tm_f;

			tm_t.lookupValue("vcid", tm_vcid);
			if (!(tm_vcid >= 0 && tm_vcid <= 7)) {
				std::cerr << "Invalid VC ID. Ignorring VC..." << std::endl;
				continue;
			}
			if (tm_t.exists("crc")) {
				tm_t.lookupValue("crc", crc);
				if (!(crc == 1 || crc == 0)) {
					crc = (tm_crc_flag_t) 1;
				}
			} else {
				crc = (tm_crc_flag_t) 1;
			}
			if (tm_t.exists("ocf_flag")) {
				tm_t.lookupValue("ocf_flag", ocf);
				if (!(ocf == 1 || ocf == 0)) {
					ocf = (tm_ocf_flag_t) 0;
				}
			} else {
				ocf = (tm_ocf_flag_t) 0;
			}
			if (tm_t.exists("ocf_type")) {
				tm_t.lookupValue("ocf_type", ocf_type);
				if (!(ocf_type == 1 || ocf_type == 0)) {
					ocf_type = (tm_ocf_type_t) 0;
				}
			} else {
				ocf_type = (tm_ocf_type_t) 0;
			}
			if (tm_t.exists("sec_hdr_on")) {
				tm_t.lookupValue("sec_hdr_on", sec_hdr_on);
				if (!(sec_hdr_on == 1 || sec_hdr_on == 0)) {
					sec_hdr_on = (tm_sec_hdr_flag_t) 0;
				}
			} else {
				sec_hdr_on = (tm_sec_hdr_flag_t) 0;
			}
			if (tm_t.exists("sync_flag")) {
				tm_t.lookupValue("sync_flag", sync_flag);
				if (!(sync_flag == 1 || sync_flag == 0)) {
					sync_flag = (tm_sync_flag_t) 0;
				}
			} else {
				sync_flag = (tm_sync_flag_t) 0;
			}
			if (tm_t.exists("stuff_state")) {
				tm_t.lookupValue("stuff_state", stuff_state);
				if (!(stuff_state == 1 || stuff_state == 0)) {
					stuff_state = (tm_stuff_state_t) 1;
				}
			} else {
				stuff_state = (tm_stuff_state_t) 1;
			}
			std::string tm_name;
			if (tm_t.exists("vc_name")) {
				tm_t.lookupValue("vc_name", tm_name);
			}
			osdlp_tm_init(&tm_f, scid, mc_count, (uint8_t) tm_vcid,
			              (tm_ocf_flag_t) ocf, (tm_ocf_type_t) ocf_type,
			              (tm_sec_hdr_flag_t) sec_hdr_on, (tm_sync_flag_t) sync_flag,
			              0, NULL, (tm_crc_flag_t) crc, m_params->tm_frame_len,
			              m_params->tm_max_sdu_len, MAX_TM_VCS, 20,
			              (tm_stuff_state_t) stuff_state, tm_util);
			virtual_channel::sptr vc_tm = virtual_channel::make_shared(tm_f,
			                              tm_f.mission.vcid, tm_name);
			vc_tm_configs->push_back(vc_tm);
		}

		/* ------------------------ TC -----------------------------------*/

		int tc_vcid, tc_crc, seg_hdr_flag;
		const libconfig::Setting &tc_vcs = root["tc"];
		size_t num_tcs = tc_vcs.getLength();
		for (int i = 0; i < num_tcs; i++) {
			libconfig::Setting &tc_t = tc_vcs[i];
			if (!(tc_t.exists("vcid"))) {
				throw std::runtime_error(
				        "TC Configuration incomplete - VCID missing");
			}
			struct tc_transfer_frame tc_f;
			struct cop_config cop;
			if (instance.compare("fop") == 0) {
				if (!(tc_t.exists("fop"))) {
					std::cerr
					                << "TC Configuration incomplete - FOP config missing. Ignorring VC..."
					                << std::endl;
					continue;
				} else {
					int t1_init, tx_lim, width, tt;
					const libconfig::Setting &fop_cfg = tc_t["fop"];
					if (fop_cfg.exists("t1_init")) {
						fop_cfg.lookupValue("t1_init", t1_init);
					} else {
						t1_init = 20;
					}

					if (fop_cfg.exists("transmission_limit")) {
						fop_cfg.lookupValue("transmission_limit", tx_lim);
					} else {
						tx_lim = (uint8_t) 3;
					}

					if (fop_cfg.exists("win_width")) {
						fop_cfg.lookupValue("win_width", width);
					} else {
						width = (uint8_t) 5;
					}

					if (fop_cfg.exists("timeout_type")) {
						fop_cfg.lookupValue("timeout_type", tt);
					} else {
						tt = (uint8_t) 0;
					}
					osdlp_prepare_fop(&cop.fop, (uint8_t) width, FOP_STATE_INIT,
					                  (uint16_t) t1_init, (uint8_t) tt, (uint8_t) tx_lim);
				}
			} else if (instance.compare("farm") == 0) {
				if (!(tc_t.exists("farm"))) {
					std::cerr
					                << "TC Configuration incomplete - FARM config missing. Ignorring VC..."
					                << std::endl;
					continue;
				} else {
					int width;
					const libconfig::Setting &farm_cfg = tc_t["farm"];
					if (farm_cfg.exists("win_width")) {
						farm_cfg.lookupValue("win_width", width);
					} else {
						width = 5;
					}
					osdlp_prepare_farm(&cop.farm, FARM_STATE_OPEN,
					                   (uint16_t) width);
				}
			}
			int ret;
			tc_t.lookupValue("vcid", tc_vcid);
			if (!(tc_vcid >= 0 && tc_vcid <= 63)) {
				std::cerr << "Invalid VC ID. Ignorring VC..." << std::endl;
				continue;
			}

			if (tc_t.exists("segmentation")) {
				tc_t.lookupValue("segmentation", seg_hdr_flag);
				if (!(seg_hdr_flag == 1 || seg_hdr_flag == 0)) {
					seg_hdr_flag = (tc_seg_hdr_t) 0;
				}
			} else {
				seg_hdr_flag = (tc_seg_hdr_t) 0;
			}
			if (tc_t.exists("crc")) {
				tc_t.lookupValue("crc", tc_crc);
				if (!(tc_crc == 1 || tc_crc == 0)) {
					tc_crc = (tc_crc_flag_t) 1;
				}
			} else {
				tc_crc = (tc_crc_flag_t) 1;
			}
			std::string tc_name;
			if (tc_t.exists("vc_name")) {
				tc_t.lookupValue("vc_name", tc_name);
			}
			if (!(tc_t.exists("map"))) {
				throw std::runtime_error(
				        "TC Configuration incomplete - MAP missing");
			}
			const libconfig::Setting &maps = tc_t["map"];
			size_t num_maps = maps.getLength();
			osdlp_tc_init(&tc_f, scid, m_params->tc_max_sdu_len,
			              m_params->tc_max_frame_len, m_params->tc_rx_queue_max_cap,
			              tc_vcid, 0, (tc_crc_flag_t) tc_crc,
			              (tc_seg_hdr_t) seg_hdr_flag, (tc_bypass_t) 0, (tc_ctrl_t) 0,
			              tc_util, cop);
			virtual_channel::sptr vc_tc = virtual_channel::make_shared(tc_f,
			                              tc_f.mission.vcid, tc_name);
			for (int j = 0; j < num_maps; j++) {

				libconfig::Setting &mp = maps[j];
				if (!(mp.exists("mapid"))) {
					throw std::runtime_error(
					        "TC Configuration incomplete - MAP ID missing");
				}
				int mapid;
				mp.lookupValue("mapid", mapid);
				if (!(mapid >= 0 && mapid <= 31)) { // TODO check standard
					std::cerr << "Invalid VC ID. Ignorring VC..." << std::endl;
					continue;
				}
				tc_bypass_t bp;
				if (mp.exists("bypass")) {
					mp.lookupValue("bypass", ret);
					if (ret == 1 || ret == 0) {
						bp = (tc_bypass_t) ret;
					} else {
						bp = (tc_bypass_t) 0;
					}
				} else {
					bp = (tc_bypass_t) 0;
				}
				tc_ctrl_t ctrl;
				if (mp.exists("ctrl_cmd")) {
					mp.lookupValue("ctrl_cmd", ret);
					if (ret == 1 || ret == 0) {
						ctrl = (tc_ctrl_t) ret;
					} else {
						ctrl = (tc_ctrl_t) 0;
					}
				} else {
					ctrl = (tc_ctrl_t) 0;
				}
				std::vector<uint8_t> data;
				std::string data_in;
				if (mp.exists("data")) {
					mp.lookupValue("data", data_in);
				}

				int ret = tokenize(data_in, &data);
				if (ret) {
					std::cerr << "Invalid data in MAP " << mapid <<
					          " VCID " << tc_vcid << " . Ignoring ... " <<
					          std::endl;
				}
				std::string name;
				if (mp.exists("name")) {
					mp.lookupValue("name", name);
				}
				vc_tc->add_map((uint16_t) mapid, bp, ctrl, data, name);
			}
			vc_tc_configs->push_back(vc_tc);
		}
	} catch (libconfig::SettingNotFoundException &e) {
		printf("Essential setting not found\n");
	}
	return 0;
}

int
tokenize(std::string str, std::vector<uint8_t> *data_out)
{
	std::string token;
	std::istringstream tokenStream(str);
	std::stringstream ss;
	int val_in;
	uint8_t val;
	while (std::getline(tokenStream, token, 'x')) {
		if (token == "")
			continue;
		val_in = std::stoi(token, 0, 16);
		if (val_in < 0 || val_in > 255) {
			data_out->clear();
			return -1;
			break;
		} else {
			val = (uint8_t) val_in;
			data_out->push_back(val);
		}
	}
	return 0;
}
