/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>

#include "compat.h"
#include "error.h"
#include "inet.h"

/** available errnos (POSIX.1)  */
enum errno_numbers
{
	RFS_SUCCESS = 0,
	RFS_E2BIG,
	RFS_EACCES,
	RFS_EADDRINUSE,
	RFS_EADDRNOTAVAIL,
	RFS_EAFNOSUPPORT,
	RFS_EAGAIN,
	RFS_EALREADY,
	RFS_EBADF,
	RFS_EBADMSG,
	RFS_EBUSY,
	RFS_ECANCELED,
	RFS_ECHILD,
	RFS_ECONNABORTED,
	RFS_ECONNREFUSED,
	RFS_ECONNRESET,
	RFS_EDEADLK,
	RFS_EDESTADDRREQ,
	RFS_EDOM,
	RFS_EDQUOT,
	RFS_EEXIST,
	RFS_EFAULT,
	RFS_EFBIG,
	RFS_EHOSTUNREACH,
	RFS_EIDRM,
	RFS_EILSEQ,
	RFS_EINPROGRESS,
	RFS_EINTR,
	RFS_EINVAL,
	RFS_EIO,
	RFS_EISCONN,
	RFS_EISDIR,
	RFS_ELOOP,
	RFS_EMFILE,
	RFS_EMLINK,
	RFS_EMSGSIZE,
	RFS_EMULTIHOP,
	RFS_ENAMETOOLONG,
	RFS_ENETDOWN,
	RFS_ENETRESET,
	RFS_ENETUNREACH,
	RFS_ENFILE,
	RFS_ENOBUFS,
	RFS_ENODATA,
	RFS_ENODEV,
	RFS_ENOENT,
	RFS_ENOEXEC,
	RFS_ENOLCK,
	RFS_ENOLINK,
	RFS_ENOMEM,
	RFS_ENOMSG,
	RFS_ENOPROTOOPT,
	RFS_ENOSPC,
	RFS_ENOSR,
	RFS_ENOSTR,
	RFS_ENOSYS,
	RFS_ENOTCONN,
	RFS_ENOTDIR,
	RFS_ENOTEMPTY,
	RFS_ENOTSOCK,
	RFS_ENOTSUP,
	RFS_ENOTTY,
	RFS_ENXIO,
	RFS_EOPNOTSUPP,
	RFS_EOVERFLOW,
	RFS_EPERM,
	RFS_EPIPE,
	RFS_EPROTO,
	RFS_EPROTONOSUPPORT,
	RFS_EPROTOTYPE,
	RFS_ERANGE,
	RFS_EROFS,
	RFS_ESPIPE,
	RFS_ESRCH,
	RFS_ESTALE,
	RFS_ETIME,
	RFS_ETIMEDOUT,
	RFS_ETXTBSY,
	RFS_EWOULDBLOCK,
	RFS_EXDEV
};

