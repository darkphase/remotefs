/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if ! defined _RFS_NSS_H_
#define _RFS_NSS_H_

#include <pwd.h>
#include <grp.h>
#include <stdint.h>

/* commom struct and definition for the library and the server */
#define _NSS_RFD_SOCK_H_

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif


typedef enum cmd_e
{
   INC_CONN = 1,   /* 01 */
   DEC_CONN,       /* 02 */
   CHECK_SERVER,   /* 04 */
   GETPWNAM,       /* 05 */
   GETPWUID,       /* 06 */
   GETGRNAM,       /* 07 */
   GETGRGID,       /* 08 */
   PUTPWNAM,       /* 09 */
   PUTGRNAM,       /* 0a 10 */
   GETPWENT,       /* 0b 11 */
   GETGRENT,       /* 0c 12 */
   SETPWENT,       /* 0d 13 */
   SETGRENT,       /* 0e 14 */
   ENDPWENT,       /* 0f 15 */
   ENDGRENT,       /* 10 16 */
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
 * no problem on openSolaris and furher more 256 characters for the server name
 */

#define RFS_LOGIN_NAME_MAX 272

typedef struct cmd_s
{
   int32_t cmd;
   int32_t found;
   int32_t id;
   char  name[RFS_LOGIN_NAME_MAX+1];
} cmd_t;

#define RFS_NSS_OK         0
#define RFS_NSS_SYS_ERROR  1
#define RFS_NSS_NO_SERVER  2

#define RFS_REM_PORT       5004

#define SOCKNAME "/tmp/rfs_sock"

#define PIDFILE "/var/run/rfs_nss.pid"

#define LIBRFS_NSS "librfs_nss.so"

int control_rfs_nss(const int cmd, const char *name, const char *host, uid_t *id);

uid_t rfs_putpwnam(char *name, char *host);
gid_t rfs_putgrnam(char *name, char *host);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif
