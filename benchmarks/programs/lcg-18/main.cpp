#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 lcg(i64 n) {
    i64 x = 1;
    i64 s = 0;
    i64 i = 0;
    while (i < n) {
        x = (x * 1103515245 + 12345) % 2147483648;
        s = (s + (x >> 16)) % 1000000007;
        i = i + 1;
    }
    return s;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(lcg(n)));
    return 0;
}
