#ifndef SIG_H
#define SIG_H

extern void sig_catch(int,void (*)(int));

extern void sig_pipeignore(void);
extern void sig_pipedefault(void);

#endif
