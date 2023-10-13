/* START OF THIS CODE WAS OBTAINED FROM

https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpserver.c
*/

/* 
* udpserver.c - A simple UDP echo server 
* usage: udpserver <port>
*/

#include "bm.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

const int BUFSIZE = 4096;

#define OPTION(opts, text)	fprintf(stderr, " %-32s  %s\n", opts, text)
#define CONT(text) 		fprintf(stderr, " %-32s  %s\n", "", text)

extern int errno;
extern int optind;
extern char *optarg;

static void print_usage(char *exe)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s [options] /path/to/kernel_image\n\n", exe);
	fprintf(stderr, "  options:\n");

	OPTION("--help, -h", "Prints this message.");
	OPTION("--bind-to, -B <iface name>", "Restrict incoming packets to only those on specified interface");
}

int main(int argc, char **argv)
{
	int dhcpInFd;

	struct sockaddr_in clientaddr;
	unsigned int clientlen =  sizeof(clientaddr);
	char *hostaddrp;
	char *bind_iface = NULL;

	unsigned char buf[BUFSIZE]; 
	int n;
	int ret = 0;
	int index = 0;

	char opts[] = "+hB:";
	struct option long_opts[] = {
		{ "help",		no_argument, NULL, 'h'},
		{ "bind-to",		required_argument, NULL, 'B'},
		{0}
	};

	while (ret != -1) {
		ret = getopt_long(argc, argv, opts, long_opts, &index);
		switch (ret) {
		case 'h':
		case '?':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 'B':
			bind_iface = optarg;
			break;

		case -1:
			break;

		default:
			fprintf(stderr, "Unparsed option: %c\n", ret);
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	dhcpInFd = openUDPSocket(INADDR_BROADCAST, bind_iface, 67, 1);
	if(dhcpInFd < 0) {
		printf("Failed to create UDP socket\n");
		exit(EXIT_FAILURE);
	}

	/* For the moment we are just looping will eventually 
	make this event driven */
	while (1) {
		bzero(buf, BUFSIZE);
		n = recvfrom(dhcpInFd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server received datagram from %s\n", hostaddrp);
		printf("server received %d\n", n);
		hexDump(buf,n);
		//    processDHCPMessage(buf, n, dhcpOutBroadcastFd);
		processDHCPMessage(buf, n);    
	}
}

