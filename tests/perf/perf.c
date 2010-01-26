#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "getopts.h"

#define PERF_SUCCESS 1
#define PERF_ERROR   0

/* variables set from command line */
static int   csv = 0;
static int   maxFile = 1;
static char *testDir = NULL;
static char *remount = NULL;
static off_t totalSize = 0;
static off_t minSize = 0;
static off_t maxSize = 1;
static off_t stepSize = 0;
static int   stepType = 0;
static int   numberOp = 1;
static int   rwBufSize = 4096;
static int   createFileBefore = 1;
static int   printCreateTime = 0;
static int   doSleep = 0;

#ifdef HAS_CLOCK_GETTIME
static struct timespec start;
static struct timespec end;
double endd=1.0;
double startd=1.0;
double dt;
#define STARTTIME() clock_gettime(CLOCK_REALTIME, &start);
#define ENDTIME()   clock_gettime(CLOCK_REALTIME, &end);
#define SETDT()     startd = (double)start.tv_sec + (double)start.tv_nsec/1000000000.0;\
                    endd   = (double)end.tv_sec   + (double)end.tv_nsec/1000000000.0;\
                    dt     = (endd - startd)*1000000.0;
#else
static struct timeval start;
static struct timeval end;
static long dt;
#define STARTTIME() gettimeofday(&start, NULL);
#define ENDTIME()   gettimeofday(&end, NULL);
#define SETDT()     dt = (end.tv_sec  - start.tv_sec) * 1000000;\
                    dt += end.tv_usec - start.tv_usec;
#endif

/* some little functions for the various tests */
int setFileSize(char *file, off_t size)
{
   return truncate(file, size);
}

int createDir(char *dir)
{
   return mkdir(dir, 0777);
}

int deleteDir(char *dir)
{
   return rmdir(dir);
}

/***************************************************
 * createFile()
 *
 * Rerturn 1 on success, 0 on error
 *
 **************************************************/
 
int createFile(char *file)
{
   int fd;
   errno = 0;
#if defined SOLARIS || defined DARWIN || defined FREEBSD
   fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
   if ( fd > -1 ) close(fd);
   else fprintf(stderr, "createFile %s: %s\n", file, strerror(errno));
   return fd == -1 ? 0 : 1;
#else
   fd = mknod(file, S_IFREG|0666,0);
   if (fd) fprintf(stderr, "createFile %s: %s\n", file, strerror(errno));
   return fd ? 0 : 1;
#endif
}

/***************************************************
 * deleteFile()
 *
 * Rerturn 1 on success, 0 on error
 *
 **************************************************/
 
int deleteFile(char *file)
{
   errno = 0;
   int ret = unlink(file);
   if (ret < 0 )
      fprintf(stderr,"unlink(%s): %s\n",file, strerror(errno));
   return ret < 0 ? 0 : 1;
}

/***************************************************
 * createRandomFile()
 *
 * Rerturn 1 on success, 0 on error
 *
 **************************************************/
 
int createRandomFile(char *name)
{
   char randBuf[4096];
   ssize_t wr = 0;
   ssize_t nb = 0;
   int i;
   FILE *fp;
   /* fill a buffer with random values */
   srand((int)time(NULL));
   for (i=0; i<4096;i++)
   {
       randBuf[i] = (unsigned char)rand();
   }
   if ((fp = fopen(name,"w")) == NULL)
   {
      fprintf(stderr,"Failed to create %s: %s\n",name,strerror(errno));
      return 0;
   }
   while(wr < maxSize)
   {
      nb = maxSize - wr > sizeof(randBuf)? 4096: maxSize - wr;
      if ( nb == -1 )
      {
         fprintf(stderr,"file %s: %s\n",name,strerror(errno));
         fclose(fp);
         return 0;
      }
      i = fwrite(randBuf, 1,nb, fp);
      wr += i;
   }
   fclose(fp);
   return 1;
}

