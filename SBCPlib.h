#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// HEADER TYPE
static const int JOIN=2;
static const int SEND=4;
static const int FWD=3;
static const int ACK=7;
static const int NAK=5;
static const int ONLINE=8;
static const int OFFLINE=6;
static const int IDLE=9;

// ATTRIBUTE TYPE
static const int USERNAME=2;
static const int MESSAGE=4;
static const int REASON=1;
static const int CLIENT_COUNT=3;

struct SBCP_header{
	unsigned int vrsn : 9;
	unsigned int type : 7;
	int length;
};
struct SBCP_attribute{
	int type;
	int length;
	char payload[512];
};
struct SBCP_message{
	struct SBCP_header header;
	struct SBCP_attribute attribute[2];
};
struct SBCP_client_info{
	char username[16];
	int fd;
};

