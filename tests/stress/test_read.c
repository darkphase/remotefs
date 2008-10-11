
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "test_read.h"
#include "file.h"

void test_read()
{
    srand(time(NULL));

    printf("creating test file..\n");
    if (create_test_file(1024 * 1024 * 10) != 0)
    {
	fprintf(stderr, "error creating test file\n");
	return;
    }
    
    printf("validating test file..\n");
    if (validate_test_file() != 0)
    {
	fprintf(stderr, "error validating test file\n");
	return;
    }
    printf("done\n");
    
    FILE *fp = fopen(test_file, "rb");
    if (fp == NULL)
    {
	fprintf(stderr, "error opening %s\n", test_file);
	return;
    }
    
    if (fseek(fp, 0, SEEK_END) != 0)
    {
	fprintf(stderr, "error seeking to the end of the file\n");
	return;
    }
    
    off_t last_pos = ftell(fp);
    if (last_pos == (off_t)-1)
    {
	fprintf(stderr, "can't get file size\n");
	return;
    }
    
    off_t buffer[1024 * 512 / sizeof(off_t)] = { 0 };
    
    int done = fread(buffer, sizeof(buffer), 1, fp);
    
    if (done > 0)
    {
	fprintf(stderr, "seems like there are data after end of the file :(\n");
	return;
    }
    
    if (done < 0)
    {
	fprintf(stderr, "read after end of the file returned error\n");
	return;
    }
    
    if (fseek(fp, last_pos + 1024, SEEK_SET) != 0)
    {
	fprintf(stderr, "seek after end of the file was not successful :(\n");
	return;
    }
    
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
	fprintf(stderr, "can't seek back to the beginning of the file\n");
	return;
    }
    
    unsigned random_seeks = 1000 * 1000;
    unsigned i = 0; for (i = 0; i < random_seeks; ++i)
    {
	off_t rand_offset = rand() % (last_pos / sizeof(*buffer)) + rand() % (last_pos / sizeof(*buffer)) / 4;
	printf("seek (%u) to %lu\n", i, rand_offset * sizeof(*buffer));
	if (fseek(fp, rand_offset * sizeof(*buffer), SEEK_SET) != 0)
	{
	    if (rand_offset * sizeof(*buffer) <= last_pos)
	    {
		fprintf(stderr, "can't seek inside of file at pos %lu\n", rand_offset * sizeof(*buffer));
		return;
	    }
	    
	    continue;
	}
	
	unsigned values_to_check = fread(buffer, sizeof(*buffer), 1024 * 512 / sizeof(*buffer), fp); /* may be 0 */
	
	int j = 0; for (j = 0; j < values_to_check; ++j)
	{
	    if (buffer[j] != rand_offset + j)
	    {
		fprintf(stderr, "wrong data at pos %lu, actual data %lu (buffer offset: %u, seek number: %u, offset: %lu)\n", 
			rand_offset + j, 
			buffer[j], 
			j, 
			i,
			rand_offset);
		return;
	    }
	}
    }
    
    if (fclose(fp) != 0)
    {
	fprintf(stderr, "error closing %s\n", test_file);
	return;
    }
    
    /* cleanup(); */
}
