int main()
{
    int res;
    int n = 1;

    while (n++ < 100)
    {
        res = 0;
        res += 2;
        res += 2;
        res *= 4;
        res /= 2;
    }
    return res;
}