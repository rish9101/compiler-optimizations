#include <malloc.h>

int main(int x)
{

    int *a, *b, *c;
    int d = 203, e = 23123, f = 4334;

    a = malloc(0x4);

    if (x != 32434)
    {
        b = a;
    }
    else
    {
        b = malloc(0x4);
    }
    printf("%p", b);
    // b = a;
    // c = b;

    return *b;
}