/***************************************************
 * copyFile()
 *
 * Rerturn 1 on success, 0 on error
 *
 **************************************************/
 
int copyFile(char *src, char *tgt)
{
   FILE *srcFp;
   FILE *tgtFp;
   char *rwBuf = malloc(rwBufSize);
   char buffer[4096];
   size_t nb;
   size_t wr;

   if ( rwBuf == NULL )
   {
      fprintf(stderr,"No more memory\n");
      return 0;
   }

   errno = 0;
   if ( (srcFp = fopen(src,"r")) == NULL )
   {
      fprintf(stderr,"copyFile open src %s: %s\n",src,strerror(errno));
      return 0;
   }

   if ( (tgtFp = fopen(tgt,"w")) == NULL )
   {
      fprintf(stderr,"copyFile open tgt %s: %s\n",tgt,strerror(errno));
      fclose(srcFp);
       return 0;
   }
   
   errno = 0;
   while((nb=fread(rwBuf, 1, rwBufSize,srcFp))>0)
   {
      if ( nb < 0 )
      {
         fprintf(stderr,"Read error %s\n",strerror(errno));
         break;
      }
      if ( nb > 0 )
         wr = fwrite(rwBuf, 1, nb,tgtFp);
      if ( wr != nb )
         fprintf(stderr,"fwrite() wrote %d bytes instead of %d %s\n",
                 wr, nb, strerror(errno));
      errno = 0;
   }

   fclose(tgtFp);
   fclose(srcFp);
   free(rwBuf);

   return 1;
}

/***************************************************
 * copyNFile()
 *
 * Rerturn 1 on success, 0 on error
 *
 **************************************************/

int copyNFile(char *src, int ssrc, char *tgt, int stgt, int nb)
{

   FILE **srcFp = (FILE**)calloc(sizeof(FILE*),nb);
   FILE **tgtFp = (FILE**)calloc(sizeof(FILE*),nb);
   int  *feof    = (int*)calloc(sizeof(int),nb);
   char buffer[4096];
   char *rwBuf = malloc(rwBufSize);
   int  i;
   int  ret = 0;
   int  eof = 0;

   if ( srcFp == NULL || tgtFp == NULL || feof == NULL || rwBuf == NULL )
   {
      fprintf(stderr,"No more memory\n");
      return 0;
   }

   for (i = 0; i < nb; i++)
   {
      if (    (ssrc != nb && (stgt + i) < maxFile)
           || (stgt != nb && (ssrc + i) < maxFile)
         )
      {
          snprintf(buffer,sizeof(buffer),"%s%d",src, ssrc+i);
          if ( (srcFp[i] = fopen(buffer,"r")) == NULL )
          {
             fprintf(stderr,"%s open failed error %s\n",buffer,strerror(errno));
             feof[i] = 1;
             eof++;
             ret = 0;
             goto cleanCpNfile;
          }
          snprintf(buffer,sizeof(buffer),"%s%d",tgt, stgt+i);
          if ( (tgtFp[i] = fopen(buffer,"w")) == NULL )
          {
             fprintf(stderr,"%s open failed error %s\n",buffer,strerror(errno));
             ret = 0;
             goto cleanCpNfile;
          }
      }
      else
      {
         feof[i] = 1;
         eof++;
      }
   }

   while (eof != nb)
   {
       for(i=0;i<nb;i++)
       {
          if ( ! feof[i] && srcFp[i] )
          {
             ret = fread(rwBuf, 1, rwBufSize,srcFp[i]);
             if ( ret == -1 )
             {
                fprintf(stderr,"Read failed error %s\n",strerror(errno));
                ret = 0;
                goto cleanCpNfile;
             }
             if ( ret < rwBufSize )
             {
                eof++;
                feof[i] = 1;
             }
             if ( ret > 0 )
             {
                fwrite(rwBuf, 1, ret,tgtFp[i]);
             }
          }
       }
   }
   ret = 1;

cleanCpNfile:
   for ( i = 0; i < nb; i++ )
   {
      if (tgtFp[i] ) fclose(tgtFp[i]);
      if (srcFp[i] )  fclose(srcFp[i]);
   }
   free(srcFp);
   free(tgtFp);
   free(feof);
   free(rwBuf);

   return ret;
}

