/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSD_H
#define RFSD_H

#include "instance_server.h"

/** server interface, basically for signals */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

DECLARE_RFSD_INSTANCE(rfsd_instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // RFSD_H
