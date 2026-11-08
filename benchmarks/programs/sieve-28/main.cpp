#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 sieve(i64 n) {
    std::vector<i64> s(n + 1);
    i64 i = 2;
    while (i * i <= n) {
        if (s[i] == 0) {
            i64 j = i * i;
            while (j <= n) {
                s[j] = 1;
                j = j + i;
            }
        }
        i = i + 1;
    }
    i64 c = 0;
    i = 2;
    while (i <= n) {
        if (s[i] == 0) {
            c = c + 1;
        }
        i = i + 1;
    }
    return c;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(sieve(n)));
    return 0;
}
