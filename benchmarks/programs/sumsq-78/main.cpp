#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 sumsq(i64 n) {
    i64 s = 0;
    i64 i = 0;
    while (i < n) {
        s = s + (i * i) % 7;
        i = i + 1;
    }
    return s;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(sumsq(n)));
    return 0;
}
