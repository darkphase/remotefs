/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_CMD_H
#define RFSNSS_CMD_H

/** rfs_nss commands */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum nss_commands 
{
	cmd_first = 0,

	cmd_inc, 
	cmd_dec, 
	cmd_addserver, 
	cmd_adduser, 
	cmd_addgroup, 

	/* reserved */

	cmd_getpwnam = 50, 
	cmd_getpwuid, 
	cmd_getgrnam, 
	cmd_getgrgid, 
	cmd_getpwent, 
	cmd_setpwent, 
	cmd_endpwent, 
	cmd_getgrent, 
	cmd_setgrent, 
	cmd_endgrent, 

	cmd_last
};

struct nss_command
{
	uint16_t command;
	uint32_t data_len;
};

struct nss_answer
{
	uint16_t command;
	uint32_t data_len;
	int32_t  ret;
	uint32_t ret_errno;
};

const char* describe_nss_command(enum nss_commands cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_CMD_H */

