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
#include "logger.h"
#include <string.h>
#include <chrono>
#include <algorithm>

logger::sptr
logger::make_shared(uint16_t port)
{
	return std::shared_ptr<logger>(new logger(port));
}

logger::logger(uint16_t port)
{
	d_port = port;
	if ((d_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&d_servaddr, 0, sizeof(d_servaddr));
	d_servaddr.sin_family = AF_INET;
	d_servaddr.sin_port = htons(port);
	d_servaddr.sin_addr.s_addr = inet_addr(OUTPUT_ADDR);
}

void
logger::log_output(std::string out_str)
{
	std::time_t n = std::chrono::system_clock::to_time_t(
	                        std::chrono::system_clock::now());
	std::string out_time = std::string(std::ctime(&n)) + ": " + out_str;
	out_time.erase(std::remove(out_time.begin(), out_time.end(), '\n'),
	               out_time.end());
	out_time += "\n";
	sendto(d_sockfd, out_time.data(), out_time.size(),
	       MSG_CONFIRM, (const struct sockaddr *) &d_servaddr, sizeof(d_servaddr));
}

logger::~logger()
{

}

