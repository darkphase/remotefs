#ifndef COMPAT_H
#define COMPAT_H

#if defined FREEBSD
#endif /* FREEBSD */

#if defined DARWIN
#endif /* DARWIN */

#if defined SOLARIS
#endif /* SOLARIS */

#if defined QNX
#endif /* QNX */

#if ! defined O_ASYNC
#	define O_ASYNC 0
#endif

#endif /* COMPAT_H */
