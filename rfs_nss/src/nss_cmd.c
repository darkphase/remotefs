/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "nss_cmd.h"

const char* describe_nss_command(enum nss_commands cmd)
{
	switch (cmd)
	{
		case cmd_inc:         return "inc";
		case cmd_dec:         return "dec";
		case cmd_addserver:   return "addserver";
		case cmd_adduser:     return "adduser";
		case cmd_addgroup:    return "addgroup";
		
		case cmd_getpwnam:    return "getpwnam";
		case cmd_getpwuid:    return "getpwuid";
		case cmd_getgrnam:    return "getgrnam";
		case cmd_getgrgid:    return "getgrgid";
		case cmd_getpwent:    return "getpwent";
		case cmd_setpwent:    return "setpwent";
		case cmd_endpwent:    return "endpwent";
		case cmd_getgrent:    return "getgrent";
		case cmd_setgrent:    return "setgrent";
		case cmd_endgrent:    return "endgrent";

		default:              return "unknown";
	}
}

