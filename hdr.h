/*$Id$*/

#ifndef HDR_H
#define HDR_H

extern void hdr_transferenc(void);
extern void hdr_ctboundary(void);
extern void hdr_datemsgid(unsigned long when);
extern void hdr_mime(const char *ctype);
extern void hdr_ctype(const char *ctype);

#endif
