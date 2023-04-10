
int test(int x)
{
    int a = 1;
    int b = 4567;
    int p = 0;
    int c = 10000;
    // for (int i = 0; i < 100; i++)

    while (x < 1234)
    {
        p = x * 2;
        a++;
        // b--;
    }
    return p + c;
}

int main()
{
    int r = test(3);
    return r;
}