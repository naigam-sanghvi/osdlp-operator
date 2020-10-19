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

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include "mission_config.h"
#include "queue_api.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

class logger
{
public:
	typedef std::shared_ptr<logger> sptr;
	static sptr
	make_shared(uint16_t port);

	~logger();

	/**
	 * @param string to print to stdout
	 * @param rm_n remove new line chars
	 */
	void
	log_output(std::string, bool rm_n);

protected:
	logger(uint16_t port);

private:
	int d_sockfd;
	struct sockaddr_in d_servaddr;
	uint16_t d_port;
};

static void
log_output(std::string out_str);

#endif /* SRC_LOGGER_H_ */
