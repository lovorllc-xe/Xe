#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 popcnt(i64 n) {
    i64 t = 0;
    i64 i = 0;
    while (i < n) {
        i64 x = i;
        while (x != 0) {
            x = x & (x - 1);
            t = t + 1;
        }
        i = i + 1;
    }
    return t;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(popcnt(n)));
    return 0;
}
