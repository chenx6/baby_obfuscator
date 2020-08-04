#include <stdio.h>
#include <string.h>

char encrypt_data[] = {15, -10, -5, -23, 41, 6, -5, -2, 83, 0};

int main(int argc, char *argv[]) {
  char password[30];
  // right flag: test_flag
  scanf("%30s", password);
  size_t len = strlen(password);
  for (int i = 0; i < len; i++) {
    password[i] ^= 42;
    password[i] += 6;
  }
  for (int i = 1; i < len; i++) {
    password[i - 1] -= password[i];
  }
  if (!strncmp(password, encrypt_data, len)) {
    printf("Correct!");
  }
  return 0;
}
