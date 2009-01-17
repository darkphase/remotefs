/****************************************************
 * File getopts.c
 *
 * Parse command line arguments
 *
 * Content: getOpts()
 *
 * Author: Jean-Jacques Sarton
 *         SIEMENS I&S IS ICS 53
 *
 ****************************************************/
#include <stdio.h>
#include "getopt.h"

/****************************************************
 * Function: getOpts()
 *
 * Parse the command line arguments
 *
 * Input:
 *   char  *optString known options in the form
 *                    "abc:ef:". If a colon ist set
 *                    after the option character
 *                    an argument is expected
 *   int    argc
 *   char **argv
 *
 * Output:
 *   char **opt      set to NULL or to the option
 *                   argument if argument required.
 *                   If the command line argument
 *                   don't begin with '-' opt point
 *                   to the actual argument
 *   int  *nxt       index for next option to be
 *                   processed by getOpts
 * Return:
 *   char option character
 *   or '\0' if options not found
 ****************************************************/

char getOpts(char *optString, int argc, char **argv, char **opt, int *nxt)
{
   static int idx = 0;
   char optChar = '\0';
   *opt         = NULL;

   if ( idx < argc )
   {
      if ( argv[idx][0] == '-' && (optChar=argv[idx][1]) )
      {
         /* search for option character */
         while(*optString)
         {
            if ( *optString == optChar )
            {
               /* option character found */
               if ( optString[1] == ':' )
               {
                  /* expect argument */
                  if ( argv[idx][2] )
                  {
                     *opt = &argv[idx][2];
                     idx += 1;
                  }
                  else
                  {
                     idx += 1;
                     *opt = argv[idx];
                     idx += 1;
                  }
                  *nxt = idx;
                  return optChar;
               }
               else
               {
                  idx += 1;
                  *nxt = idx;
                  return optChar;
               }
            }
            else
            {
               optString++;
            }
         }
         /* at this stage we will return '\0' and
          * *opt is set to NULL, this may be an error"
          */
      }
      else
      {
         /* no '-' for this argument, may be a file name or ... */
         /* the caller is reponsible for further handling, we   */
         /* will return cgaracter '\0'                          */
         *opt = argv[idx];
      }
   }
   return '\0';
}

