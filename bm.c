/* START OF THIS CODE WAS OBTAINED FROM

https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpserver.c
*/

/* 
* udpserver.c - A simple UDP echo server 
* usage: udpserver <port>
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bm.h"

typedef unsigned char byte;

const byte DHCP_OP_BOOTREQUEST = 1;
const byte DHCP_OP_BOOTREPLY = 2;
const byte DHCP_FLAGS_BROADCAST[2] = { 0x80, 0x00 };

struct DHCP_DISCOVERY_MESSAGE {
	byte Op;
	byte Htype;
	byte Hlen;
	byte Hops;
	byte XID[4];
	byte Sec[2];
	byte Flags[2];
	byte CIAddr[4];
	byte YIAddr[4];
	byte SIAddr[4];
	byte GIAddr[4];
	byte CHAddr[16];
	byte SName[64];
	byte File[128];
	byte Options[];
};

const byte DHCP_OPTIONS_MAGIC[4] = { 99, 130, 83, 99 };
const byte DHCP_OPTION_PAD_CODE = 0x00;
const byte DHCP_OPTION_END_CODE = 0xff;

struct DHCP_OPTION {
	byte Code;
	byte Len;
	byte Data[];
};


const byte TFTP_OPCODE_RD_REQ[2] = { 0x01, 0x00 };
const byte TFTP_OPCODE_WR_REQ[2] = { 0x02, 0x00 };
const byte TFTP_OPCODE_DATA[2]   = { 0x03, 0x00 };
const byte TFTP_OPCODE_ACK[2]    = { 0x04, 0x00 };
const byte TFTP_OPCODE_ERROR[2]  = { 0x05, 0x00 };

struct TFTP_ERROR {
	const byte ErrorCode[2];
	char  ErrorString[80];
} TFPT_ERRORS[] = {
	{ { 0x0, 0x0 }, "Not defined, see error message (if any)." },
	{ { 0x1, 0x0 }, "File not found." },
	{ { 0x2, 0x0 }, "Access violation"},
	{ { 0x3, 0x0 }, "Disk full or allocation exceeded" },
	{ { 0x4, 0x0 }, "Illegal TFTP operation" },
	{ { 0x5, 0x0 }, "Unknown transfer ID" },
	{ { 0x6, 0x0 }, "File already exists" },
	{ { 0x7, 0x0 }, "No such user" }
};

struct TFTP_HEADER {
	byte OpCode[2];
};

struct TFTP_REQUEST_PACKET {
	struct TFTP_HEADER Header;
	byte   FileModeOptions[];
	// byte FileName[]; 0 terminated byte array
	// byte Mode[]; 0 terminated byte array
	// byte Options[]; 0 terminated byte array
};

struct TFTP_DATA_PACKET {
	struct TFTP_HEADER Header;
	byte BlockNumber[2];
	byte Data[];
};

struct TFTP_ACK_PACKET {
	struct TFTP_HEADER Header;
	byte BlockNumber[2];
};

struct TFTP_ERROR_PACKET {
	struct TFTP_HEADER Header;
	byte Error[2];
	byte ErrMsg[];
};

struct TFTP_OPTION_ACK_PACKET {
	struct TFTP_HEADER Header;
	byte Options[];
};



/*
* error - wrapper for perror
*. WARNING WILL NEED TO PROVIDE THIS IF YOU ARE 
*. PORTING
*/
void error(char *msg) {
	perror(msg);
}


void asciiDump(unsigned char *s, int len)
{
	unsigned char c;
	for (int i=0; i<len; i++) {
		c=s[i];
		if (c>=' ' && c<='~') printf("%c", c);
		else printf(".");
	}
}

int hexDump(unsigned char *start, int bytes) 
{
	int j;
	for (j=0; j<bytes;) {
		int offset=j*16;
		int end=((bytes-j) < 16) ? bytes-j : 16;
		printf("%08x:  ", offset);
		for (int i=0;i<end;i++) {
			printf("%02x  ", start[offset+i]);
		}
		for (int i=end;i<16;i++) printf("    ");
		printf("|");
		for (int i=0;i<end;i++) {
			unsigned char c=start[offset+i];
			if (c>=' ' && c<='~') printf("%c", c);
			else printf(".");
		}
		printf("|\n");
		j+=end;
	}
	return j;
}

