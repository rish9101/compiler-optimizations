#include <malloc.h>

struct C
{
    int f1;
};

struct B
{
    int f1;
    struct C *f2;
};

struct A
{
    int f1;
    char f2;
    struct B *f3;
};

struct A * global_ptr;

int main(int x)
{

    int *y = malloc(0x4);

    struct A *a = malloc(sizeof(struct A));
    a->f3 = malloc(sizeof(struct B));
    a->f3->f2 = malloc(sizeof(struct C));

    if (x > 2) {
    struct B *b = malloc(sizeof(struct B));
    *y = b->f1;
    }
    // struct A *a1 = malloc(sizeof(struct A));
    // a1->f3.f2 = malloc(sizeof(struct C));

    // a->f3.f2 = a1->f3.f2;

    // a->f3->f2->f1 = 434;
    global_ptr = a;
    *y = global_ptr->f1;

    return a->f3->f1 + (*y);
}