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

#include <iostream>
#include <cstdio>
#include <cstring>
#include <thread>
#include "queue_api.h"
#include "virtualchannel.h"
#include "config.h"
#include "qubik_fop_inst.h"
#include "qubik_farm_inst.h"

extern "C" {
#include "../lib/osdlp/include/osdlp.h"
}

int
main(int argc, char *argv[])
{
	qubik_fop_inst fop = qubik_fop_inst();
	qubik_farm_inst farm = qubik_farm_inst();
	std::string path(argv[1]);
	init_structs(path);
	if (get_mission_params().instance.compare("fop") == 0) {
		std::thread th1(&qubik_fop_inst::fop_transmitter, fop);
		std::thread th2(&qubik_fop_inst::fop_receiver, fop);
		th1.join();
		th2.join();
	} else {
		std::thread th1(&qubik_farm_inst::farm_transmitter, farm);
		std::thread th2(&qubik_farm_inst::farm_receiver, farm);
		th1.join();
		th2.join();
	}
}
