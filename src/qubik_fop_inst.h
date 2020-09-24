/*
 * qubik_fop_inst.h
 *
 *  Created on: May 27, 2020
 *      Author: vardakis
 */

#ifndef SRC_QUBIK_FOP_INST_H_
#define SRC_QUBIK_FOP_INST_H_

class qubik_fop_inst
{
public:
	qubik_fop_inst();
	~qubik_fop_inst();

	void
	fop_transmitter();

	void
	fop_receiver();

	typedef enum {
		NOT_CONNECTED = 0,
		CONNECTED = 1
	} link_state_t;
};


#endif /* SRC_QUBIK_FOP_INST_H_ */