/* helper functions for output and parsing */
char *sizeToName(off_t sz)
{
   static char buffer[1024];
   *buffer = '\0';

   if ( sz >= (1024*1024*1024) )
   {
      snprintf(buffer,sizeof(buffer),"%04.1fH",
               (double)sz/(1024.0*1024.0*1024.0));
   }
   else if ( sz >= (1024*1024) )
   {
      snprintf(buffer,sizeof(buffer),"%04.1fM",
               (double)sz/(1024.0*1024.0));
   }
   else
   {
      snprintf(buffer,sizeof(buffer),"%04.1fK",
               (double)sz/1024.0);
   }
   return buffer;
}


char *specToName(off_t sz, int nb, int spec)
{
   static char buffer[1024];
   *buffer = '\0';

   if ( spec )
   {
      if ( sz >= (1024*1024*1024) )
      {
         snprintf(buffer,sizeof(buffer),"%.1fG",
                  (double)sz/(1024.0*1024.0*1024.0));
      }
      else if ( sz >= (1024*1024) )
      {
         snprintf(buffer,sizeof(buffer),"%.1fM",
                  (double)sz/(1024.0*1024.0));
      }
      else
      {
         snprintf(buffer,sizeof(buffer),"%.1fK",
                  (double)sz/1024.0);
      }
   }
   else
   {
      if ( sz >= (1024*1024*1024) )
      {
         snprintf(buffer,sizeof(buffer),"%d files %.1fG",
                  nb,
                  (double)sz/(1024.0*1024.0*1024.0));
      }
      else if ( sz >= (1024*1024) )
      {
         snprintf(buffer,sizeof(buffer),"%d files %.1fM",
                  nb,
                  (double)sz/(1024.0*1024.0));
      }
      else
      {
         snprintf(buffer,sizeof(buffer),"%d files %.1fK",
                  nb,
                  (double)sz/1024.0);
      }
   }
   return buffer;
}

int deleteTestDir(int delDir)
{
   char buf[4096];
  /* cleanup */
  errno = 0;
   DIR *dir = opendir(testDir);
   if ( dir != NULL )
   {
      struct dirent *ent;
      while(ent=readdir(dir))
      {
         if ( strcmp(ent->d_name,".") == 0 ||
              strcmp(ent->d_name,"..") == 0
            )
         {
            continue;
         }
         else
         {
            snprintf(buf,sizeof(buf),"%s/%s",testDir,ent->d_name);
            deleteFile(buf);
         }
      }
      closedir(dir);
   }
   else
   {
      fprintf(stderr,"opendir %s\n",strerror(errno));
   }
   if ( delDir)
   {
      deleteDir(testDir);
   }
   return 0;
}

int deleteSourceFiles()
{
   DIR *dir = opendir(testDir);
   if ( dir != NULL )
   {
      struct dirent *ent;
      while(ent=readdir(dir))
      {
         if ( strncmp(ent->d_name,"sourcefile", 10) == 0 )
         {
            deleteFile(ent->d_name);
         }
      }
      closedir(dir);
   }
   return 1;
}


/* ******************************************************
Test for remotefs perfoemance, mainly copying file from
to server.

In order to get correct times we need to do the single
operations more times.
Parameters

testDir: mnt/testdir # -d
maxFiles: 1024       # -f
minSize: 512         # -m
#maxSize: 256M       # -M
maxSize: 64k
sizeStep: 0.5k       # -s
totalSize: 256M      # -t
#sizeStep: 2x

****************************************************** */

