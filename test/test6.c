#include <stdio.h>

int main(int argc, const char *argv[]) {
  int num;
  scanf("%d", &num);
  switch (num) {
  case 1:
    printf("you input number one\n");
    break;
  case 2:
    printf("you input number two\n");
    break;
  default:
    printf("you input what?\n");
    break;
  }
  return 0;
}
