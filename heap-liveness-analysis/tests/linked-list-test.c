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
// a->f2->f2->f2->f2->f2->f2->f2 = malloc(sizeof(struct A));
// a->f2->f2->f2->f2->f2->f2->f2->f2 = malloc(sizeof(struct A));
    // if (argc > 2)
    // {
    //     struct B *b = malloc(sizeof(struct B));
    //     *y = b->f1;
    // }
    // struct A *a1 = malloc(sizeof(struct A));
    // a1->f3.f2 = malloc(sizeof(struct C));

    // a->f3.f2 = a1->f3.f2;

    // a->f3->f2->f1 = 434;
    // global_ptr = a;
    // *y = global_ptr->f1;

    // int ret= a->f3->f1 + (*y);
    // free(a->f3->f2);
    // free(a->f3);
    // free(a);
    // free(y);
    return 1;
}