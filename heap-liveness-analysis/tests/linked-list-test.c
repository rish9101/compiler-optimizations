#include <malloc.h>

struct A
{
    int f1;
    struct A *f2;
};

struct A *global_ptr;

int main(int argc, char **argv)
{

    // int *y = malloc(0x4);

    struct A *a = malloc(sizeof(struct A));
    a->f2 = malloc(sizeof(struct A));
    a->f2->f2 = malloc(sizeof(struct A));
    a->f2->f2->f2 = malloc(sizeof(struct A));
    a->f2->f2->f2->f2 = malloc(sizeof(struct A));
    a->f2->f2->f2->f2->f2 = malloc(sizeof(struct A));
    a->f2->f2->f2->f2->f2->f2 = malloc(sizeof(struct A));

    return 1;
}
