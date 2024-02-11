#ifndef MIME_H
#define MIME_H

#include "stralloc.h"

extern void decodeQ(const char *,unsigned int,stralloc *);
extern void decodeB(const char *,unsigned int,stralloc *);
extern void encodeQ(const char *,unsigned int,stralloc *);
extern void encodeB(const char *,unsigned int,stralloc *,int);
extern void decodeHDR(const char *,unsigned int,stralloc *);
extern void concatHDR(const char *,unsigned int,stralloc *);
extern int unfoldHDR(char *,unsigned int,stralloc *,const char *, const stralloc *,int);

extern void author_addr(stralloc *out,const char *,unsigned int);
extern void author_name(stralloc *out,const char *,unsigned int,int);

/* Characters */
#define ESC 0x1B
#define SI 0x0F
#define SO 0x0E
/* iso-2022-jp back-to-ascii seq */
#define TOASCII "\x1B(B"
/* to JP. Last char [B|@] must be added */
#define TOJP "\x1B$"

/* iso-2022 SI sequence as string */
#define TOSI "\x0F"
/* SI \n SO */
#define SI_LF_SO "\x0F\n\x0E"

/* in these bit 0 determines the number of bytes (1 or 2) in ss2/ss3 codes */
/* it is 2 for CN,1 for JP, and they are not used for KR  bit 3 for        */
/* iso-2022 */
#define CS_2022_MASK 0x08
#define CS_2022_JP 0x08
#define CS_2022_KR 0xA0
#define CS_NONE 0
#define CS_BAD 0xffff

#define CS_2022_CN 0x09
/* Other Chinese ones. bit 7 set means MSB of 2-byte seq. No ss2/ss3 consid*/
#define CS_CN 0x10

#define MIME_NONE 0
#define MIME_APPLICATION_OCTETSTREAM 1
#define MIME_MULTI 0x80
#define MIME_MULTI_ALTERNATIVE 0x81
#define MIME_MULTI_MIXED 0x82
#define MIME_MULTI_DIGEST 0x83
#define MIME_MULTI_SIGNED 0x84

#define MIME_TEXT 0x40
#define MIME_TEXT_PLAIN 0x41
#define MIME_TEXT_HTML 0x42
#define MIME_TEXT_ENRICHED 0x43
#define MIME_TEXT_VCARD 0x44

#define MIME_MESSAGE 0x20
#define MIME_MESSAGE_RFC822 0x21

#define CTENC_NONE 0
#define CTENC_QP 1
#define CTENC_BASE64 2

/* this is a linked list of mime type info. */
typedef struct {
	int level;
	unsigned int mimetype;
	unsigned int ctenc;
	unsigned int cs;		/* charset flag - expand later */
	void *previous;
	void *next;
	stralloc boundary;
	stralloc charset;
	stralloc ctype;
} mime_info;

#endif