off_t stoi(char *opt)
{
   off_t sz = 0;
   int dot = 0;
   int post = 1;
   
   while(*opt)
   {
      if(isdigit((int)*opt))
      {
         sz = (sz * 10) + (*opt-'0');
         if (dot)
             post *= 10;
      }
      else if ( *opt == 'k' || *opt == 'K' )
      {
         sz *= 1024;
         break;
      }
      else if ( *opt == 'm' || *opt == 'M' )
      {
         sz *= (1024*1024);
         break;
      }
      else if ( *opt == 'g' || *opt == 'G' )
      {
         sz *= (1024*1024*1024);
         break;
      }
      else if ( *opt == '.')
      {
         dot = 1;
      }
      opt++;
   }

   sz /= post;

   return sz;
}

off_t sstoi(char *opt, int *stepType)
{
   off_t sz = 0;
   int dot = 0;
   int post = 1;
   
   while(*opt)
   {
      if(isdigit((int)*opt))
      {
         sz = (sz * 10) + (*opt-'0');
         if (dot)
             post *= 10;
      }
      else if ( *opt == 'k' || *opt == 'K' )
      {
         sz *= 1024;
         break;
      }
      else if ( *opt == 'm' || *opt == 'M' )
      {
         sz *= (1024*1024);
         break;
      }
      else if ( *opt == 'g' || *opt == 'G' )
      {
         sz *= (1024*1024*1024);
         break;
      }
      else if ( *opt == '.' )
      {
         dot = 1;
      }
      else if ( *opt == 'x' || *opt == 'X' )
      {
         *stepType = 'x';
      }
      else
      {
         return 0;
      }
      opt++;
   }

   sz /= post;

   return sz;
}

/* main test functions */
int create(void)
{
   char buf[4096];
   char *s;
   int i;
   int ret = 1;
   char *spec;
   if ( ret && createDir(testDir) )
      ret = 0;

   if ( totalSize < maxSize )
   {
       maxSize = totalSize;
   }
   int nb = maxSize;
   spec = specToName(0, nb, csv);

   STARTTIME();
   for(i=0;i < nb &&createFileBefore;i++)
   {
      snprintf(buf,sizeof(buf), "%s/f_%d",testDir,i);
      int fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if ( fd > 0 )
      {
         close(fd);
      }
      else
      {
         ret = 0;
         break;
      }
   }
   ENDTIME();
   SETDT();
   double createPerSecond = nb*1000000.0/dt;

   fprintf(stdout,"Create %-18s time %8.3f sec %8.0f     F/s\n",
           spec,
           (double)dt/1000000,
           createPerSecond
           );
   STARTTIME();
   for(i=0;i < nb; i++)
   {
      snprintf(buf,sizeof(buf),"%s/f_%d",testDir,i);
      deleteFile(buf);
   }
   ENDTIME();
   SETDT();
   createPerSecond = nb*1000000.0/dt;
   fprintf(stdout,"Delete %-18s time %8.3f sec %8.0f     F/s\n",
           spec,
           (double)dt/1000000,
           createPerSecond
           );
   
   deleteTestDir(1);
   return ret;
}

