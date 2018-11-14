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


const int BUFSIZE = 4096;

int
main(int argc, char **argv)
{
  int dhcpInFd;
  // int dhcpOutBroadcastFd;
  // struct sockaddr_in broadcastAddr;
  
  struct sockaddr_in clientaddr;
  unsigned int clientlen =  sizeof(clientaddr);
  char *hostaddrp;
  
  unsigned char buf[BUFSIZE]; 
  int n; 
  
  /* JA FIXME: Need to restrict this to only the ip associated with the
     private backend network */ 
  dhcpInFd = openUDPSocket(INADDR_ANY,67,0);

  // dhcpOutBroadcastFd = openUDPSocket(INADDR_ANY,,1);
  // bzero((char *) &GLOBALS.broadcastAddr, sizeof(GLOBALS.broadcastAddr));
  // broadcastAddr.sin_family = AF_INET;
  // broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  // addr.sin_port = ;
  

  /* For the moment we are just looping will eventually 
     make this event driven */
  while (1) {
    bzero(buf, BUFSIZE);
    n = recvfrom(dhcpInFd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
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


