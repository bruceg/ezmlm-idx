/*$Id$*/

#ifndef HDR_H
#define HDR_H

extern void hdr_transferenc(void);
extern void hdr_ctboundary(void);
extern void hdr_datemsgid(unsigned long when);
extern void hdr_mime(const char *ctype);
extern void hdr_ctype(const char *ctype);
extern void hdr_from(const char *append);

extern void hdr_add(const char *value,unsigned int len);
extern void hdr_adds(const char *line);
extern void hdr_add2(const char *start,const char *value,unsigned int len);
extern void hdr_add2s(const char *start,const char *value);

#endif
