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
 *
 *
 * Its based on: The Arduino UDPSendReceiveString example from Nov 3rd 2019:
 * https://www.arduino.cc/en/Tutorial/UDPSendReceiveString
 * The Arduino UdpNtpClient example from Nov. 25th 2019:
 * https://www.arduino.cc/en/Tutorial/UdpNtpClient
 * And a stackoverflow post about converting unix timestamp to a date:
 * https://stackoverflow.com/questions/7136385/calculate-day-number-from-an-unix-timestamp-in-a-math-way
 */

#include "Ethernet.h"
#include "EthernetUdp.h"
#include "ntp.h"

char packetBuffer[NTP_PACKET_SIZE];

extern char timeServer[];
extern EthernetUDP udp;

void sendNtpRequest(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void readNtpTimestamp(char* buffer) {
  if (udp.parsePacket()) {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

// from: https://stackoverflow.com/questions/7136385/calculate-day-number-from-an-unix-timestamp-in-a-math-way
    long z = epoch / 86400L + 719468L;
    long era = (z >= 0 ? z : z - 146096L) / 146097L;
    unsigned long doe = static_cast<unsigned long>(z - era * 146097L);
    unsigned long yoe = (doe - doe/1460 + doe/36524L - doe/146096L) / 365L;
    long y = static_cast<long>(yoe) + era * 400L;
    unsigned long doy = doe - (365L*yoe + yoe/4L - yoe/100L);
    unsigned long mp = (5L*doy + 2L)/153L;
    unsigned long d = doy - (153L*mp+2L)/5L + 1L;
    unsigned long m = mp + (mp < 10L ? 3L : -9L);
    y += (m <= 2L);

    sprintf(buffer, "%04d-%02d-%02dT%02d:%d:%02d", int(y), int(m), int(d), int((epoch  % 86400L) / 3600), int((epoch  % 3600) / 60), int(epoch % 60));
  }  
}
