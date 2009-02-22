
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../src/nss_client.h"

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		exit(1);
	}
	
	printf("checking result for %s: %s\n", argv[1], strerror(-nss_check_user(argv[1])));
	if (argc > 2)
	{
		printf("checking result for %s: %s\n", argv[2], strerror(-nss_check_group(argv[2])));
	}

	return 0;
}


