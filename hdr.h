/*$Id$*/

#ifndef HDR_H
#define HDR_H

enum ctype {
  CTYPE_TEXT,
  CTYPE_MULTIPART,
  CTYPE_DIGEST,
  CTYPE_MESSAGE,
};

extern void hdr_transferenc(void);
extern void hdr_ctboundary(void);
extern void hdr_datemsgid(unsigned long when);
extern void hdr_mime(enum ctype ctype);
extern void hdr_ctype(enum ctype ctype);
extern void hdr_from(const char *append);
extern void hdr_boundary(int last);

extern void hdr_add(const char *value,unsigned int len);
extern void hdr_adds(const char *line);
extern void hdr_add2(const char *start,const char *value,unsigned int len);
extern void hdr_add2s(const char *start,const char *value);

#endif
