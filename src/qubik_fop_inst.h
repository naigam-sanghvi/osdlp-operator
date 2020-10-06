/*
 * qubik_fop_inst.h
 *
 *  Created on: May 27, 2020
 *      Author: vardakis
 */

#ifndef SRC_QUBIK_FOP_INST_H_
#define SRC_QUBIK_FOP_INST_H_

#define VCID 1

#include "logger.h"

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

	logger::sptr
	get_logger();

private:
	static void
	forward_frames(int sockfd, struct sockaddr_in servaddr);

};


#endif /* SRC_QUBIK_FOP_INST_H_ */
