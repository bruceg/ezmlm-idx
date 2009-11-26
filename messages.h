#ifndef MESSAGES_H
#define MESSAGES_H

extern const char *messages_getn(const char *msg,const char *params[10]);
extern const char *messages_get0(const char *name);
extern const char *messages_get1(const char *name,const char *p1);
extern const char *messages_get2(const char *name,const char *p1,const char *p2);

#define MSG(X) messages_get0(#X)
#define MSG1(X,P1) messages_get1(#X,P1)
#define MSG2(X,P1,P2) messages_get2(#X,P1,P2)

#endif
