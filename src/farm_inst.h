/*
 * farm_inst.h
 *
 *  Created on: May 28, 2020
 *      Author: vardakis
 */


class farm_instance
{
public:
	farm_instance();
	virtual
	~farm_instance();

	static farm_instance *
	make();

	virtual void
	farm_transmitter() = 0;

	virtual void
	farm_receiver() = 0;

};

