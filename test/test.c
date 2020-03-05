#include <stdio.h>

int main() {
  puts("This is a testing string!");
  char ch;
  if ((ch = getchar()) == '6') {
    printf("6666%c\n", ch);
  } else {
    printf("WTF?!\n");
  }
  return 0;
}
