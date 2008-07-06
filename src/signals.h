#ifndef SIGNALS_H
#define SIGNALS_H

/* base signals routine */

#include <signal.h>

/** install signal handler
@param sig signal
@param signal_proc signal handler
*/
void install_signal_handler(int sig, void (*signal_proc)(int , siginfo_t *, void *));

#endif // SIGNALS_H