int copy(void)
{
   int   i;
   int   nb;
   char buf[4096];
   char *s;
   char *spec;
   int first = 1;
   off_t actSize = 0;
   int  ret = 1;
   int result;

   if ( totalSize < maxSize )
   {
       maxSize = totalSize;
   }
   
   /* generate source file on client and target dir on server */
   if (numberOp > 0 )
   {
      for (i=0; i < numberOp;i++)
      {
         snprintf(buf,sizeof(buf),"sourceFile_%d",i);
         if ( createFile(buf) == 0)
            ret = 0;
      }
   }
   else
   {
      fprintf(stderr,"Wrong argument!\n");
      return 0;
   }

   if ( ret && createDir(testDir) )
      ret = 0;

   if ( ret && csv )
   {
      printf("\"size\",\"write\",\"read\"\n");
   }

   while (actSize <= maxSize && ret )
   {
      if ( stepType == 'x' )
      {
         if ( first )
         {
            actSize = minSize;
            first = 0;
         }
         else
         {
            actSize *= stepSize;
         }
      }
      else
      {
         if ( first )
         {
            actSize = minSize;
            first = 0;
         }
         else
         {
            actSize += stepSize;
         }
      }

      if ( actSize > maxSize )
         break;

      if ( actSize > 0 )
         nb = totalSize / actSize;
      if ( nb > maxFile )
         nb = maxFile;
      if ( numberOp > 1 )
      {
         int r = nb % numberOp;
         nb -= r;
         if ( nb == 0 )
         {
            break;
         }
      }

      /* set size of source file(s) */
      for (i=0; i < numberOp;i++)
      {
         snprintf(buf,sizeof(buf),"sourceFile_%d",i);
         setFileSize(buf,actSize);
      }


      s = sizeToName(actSize);
      spec = specToName(actSize, nb, csv);

      /* create all target file */
      STARTTIME();
      for(i=0;i < nb &&createFileBefore;i++)
      {
         snprintf(buf,sizeof(buf), "%s/%s_%d",testDir,s,i);
         if ( createFile(buf) == 0 )
         {
            ret = 0;
            break;
         }
      }

      if ( printCreateTime && createFileBefore)
      {
         ENDTIME();
         SETDT();
         int createPerSecond = nb*1000000/dt;
         fprintf(stdout,"Make  %-18s time %8.3f sec %8d     F/s\n",
                 spec,
                 (double)dt/1000000,
                 createPerSecond
                 );
      }
      if ( doSleep && createFileBefore)
      {
         sleep(doSleep);
      }
      
      /*************************/
      /* copy source to target */
      /*************************/
      STARTTIME();
      for ( i=0;i < nb ; i += numberOp )
      {
         if ( numberOp == 1 )
         {
            snprintf(buf,sizeof(buf), "%s/%s_%d",testDir,s,i);
            result = copyFile("sourceFile_0", buf);
            
         }
         else
         {
            snprintf(buf,sizeof(buf), "%s/%s_",testDir,s);
            result = copyNFile("sourceFile_", 0, buf, i, numberOp<nb?numberOp:nb);
         }
         if ( result == 0 )
         {
            ret = 0;
            break;
         }
      }
      ENDTIME();
      SETDT();

      /* end of copy print out results */
      if ( ! csv )
         fprintf(stdout,"Write %-18s time %8.3f sec %12.3f MBps\n",
                 spec,
                 (double)dt/1000000.0,
                 (double)(nb*actSize)/((double)dt/1000000.0)/(1024.0*1024.0)
                 );
      else
          fprintf(stdout,"\"%s\",\"%.3f\",",
                 spec,
                 (double)(nb*actSize)/((double)dt/1000000.0)/(1024.0*1024.0)
                 );

      /*******************/
      /* check file size */
      /*******************/
      for( i=0;i < nb; i++)
      {
         struct stat st;
         snprintf(buf,sizeof(buf), "%s/%s_%d", testDir,s, i);
         if (stat(buf,&st) == -1)
         {
            fprintf(stderr,"File %s not found\n",buf);
            ret = 0;
         }
         else
         {
            if ( st.st_size != actSize )
            {
               fprintf(stderr,"Wrong size (%llu) for file %s\n",st.st_size, buf);
               ret = 0;
            }
         }
      }
      
      if ( remount && ret )
      {
         system(remount);
      }

      /*************************/
      /* copy target to source */
      /*************************/
      STARTTIME();
      for(i=0;i < nb;i += numberOp)
      {
         if ( numberOp == 1 )
         {
            snprintf(buf,sizeof(buf), "%s/%s_%d",testDir,s,i);
            result = copyFile(buf, "sourceFile_0");
         }
         else
         {
            snprintf(buf,sizeof(buf), "%s/%s_",testDir,s);
            result = copyNFile(buf, i, "sourceFile_", 0, numberOp<nb?numberOp:nb);
         }
         if ( result == 0 )
         {
            ret = 0;
            break;
         }
      }
      ENDTIME();

      /*******************/
      /* check file size */
      /*******************/
      for( i=0;i < numberOp; i++)
      {
         struct stat st;
         snprintf(buf,sizeof(buf), "sourceFile_%d", i);
         if (stat(buf,&st) == -1)
         {
            fprintf(stderr,"File %s not found\n",buf);
            ret = 0;
         }
         else
         {
            if ( st.st_size != actSize )
            {
               fprintf(stderr,"Wrong size (%llu) for file %s\n",st.st_size, buf);
               ret = 0;
            }
         }
      }

      /* print out results */
      SETDT();
      if ( ! csv )
         fprintf(stdout,"Read  %-18s time %8.3f sec %12.3f MBps\n",
                 spec,
                 (double)dt/1000000,
                 (double)(nb*actSize)/((double)dt/1000000)/(1024.0*1024.0)
                 );
      else
          fprintf(stdout,"\"%.3f\"\n",
                 (double)(nb*actSize)/((double)dt/1000000)/(1024.0*1024.0)
                 );
      /* remove files from test directory */
      deleteTestDir(0);
   }

   /* cleanup */
   for (i=0; i < numberOp;i++)
   {
      snprintf(buf,sizeof(buf),"sourceFile_%d",i);
      deleteFile(buf);
   }
   deleteTestDir(1);

   return ret;
}

