#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 gcd(i64 a, i64 b) {
    i64 x = a;
    i64 y = b;
    while (y != 0) {
        i64 t = x % y;
        x = y;
        y = t;
    }
    return x;
}

static i64 gcdsum(i64 n) {
    i64 s = 0;
    i64 i = 1;
    while (i <= n) {
        s = s + gcd(i, n - i + 1);
        i = i + 1;
    }
    return s;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(gcdsum(n)));
    return 0;
}
