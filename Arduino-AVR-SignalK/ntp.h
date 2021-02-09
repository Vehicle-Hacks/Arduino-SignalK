/*   Arduino - SignalK
 *   Copyright (C) 2019 Ralph Grewe
 *  
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 
const unsigned int localNtpPort = 8888;      // local port to listen for UDP packets
const unsigned int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

void sendNtpRequest(const char * address);
void readNtpTimestamp(char* buffer);