int hton_errno(int host_errno)
{
	switch (host_errno)
	{
	case 0:             return RFS_SUCCESS;
	
	case E2BIG:         return htonl(RFS_E2BIG);
	case EACCES:        return htonl(RFS_EACCES);
	case EADDRINUSE:    return htonl(RFS_EADDRINUSE);
	case EADDRNOTAVAIL: return htonl(RFS_EADDRNOTAVAIL);
	case EAFNOSUPPORT:  return htonl(RFS_EAFNOSUPPORT);
	case EAGAIN:        return htonl(RFS_EAGAIN);
#if EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:   return htonl(RFS_EWOULDBLOCK);
#endif
#if EALREADY != EBUSY
	case EALREADY:      return htonl(RFS_EALREADY);
#endif
	case EBADF:         return htonl(RFS_EBADF);
	case EBADMSG:       return htonl(RFS_EBADMSG);
	case EBUSY:         return htonl(RFS_EBUSY);
	case ECANCELED:     return htonl(RFS_ECANCELED);
	case ECHILD:        return htonl(RFS_ECHILD);
	case ECONNABORTED:  return htonl(RFS_ECONNABORTED);
	case ECONNREFUSED:  return htonl(RFS_ECONNREFUSED);
	case ECONNRESET:    return htonl(RFS_ECONNRESET);
	case EDEADLK:       return htonl(RFS_EDEADLK);
	case EDESTADDRREQ:  return htonl(RFS_EDESTADDRREQ);
	case EDOM:          return htonl(RFS_EDOM);
	case EDQUOT:        return htonl(RFS_EDQUOT);
	case EEXIST:        return htonl(RFS_EEXIST);
	case EFAULT:        return htonl(RFS_EFAULT);
	case EFBIG:         return htonl(RFS_EFBIG);
	case EHOSTUNREACH:  return htonl(RFS_EHOSTUNREACH);
	case EIDRM:         return htonl(RFS_EIDRM);
	case EILSEQ:        return htonl(RFS_EILSEQ);
	case EINPROGRESS:   return htonl(RFS_EINPROGRESS);
	case EINTR:         return htonl(RFS_EINTR);
	case EINVAL:        return htonl(RFS_EINVAL);
	case EIO:           return htonl(RFS_EIO);
	case EISCONN:       return htonl(RFS_EISCONN);
	case EISDIR:        return htonl(RFS_EISDIR);
	case ELOOP:         return htonl(RFS_ELOOP);
	case EMFILE:        return htonl(RFS_EMFILE);
	case EMLINK:        return htonl(RFS_EMLINK);
	case EMSGSIZE:      return htonl(RFS_EMSGSIZE);
	case EMULTIHOP:     return htonl(RFS_EMULTIHOP);
	case ENAMETOOLONG:  return htonl(RFS_ENAMETOOLONG);
	case ENETDOWN:      return htonl(RFS_ENETDOWN);
	case ENETRESET:     return htonl(RFS_ENETRESET);
	case ENETUNREACH:   return htonl(RFS_ENETUNREACH);
	case ENFILE:        return htonl(RFS_ENFILE);
	case ENOBUFS:       return htonl(RFS_ENOBUFS);
#if defined ENODATA
	case ENODATA:       return htonl(RFS_ENODATA);
#endif
	case ENODEV:        return htonl(RFS_ENODEV);
	case ENOENT:        return htonl(RFS_ENOENT);
	case ENOEXEC:       return htonl(RFS_ENOEXEC);
	case ENOLCK:        return htonl(RFS_ENOLCK);
	case ENOLINK:       return htonl(RFS_ENOLINK);
	case ENOMEM:        return htonl(RFS_ENOMEM);
	case ENOMSG:        return htonl(RFS_ENOMSG);
	case ENOPROTOOPT:   return htonl(RFS_ENOPROTOOPT);
	case ENOSPC:        return htonl(RFS_ENOSPC);
#if defined ENOSR
	case ENOSR:         return htonl(RFS_ENOSR);
#endif
#if defined ENOSTR
	case ENOSTR:        return htonl(RFS_ENOSTR);
#endif
	case ENOSYS:        return htonl(RFS_ENOSYS);
	case ENOTCONN:      return htonl(RFS_ENOTCONN);
	case ENOTDIR:       return htonl(RFS_ENOTDIR);
	case ENOTEMPTY:     return htonl(RFS_ENOTEMPTY);
	case ENOTSOCK:      return htonl(RFS_ENOTSOCK);
	case ENOTSUP:       return htonl(RFS_ENOTSUP);
#if EOPNOTSUPP != ENOTSUP
	case EOPNOTSUPP:    return htonl(RFS_EOPNOTSUPP);
#endif
	case ENOTTY:        return htonl(RFS_ENOTTY);
	case ENXIO:         return htonl(RFS_ENXIO);
	case EOVERFLOW:     return htonl(RFS_EOVERFLOW);
	case EPERM:         return htonl(RFS_EPERM);
	case EPIPE:         return htonl(RFS_EPIPE);
	case EPROTO:        return htonl(RFS_EPROTO);
	case EPROTONOSUPPORT: return htonl(RFS_EPROTONOSUPPORT);
	case EPROTOTYPE:    return htonl(RFS_EPROTOTYPE);
	case ERANGE:        return htonl(RFS_ERANGE);
	case EROFS:         return htonl(RFS_EROFS);
	case ESPIPE:        return htonl(RFS_ESPIPE);
	case ESRCH:         return htonl(RFS_ESRCH);
	case ESTALE:        return htonl(RFS_ESTALE);
#if defined ETIME
	case ETIME:         return htonl(RFS_ETIME);
#endif
#if defined ETIMEDOUT
	case ETIMEDOUT:     return htonl(RFS_ETIMEDOUT);
#endif
	case ETXTBSY:       return htonl(RFS_ETXTBSY);
	case EXDEV:         return htonl(RFS_EXDEV);
	
	default:            return htonl(RFS_EIO);
	}
}