/*
There are several special addresses: INADDR_LOOPBACK (127.0.0.1) always refers to the local
host via  the  loopback  device;  INADDR_ANY  (0.0.0.0)  means  any  address  for  binding;
INADDR_BROADCAST  (255.255.255.255)  means  any  host  and  has  the same effect on bind as
INADDR_ANY for historical reasons.
*/
int openUDPSocket(uint32_t myaddr, char *interface, unsigned short portno, int broadcast)
{
	int sockfd;
	int optval = 1;
	struct sockaddr_in addr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
		goto out;
	}

	if (broadcast) {
		/* Set socket to allow broadcast */
		if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&optval, sizeof(optval)) < 0) {
			error("setsockopt(SO_BROADCAST) failed");
			goto out_close;
		}
	}

	if (interface != NULL) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0) {
			error("setsockopt(SO_BINDTODEVICE) failed");
			goto out_close;
		}
	}

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	(const void *)&optval , sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = myaddr;
	addr.sin_port = htons(portno);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		error("ERROR on binding");
		goto out_close;
	}

	return sockfd;

out_close:
	close(sockfd);
out:
	return -1;
}

int
processDHCPOption(struct DHCP_OPTION *o)
{

	if (o->Code == DHCP_OPTION_PAD_CODE) {
		printf("\tCode: 0x00 Pad\n");
		return 1;
	}
	if (o->Code == DHCP_OPTION_END_CODE) {
		printf("\tCode: 0xff End\n");
		return -1;
	}
	printf("\tCode: 0x%02x\n\tLen: %d\n", o->Code, o->Len);
	hexDump(o->Data, o->Len);
	return (sizeof(struct DHCP_OPTION) + o->Len);
}

int
processDHCPMessage(byte *buf, int len)
{
	struct DHCP_DISCOVERY_MESSAGE *m;

	if (len < sizeof(struct DHCP_DISCOVERY_MESSAGE))
		return 0;
	m = (struct DHCP_DISCOVERY_MESSAGE *) buf;

	printf(">>>> DHCP PACKET: buf:%p len:%d\n", m, len);
	printf("Op: 0x%02x\nHtype: 0x%02x\nHlen: %d\nHops: 0x%02x\n",
	m->Op, m->Htype, m->Hlen, m->Hops);
	printf("XID: %02x%02x%02x%02x\nSec: %02x%02x\nFlags: %02x%02x\n",
	m->XID[0],m->XID[1],m->XID[2],m->XID[3],
	m->Sec[0],m->Sec[1],
	m->Flags[0],m->Flags[1]);
	printf("CIAddr: %02x%02x%02x%02x\n",
	m->CIAddr[0],m->CIAddr[1],m->CIAddr[2],m->CIAddr[3]);
	printf("YIAddr: %02x%02x%02x%02x\n",
	m->YIAddr[0],m->YIAddr[1],m->YIAddr[2],m->YIAddr[3]);
	printf("SIAddr: %02x%02x%02x%02x\n",
	m->SIAddr[0],m->SIAddr[1],m->SIAddr[2],m->SIAddr[3]);
	printf("GIAddr: %02x%02x%02x%02x\n",
	m->GIAddr[0],m->GIAddr[1],m->GIAddr[2],m->GIAddr[3]);
	printf("CHAddr: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n",
	m->CHAddr[0],m->CHAddr[1],m->CHAddr[2],m->CHAddr[3],
	m->CHAddr[4],m->CHAddr[5],m->CHAddr[6],m->CHAddr[7],
	m->CHAddr[8],m->CHAddr[9],m->CHAddr[10],m->CHAddr[11],
	m->CHAddr[12],m->CHAddr[13],m->CHAddr[14],m->CHAddr[15]);
	printf("SName: ");
	asciiDump(m->SName, sizeof(m->SName));
	printf("\nFile: ");
	asciiDump(m->File, sizeof(m->File));
	printf("\n");

	len = len - sizeof(struct DHCP_DISCOVERY_MESSAGE);

	if (len > 4
			&& m->Options[0] == DHCP_OPTIONS_MAGIC[0]
			&& m->Options[1] == DHCP_OPTIONS_MAGIC[1]
			&& m->Options[2] == DHCP_OPTIONS_MAGIC[2]
			&& m->Options[3] == DHCP_OPTIONS_MAGIC[3]) {
		len = len - 4;
		int n = 4;
		struct DHCP_OPTION *o = (struct DHCP_OPTION *)(&m->Options[n]);
		printf("Options:\n");
		while (len && (n=processDHCPOption(o))) {
			if (n < 0) break;
			len = len - n;
			o = (struct DHCP_OPTION *) ((byte *)o + n);
		}
	}
	printf("<<<< DHCP PACKET\n");
#if 0
	/* 
	 * sendto: echo the input back to the client 
	 */
	n = sendto(bcastReplyfd, buf, strlen(buf), 0, 
	(struct sockaddr *) &clientaddr, clientlen);
	if (n < 0) 
		error("ERROR in sendto");
#endif

	return 1;
}	
