#ifndef MESSAGES_H
#define MESSAGES_H

extern const char *messages_getn(const char *msg,const char *params[10]);
extern const char *messages_get(const char *name,
				const char *p1,
				const char *p2);
#define MSG(X) messages_get(#X,0,0)
#define MSG1(X,P1) messages_get(#X,P1,0)
#define MSG2(X,P1,P2) messages_get(#X,P1,P2)

#endif
