#include <stdio.h>

void atexit(void)
{
}

extern int h_errno;
int * __errno_location(void)
{
  return &h_errno;
}
