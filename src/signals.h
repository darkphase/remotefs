#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void install_signal_handlers(int signal, void (*signal_proc)(int , siginfo_t *, void *));

#endif // SIGNALS_H