int list()
{
   int i = 0;
   int nb = 0;
   int max = 0;
   char buf[4096];

   createDir(testDir);
   if ( stepSize > 0 )
      nb = minSize;

   if ( csv )
      fprintf(stdout,"\"files\",\"first call\",\"second call\",\n");

   while (nb <= maxSize)
   {
      for (; i < nb;i++)
      {
         for(i=0;i < nb;i++)
         {
            snprintf(buf,sizeof(buf), "%s/f_%d",testDir,i);
            createFile(buf);
            /*truncate(buf, i);*/
         }
      }

      /* remount */
      if ( remount )
      {
         system(remount);
      }

      /* get attributes for all files */
      STARTTIME();
      DIR *dir = opendir(testDir);
      if ( dir != NULL )
      {
         struct dirent *ent;
         while(ent=readdir(dir))
         {
            struct stat st;
            snprintf(buf, sizeof(buf),"%s/%s",
                     testDir,ent->d_name);
            stat(buf,&st);
         }
         closedir(dir);
      }
      ENDTIME();
      SETDT();
      if (csv)
         fprintf(stdout,"\"%d,%.6f\"",
                 nb,
                 (double)dt/1000000
                 );
      else
         fprintf(stdout,"List %18d files time %8.3f sec first call\n",
                 nb,
                 (double)dt/1000000
                 );

      /* get attributes for all files */
      STARTTIME();

      dir = opendir(testDir);
      if ( dir != NULL )
      {
         struct dirent *ent;
         while(ent=readdir(dir))
         {
            struct stat st;
            snprintf(buf, sizeof(buf),"%s/%s",
                     testDir,ent->d_name);
            stat(buf,&st);
         }
         closedir(dir);
      }
      ENDTIME();
      SETDT();
      if (csv)
         fprintf(stdout,"\"%.6f\"\n",
                 (double)dt/1000000
                 );
      else
         fprintf(stdout,"List %18d files time %8.3f sec second call\n",
                 nb,
                 (double)dt/1000000
                 );
       nb += stepSize;
   }
   /* cleanup */
   deleteTestDir(1);
   return 0;
}

/***************************************************************
 * cmp()
 *
 * generate a random file and copy ot to the given directory
 * and read it back. If the data are OK return success (1)
 * else return error (0)
 *
 **************************************************************/
