#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 prefix(i64 n) {
    std::vector<i64> a(n + 1);
    i64 i = 1;
    while (i <= n) {
        a[i] = a[i - 1] + (i * i) % 13;
        i = i + 1;
    }
    i64 q = 0;
    i = 1;
    while (i <= n) {
        i64 lo = (i * 7) % n;
        i64 hi = lo + (i % 50);
        if (hi > n) {
            hi = n;
        }
        q = (q + a[hi] - a[lo]) % 1000000007;
        i = i + 1;
    }
    return q;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(prefix(n)));
    return 0;
}
