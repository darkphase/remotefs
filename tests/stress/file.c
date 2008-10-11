
#include <stdio.h>
#include <unistd.h>

#include "file.h"

int create_test_file(size_t size)
{
	FILE *fp = fopen(test_file, "wb");
	if (fp == NULL)
	{
		return -1;
	}

	off_t i = 0; for (i = 0; i < size; ++i)
	{
		if (fwrite(&i, sizeof(i), 1, fp) != 1)
		{
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	
	return 0;
}

int validate_test_file()
{
	FILE *fp = fopen(test_file, "rb");
	if (fp == NULL)
	{
		return -1;
	}
	
	if (fseek(fp, 0, SEEK_END) != 0)
	{
	    fclose(fp);
	    return -1;
	}
	
	off_t last_pos = ftell(fp);
	if (last_pos == (off_t)-1)
	{
	    fclose(fp);
	    return -1;
	}
	
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
	    fclose(fp);
	    return -1;
	}
	
	off_t i = 0; for (i = 0; i < last_pos / sizeof(i); ++i)
	{
	    off_t cur_val = (off_t)-1;
	    if (fread(&cur_val, sizeof(cur_val), 1, fp) != 1)
	    {
		fprintf(stderr, "validation error: can't read from file at pos %lu\n", i);
		fclose(fp);
		return -1;
	    }
	    
	    if (cur_val != i)
	    {
		fprintf(stderr, "validation error at pos %ld, actual value is %lu\n", i, cur_val);
		fclose(fp);
		return -1;
	    }
	}
	
	fclose(fp);
	
	return 0;
}

void cleanup()
{
    unlink(test_file);
}