int cmp(void)
{
   /* generate a random file,
     * copy it th the server
     * compare the files on server and client
     */
   #define BSIZE 4096
   FILE *s = NULL;
   FILE *t = NULL;
   char nameBuf[BSIZE];
   char buffer[2][BSIZE];
   int   sr = 0;
   int   tr = 0;
   int   result = 1; /* assume result is OK */

   createRandomFile("sourceFile");
   createDir(testDir);
   snprintf(nameBuf,sizeof(buffer),"%s/testFile",testDir);
   copyFile("sourceFile", nameBuf);

   /* read both file and compare them */
   if ((s = fopen("sourceFile","r")) == NULL)
   {
      fprintf(stderr,"open %s: %s\n","sourceFile", strerror(errno));
      result = 0;
      goto clean;
   }
   if ((t = fopen(nameBuf,"r")) == NULL)
   {
      fprintf(stderr,"open %s: %s\n",nameBuf, strerror(errno));
      fclose(t);
      result = 0;
      goto clean;
   }

   do
   {
      sr = fread(buffer[0], 1, BSIZE, s);
      tr = fread(buffer[1], 1, BSIZE, t);
      if ( sr != tr )
      {
         fprintf(stderr,"files size different\n");
         result = 0;
         break;;
      }
      if ( sr == -1 || tr == -1 )
      {
         fprintf(stderr,"read error %s\n",strerror(errno));
         result = 0;
         break;
      }
      if ( memcmp(buffer[0],buffer[1],sr) )
      {
         fprintf(stderr,"files different\n");
         result = 0;
         break;;
      }
      
   } while (sr > 0 && tr > 0);
 
clean:
   if ( s ) fclose(s);
   if ( t ) fclose(t);
   /* cleanup */
   deleteTestDir(1);
   unlink("sourceFile");
   if ( result )
   {
      printf("Success\n");
   }
   return result;
}


/* ==================================================================== */
void syntax()
{
   fprintf(stderr,"Syntax: perf test args\n");
   fprintf(stderr,"   -d test_directory *               must not exist\n");
   fprintf(stderr,"   -f max_file_number \n");
   fprintf(stderr,"   -n number_of_parallel_operations \n");
   fprintf(stderr,"   -t max_total_size_to_be copied \n");
   fprintf(stderr,"   -m size_of_smallest_file \n");
   fprintf(stderr,"   -M size_of_biggest_file *\n");
   fprintf(stderr,"   -s increment_for_file_size \n");
   fprintf(stderr,"   -b buffer_size_for_copy           default 4k\n");
   fprintf(stderr,"   -c                                csv output\n");
   fprintf(stderr,"   -l                                repeat the command until perf is killed\n");
   fprintf(stderr,"   -r remount_command                command to unmount/mount\n");
   fprintf(stderr,"   -L count                          repeat test count times\n");
   fprintf(stderr,"   -l                                repeat infinitelly \n");
   fprintf(stderr,"\n");
   fprintf(stderr,"   size may be for example 412 1K 3M and so on\n");
   fprintf(stderr,"      eg 412 Bytes / 1 KByte / 3 MegaBytes\n");
   fprintf(stderr,"   incrememt can be a size (1024) or a factor (2x)\n");
   fprintf(stderr,"   The -m, -s and -M parameters may also be used as\n");
   fprintf(stderr,"   number of files (list test)\n");
   fprintf(stderr,"\n");
   fprintf(stderr,"   args marked with * are mandatory\n");
   fprintf(stderr,"\n");
   fprintf(stderr,"   The following tests are available:\n");
   fprintf(stderr,"      copy\n");
   fprintf(stderr,"      list\n");
   fprintf(stderr,"      cmp (only integrity test)\n");
   fprintf(stderr,"      create /timing for creating and deleting files(\n");
   exit(1);
}

typedef int (*func_t)(void);
typedef struct tests_s
{
   char *name;
   int (*func)(void);
} tests_t;

tests_t tests[] =
{
   { "copy", copy },
   { "list", list },
   { "cmp",  cmp },
   { "create",  create },
   { NULL,   NULL }
};


