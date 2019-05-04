#ifndef _AWC_TTY_H
#define _AWC_TTY_H

void awc_tty_open(unsigned int num);
void awc_tty_sethand(void (*ext_acq) (void), void (*ext_rel) (void));
void awc_tty_activate(void);

#endif // _AWC_TTY_H
