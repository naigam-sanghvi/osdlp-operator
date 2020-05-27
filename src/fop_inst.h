/*
 * fop_inst.h
 *
 *  Created on: May 27, 2020
 *      Author: vardakis
 */

#ifndef SRC_FOP_INST_H_
#define SRC_FOP_INST_H_


class fop_instance
{
public:
	fop_instance();
	virtual
	~fop_instance();

	static fop_instance *
	make();

	virtual void
	fop_transmitter() = 0;

	virtual void
	fop_receiver() = 0;

};
#endif /* SRC_FOP_INST_H_ */
