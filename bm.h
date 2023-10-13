#ifndef __BM_H__
#define __BM_H__

#include <stdint.h>

// Functional Interface to the BootMe library
// For the moment have avoided any custom types in the interface

extern void error(char *msg);
extern void asciiDump(unsigned char *s, int len);
extern int  hexDump(unsigned char *start, int bytes);
extern int  openUDPSocket(uint32_t myaddr, char *interface, unsigned short portno, int broadcast);
extern int  processDHCPMessage(unsigned char  *buf, int len);

#endif
