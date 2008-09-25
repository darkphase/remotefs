#ifndef COMPAT_H
#define COMPAT_H

#if defined FREEBSD
#	define EREMOTEIO ECANCELED
#	define EBADE EINVAL
#endif /* FREEBSD */

#if defined SOLARIS
#	define EREMOTEIO ECANCELED
#endif /* SOLARIS */

#if ! defined O_ASYNC
#	define O_ASYNC 0
#endif

#endif /* COMPAT_H */
