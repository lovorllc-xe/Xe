#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 collatz(i64 n) {
    i64 total = 0;
    i64 i = 1;
    while (i < n) {
        i64 x = i;
        while (x != 1) {
            if (x % 2 == 0) {
                x = x / 2;
            } else {
                x = 3 * x + 1;
            }
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(collatz(n)));
    return 0;
}
