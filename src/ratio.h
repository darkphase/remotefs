/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RATIO_H
#define RATIO_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** calculate hits ratio in % */
static float ratio(unsigned long hits, unsigned long misses)
{
	do
	{
		unsigned long sum = hits + misses;
		if (sum == 0) { return 0; }
		if (misses == 0) { return hits > 0 ? 100 : 0; }
		return (float)hits / sum;
	}
	while (0);
}

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RATIO_H */
