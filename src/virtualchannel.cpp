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

#include "virtualchannel.h"

virtual_channel::virtual_channel(struct tm_transfer_frame tm_transfer_frame,
                                 struct tc_transfer_frame tc_transfer_frame,
                                 uint16_t vcid):
	d_vcid(vcid),
	d_tm_config(tm_transfer_frame),
	d_tc_config(tc_transfer_frame)
{

}

virtual_channel::virtual_channel(struct tm_transfer_frame tm_transfer_frame,
                                 uint16_t vcid):
	d_vcid(vcid),
	d_tm_config(tm_transfer_frame)
{

}

virtual_channel::virtual_channel(struct tc_transfer_frame tc_transfer_frame,
                                 uint16_t vcid):
	d_vcid(vcid),
	d_tc_config(tc_transfer_frame)
{

}

virtual_channel::sptr
virtual_channel::make_shared(struct tm_transfer_frame tm_transfer_frame,
                             struct tc_transfer_frame tc_transfer_frame,
                             uint16_t vcid)
{
	return std::shared_ptr<virtual_channel>(new virtual_channel(tm_transfer_frame,
	                                        vcid));
}

virtual_channel::sptr
virtual_channel::make_shared(struct tm_transfer_frame tm_transfer_frame,
                             uint16_t vcid)
{
	return std::shared_ptr<virtual_channel>(new virtual_channel(tm_transfer_frame,
	                                        vcid));
}

virtual_channel::sptr
virtual_channel::make_shared(struct tc_transfer_frame tc_transfer_frame,
                             uint16_t vcid)
{
	return std::shared_ptr<virtual_channel>(new virtual_channel(tc_transfer_frame,
	                                        vcid));
}

virtual_channel::~virtual_channel()
{
}

void
virtual_channel::add_map(uint16_t mapid, tc_bypass_t bp, tc_ctrl_t ctrl,
                         uint8_t *data)
{
	struct map m;
	m.mapid = mapid;
	m.bypass = bp;
	m.ctrl = ctrl;
	m.data = data;
	d_mapids.push_back(m);
	return;
}

uint16_t
virtual_channel::get_vcid()
{
	return d_vcid;
}

struct tm_transfer_frame &
virtual_channel::get_tm_config()
{
	return d_tm_config;
}

struct tc_transfer_frame &
virtual_channel::get_tc_config()
{
	return d_tc_config;
}

std::vector<virtual_channel::map> &
virtual_channel::get_maps()
{
	return d_mapids;
}

std::deque<struct local_queue_item> &
virtual_channel::get_sent_queue()
{
	return d_sent_queue;
}

std::deque<std::vector<uint8_t>> &
                              virtual_channel::get_rx_queue()
{
	return d_rx_queue;
}

std::deque<std::vector<uint8_t>> &
                              virtual_channel::get_tx_queue()
{
	return d_tx_queue;
}

std::vector<struct tc_transfer_frame> &
virtual_channel::get_wait_queue()
{
	return d_wait_queue;
}

