const unsigned int localNtpPort = 8888;      // local port to listen for UDP packets
const unsigned int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

void sendNtpRequest(const char * address);
void readNtpTimestamp(char* buffer);
