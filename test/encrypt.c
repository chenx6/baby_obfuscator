#include <stdint.h>

char *__decrypt(char *encStr, uint64_t length) {
  char *curr = encStr;
  for (uint16_t i = 0; i < length; i++) {
    *curr ^= 42;
    curr++;
  }
  return encStr;
}

char *__encrypt(char *originStr, uint64_t length) {
  char *curr = originStr;
  for (uint16_t i = 0; i < length; i++) {
    *curr ^= 42;
    curr++;
  }
  return originStr;
}