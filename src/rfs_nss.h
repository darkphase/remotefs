/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if ! defined _RFS_NSS_H_

/* commom struct and definition for the library and the server */
#define _NSS_RFD_SOCK_H_

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif


typedef enum cmd_e
{
   GETPWNAM = 1,
   GETPWUID,
   GETGRNAM,
   GETGRGID,
   INC_CONN,
   DEC_CONN,
   CHECK_SERVER
} cmd_e;

/* For Darwin from <sys/param.h> MAXLOGNAME	255
 * <limits.h>
 * #if !defined(_ANSI_SOURCE)    _POSIX_LOGIN_NAME_MAX   9
 * 
 * For Linux   <limits.h> LOGIN_NAME_MAX 256
 *                              _POSIX_LOGIN_NAME_MAX	9
 * For Solaris <limits.h> LOGNAME_MAX 8
 *
 * For FreeBsd <sys/param.h> MAXLOGNAME 17 (16 + '\0')
 *                              _POSIX_LOGIN_NAME_MAX 9
 + According to these differemt values we support only
 * login name with 16 characters and hope that there is
 * no problem on openSolaris
 */
 
#define RFS_LOGIN_NAME_MAX 16

typedef struct cmd_s
{
   cmd_e cmd;
   int found;
   uid_t id;
   char  name[RFS_LOGIN_NAME_MAX+1];
} cmd_t;

#define   RFS_NSS_OK         0
#define   RFS_NSS_SYS_ERROR  1
#define   RFS_NSS_NO_SERVER  2

#define SOCKNAME "/tmp/rfs_sock"

#define PIDFILE "/var/run/rfs_nss.pid"

int control_rfs_nss(int cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif
