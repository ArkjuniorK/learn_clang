#include <stdio.h>

void printn(int n)
{
  for (int i = 0; i < n; i++)
  {
    puts("Hello world!");
  }
}

int main(int argc, char **argv)
{
  printn(5);
  return 0;
}
