
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;

  if (argc <=1)
  {
      printf("input required!\n");
      exit(0);
  }
  i = atoi(argv[1]);
  sleep(i);
  exit(0);
}
