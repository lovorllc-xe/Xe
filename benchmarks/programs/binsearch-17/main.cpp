#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 binsearch(i64 n) {
    std::vector<i64> a(n);
    i64 i = 0;
    while (i < n) {
        a[i] = i * 3;
        i = i + 1;
    }
    i64 found = 0;
    i = 0;
    while (i < n) {
        i64 key = (i * 7) % (n * 3);
        i64 lo = 0;
        i64 hi = n - 1;
        while (lo <= hi) {
            i64 mid = (lo + hi) / 2;
            if (a[mid] == key) {
                found = found + 1;
                lo = hi + 1;
            } else {
                if (a[mid] < key) {
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
        }
        i = i + 1;
    }
    return found;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(binsearch(n)));
    return 0;
}
