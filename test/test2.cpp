#include <iostream>

int main() {
  int num;
  scanf("%d", &num);
  if (num == 0) {
    printf("input error!\n");
  }
  double a;
  int b;
  try {
    a = 1.0 / num;
    b = 1 / num;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  printf("%f %d\n", a, b);
}