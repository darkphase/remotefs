#ifndef FILE_H
#define FILE_H

#include <sys/types.h>

static const char test_file[] = "test.file";

int create_test_file(size_t size);
int validate_test_file();
void cleanup();

#endif /* FILE_H */
