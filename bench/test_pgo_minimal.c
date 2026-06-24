#include <stdio.h>
int main(void) {
    printf("PGO minimal test\n");
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i * i;
    }
    printf("sum=%d\n", sum);
    return 0;
}