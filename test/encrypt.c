#include <stdio.h>

char *__decrypt(char *encStr) {
  char *curr = encStr;
  while (*curr) {
    *curr ^= 42;
    curr++;
  }
  return encStr;
}

char *__encrypt(char *originStr) {
  char *curr = originStr;
  while (*curr) {
    *curr ^= 42;
    curr++;
  }
  return originStr;
}