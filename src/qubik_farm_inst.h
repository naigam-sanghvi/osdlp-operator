/*
 * qubik_farm_inst.h
 *
 *  Created on: May 28, 2020
 *      Author: vardakis
 */

#ifndef SRC_QUBIK_FARM_INST_H_
#define SRC_QUBIK_FARM_INST_H_

#include "farm_inst.h"

class qubik_farm_inst : farm_instance
{
public:
	qubik_farm_inst();
	~qubik_farm_inst();

	void
	farm_transmitter();

	void
	farm_receiver();

};

#endif /* SRC_QUBIK_FARM_INST_H_ */
