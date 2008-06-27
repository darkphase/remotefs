#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void install_signal_handler(int signal, void (*signal_proc)(int , siginfo_t *, void *));
void reset_signal_handler(int signal);

#endif // SIGNALS_H
