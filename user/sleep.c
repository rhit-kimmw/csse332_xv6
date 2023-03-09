#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char **argv)
{
  /* TODO: Insert your code here. */
  if(argc < 2)
  {
    printf("Put the ticks to sleep. ex) sleep 10");
    exit(1);
  }

  int ticks = atoi(argv[1]);
  sleep(ticks);
  exit(0);
}