int ntoh_errno(int net_errno)
{
	switch (ntohl(net_errno))
	{
	case RFS_SUCCESS:       return 0;
	
	case RFS_E2BIG:         return E2BIG;
	case RFS_EACCES:        return EACCES;
	case RFS_EADDRINUSE:    return EADDRINUSE;
	case RFS_EADDRNOTAVAIL: return EADDRNOTAVAIL;
	case RFS_EAFNOSUPPORT:  return EAFNOSUPPORT;
	case RFS_EAGAIN:        return EAGAIN;
	case RFS_EALREADY:      return EALREADY;
	case RFS_EBADF:         return EBADF;
	case RFS_EBADMSG:       return EBADMSG;
	case RFS_EBUSY:         return EBUSY;
	case RFS_ECANCELED:     return ECANCELED;
	case RFS_ECHILD:        return ECHILD;
	case RFS_ECONNABORTED:  return ECONNABORTED;
	case RFS_ECONNREFUSED:  return ECONNREFUSED;
	case RFS_ECONNRESET:    return ECONNRESET;
	case RFS_EDEADLK:       return EDEADLK;
	case RFS_EDESTADDRREQ:  return EDESTADDRREQ;
	case RFS_EDOM:          return EDOM;
	case RFS_EDQUOT:        return EDQUOT;
	case RFS_EEXIST:        return EEXIST;
	case RFS_EFAULT:        return EFAULT;
	case RFS_EFBIG:         return EFBIG;
	case RFS_EHOSTUNREACH:  return EHOSTUNREACH;
	case RFS_EIDRM:         return EIDRM;
	case RFS_EILSEQ:        return EILSEQ;
	case RFS_EINPROGRESS:   return EINPROGRESS;
	case RFS_EINTR:         return EINTR;
	case RFS_EINVAL:        return EINVAL;
	case RFS_EIO:           return EIO;
	case RFS_EISCONN:       return EISCONN;
	case RFS_EISDIR:        return EISDIR;
	case RFS_ELOOP:         return ELOOP;
	case RFS_EMFILE:        return EMFILE;
	case RFS_EMLINK:        return EMLINK;
	case RFS_EMSGSIZE:      return EMSGSIZE;
	case RFS_EMULTIHOP:     return EMULTIHOP;
	case RFS_ENAMETOOLONG:  return ENAMETOOLONG;
	case RFS_ENETDOWN:      return ENETDOWN;
	case RFS_ENETRESET:     return ENETRESET;
	case RFS_ENETUNREACH:   return ENETUNREACH;
	case RFS_ENFILE:        return ENFILE;
	case RFS_ENOBUFS:       return ENOBUFS;
#if defined ENODATA
	case RFS_ENODATA:       return ENODATA;
#endif
	case RFS_ENODEV:        return ENODEV;
	case RFS_ENOENT:        return ENOENT;
	case RFS_ENOEXEC:       return ENOEXEC;
	case RFS_ENOLCK:        return ENOLCK;
	case RFS_ENOLINK:       return ENOLINK;
	case RFS_ENOMEM:        return ENOMEM;
	case RFS_ENOMSG:        return ENOMSG;
	case RFS_ENOPROTOOPT:   return ENOPROTOOPT;
	case RFS_ENOSPC:        return ENOSPC;
#if defined ENOSR
	case RFS_ENOSR:         return ENOSR;
#endif
#if defined ENOSTR
	case RFS_ENOSTR:        return ENOSTR;
#endif
	case RFS_ENOSYS:        return ENOSYS;
	case RFS_ENOTCONN:      return ENOTCONN;
	case RFS_ENOTDIR:       return ENOTDIR;
	case RFS_ENOTEMPTY:     return ENOTEMPTY;
	case RFS_ENOTSOCK:      return ENOTSOCK;
	case RFS_ENOTSUP:       return ENOTSUP;
	case RFS_ENOTTY:        return ENOTTY;
	case RFS_ENXIO:         return ENXIO;
	case RFS_EOVERFLOW:     return EOVERFLOW;
	case RFS_EPERM:         return EPERM;
	case RFS_EPIPE:         return EPIPE;
	case RFS_EPROTO:        return EPROTO;
	case RFS_EPROTONOSUPPORT: return EPROTONOSUPPORT;
	case RFS_EPROTOTYPE:    return EPROTOTYPE;
	case RFS_ERANGE:        return ERANGE;
	case RFS_EROFS:         return EROFS;
	case RFS_ESPIPE:        return ESPIPE;
	case RFS_ESRCH:         return ESRCH;
	case RFS_ESTALE:        return ESTALE;
#if defined ETIME
	case RFS_ETIME:         return ETIME;
#endif
#if defined ETIMEDOUT
	case RFS_ETIMEDOUT:     return ETIMEDOUT;
#endif
	case RFS_ETXTBSY:       return ETXTBSY;
	case RFS_EWOULDBLOCK:   return EWOULDBLOCK;
	case RFS_EXDEV:         return EXDEV;
	
	default:                return EIO;
	}
}
