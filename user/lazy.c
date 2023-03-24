#include "kernel/types.h"
#include "user/user.h"

#define ARRAY_LEN 1024

int main(int argc, char **argv)
{
  int *p = 0;
  int i = 0;
  int size = ARRAY_LEN;
  int print_mod = 10;

  switch(argc) {
    case 1:
    break;
    case 2:
    size = atoi(argv[1]);
    break;
    case 3:
    size = atoi(argv[1]);
    print_mod = atoi(argv[2]);
    break;
    default:
    printf("Usage: %s <number of integers to allocate> <print modulus>\n");
    printf("          Default to 1024 integers and 10.\n");
    exit(0);
    break;
  }
  

  // allocate some room on the heap for an array of size ARRAY_LEN integers.
  // Default to a single page.
  p = malloc(size * sizeof(int));
  if(!p) {
    printf("Could not allocate more memory, too bad...\n");
    exit(1);
  }
  printf("malloc'ed %d integers starting at %p\n", size, p);

  // set some values into that array
  for(; i < size; i++) {
    if(!(i%print_mod))
      printf("Accessing array entry at %d with address %p\n", i, p + i);
    p[i] = i;
  }

  // done, free it
  free(p);

  exit(0);
}