func_t checkforTest(char *s)
{
   tests_t *test = tests;
   
   if ( s == NULL )
   {
      return NULL;
   }
   while(test->name)
   {
      if ( strcmp(s,test->name)==0)
      {
         return test->func;
      }
      test++;
   }
   return NULL;
}

int killed = 0;
void clean(int code)
{
   killed = 1;
}

int main(int argc, char **argv)
{
   char *opt;
   int   idx  = 0;
   char buf[4096];
   int debug = 0;
   int loop = 0;
   int result;
   int loops = 0;
   argc--;
   argv++;
   func_t func;

   if ((func=checkforTest(argv[0])) == NULL)
   {
      syntax();
   }
   argc--;
   argv++;

   signal(SIGTERM, clean);
   signal(SIGQUIT, clean);
   signal(SIGINT, clean);

   while(idx < argc)
   {
      switch(getOpts("d:f:m:M:s:t:cr:n:b:DNPS:lL:", argc, argv, &opt, &idx))
      {
         case 'd': if ( opt) testDir = opt; else syntax(); break;
         case 'f': if ( opt) maxFile = atoi(opt); else syntax(); break;
         case 'n': if ( opt) numberOp = atoi(opt); else syntax(); break;
         case 't': if ( opt) totalSize = stoi(opt); else syntax(); break;
         case 'm': if ( opt) minSize = stoi(opt); else syntax();break;
         case 'M': if ( opt) maxSize = stoi(opt); else syntax();break;
         case 's': if ( opt) stepSize = sstoi(opt, &stepType); else syntax(); break;
         case 'b': if ( opt) rwBufSize = stoi(opt); else syntax(); break;
         case 'c': csv = 1; break;
         case 'r': remount = opt; break;
         case 'D': debug = 1; break;
         case 'N': createFileBefore=0;break;
         case 'P': printCreateTime=1;break;
         case 'S': if ( opt) doSleep=atoi(opt); else syntax();break;
         case 'l': loop = 1;break;
         case 'L': if ( opt) loops = atoi(opt); else syntax();break;
         default: printf("Wrong option\n");syntax();
      }
   }

   if (maxFile <= 0 || testDir == NULL || maxSize <= 0 || rwBufSize < 1)
   {
      if ( debug )
      {
         fprintf(stderr,"Error checking parameters:\n");
         if ( maxFile <= 0 ) fprintf(stderr,"   maxFile = %d\n",maxFile);
         if ( testDir == NULL ) fprintf(stderr,"   testDir == NULL\n");
         if ( maxSize <= 0 ) fprintf(stderr,"   maxSize = %d\n",maxSize);
         if ( rwBufSize < 1 ) fprintf(stderr,"   rwBufSize = %d\n",rwBufSize);
      }
      syntax();
   }

   /* if test directory exist exit */
   if ( access(testDir,W_OK) == 0 )
   {
      fprintf(stderr,"Sorry <%s> exist\n", testDir);
      return 1;
   }

   if ( totalSize <= 0 )
   {
      totalSize = maxSize * maxFile;
   }

   if ( stepSize <= 0 && minSize <= 0 )
   {
      minSize = stepSize = maxSize;
   }

   if ( loop )
   {
      result = 1;
      while((result=func()))
         if ( killed ) break;
      /* at this stage we may have to make some cleanup */
      deleteTestDir(1);
      /* delete also sourcefile and sourcefile_* within the actual directory */
      deleteSourceFiles();
   }
   else if ( loops )
   {
      result = 1;
      while((result=func()) && --loops > 0)
         if ( killed )
         {
            /* at this stage we may have to make some cleanup */
            deleteTestDir(1);
            /* delete also sourcefile and sourcefile_* within the actual directory */
            deleteSourceFiles();
            break;
        }
   }
   else
   {
      result = func();
   }

   return result == 1 ? 0 : 1;
}
