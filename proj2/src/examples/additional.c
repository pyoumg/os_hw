#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int
main(int argc, char** argv)
{
    
    if (argc != 5) {
        printf("usage: additional a b c d\n");
        return EXIT_FAILURE;
    }
    
    int ans[2] = { 0, };
    int num[4] = { 0, };
    for (int i = 0; i < 4; i++) {
        num[i] = atoi(argv[i+1]);
    }
    ans[0] = fibonacci(num[0]);
    ans[1] = max_of_four_int(num[0], num[1], num[2], num[3]);

    printf("%d %d\n", ans[0], ans[1]);
    return EXIT_SUCCESS;
    
}
