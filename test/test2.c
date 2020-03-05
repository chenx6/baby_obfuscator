#include <stdio.h>
#include <string.h>

int main()
{
    char *str = "test";
    if (!strcmp(str, str)) {
        puts("test");
        printf("success\n");
    }
    return 0;
}
