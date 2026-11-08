#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 editdist(i64 n) {
    std::vector<i64> a(n);
    std::vector<i64> b(n);
    i64 x = 7;
    i64 i = 0;
    while (i < n) {
        x = (x * 1103515245 + 12345) % 2147483648;
        a[i] = (x >> 8) % 4;
        x = (x * 1103515245 + 12345) % 2147483648;
        b[i] = (x >> 8) % 4;
        i = i + 1;
    }
    std::vector<i64> prev(n + 1);
    std::vector<i64> cur(n + 1);
    i = 0;
    while (i <= n) {
        prev[i] = i;
        i = i + 1;
    }
    i = 1;
    while (i <= n) {
        cur[0] = i;
        i64 j = 1;
        while (j <= n) {
            i64 cost = 1;
            if (a[i - 1] == b[j - 1]) {
                cost = 0;
            }
            i64 best = prev[j - 1] + cost;
            if (prev[j] + 1 < best) {
                best = prev[j] + 1;
            }
            if (cur[j - 1] + 1 < best) {
                best = cur[j - 1] + 1;
            }
            cur[j] = best;
            j = j + 1;
        }
        j = 0;
        while (j <= n) {
            prev[j] = cur[j];
            j = j + 1;
        }
        i = i + 1;
    }
    return prev[n];
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(editdist(n)));
    return 0;
}
