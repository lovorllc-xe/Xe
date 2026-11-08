#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 hashtab(i64 n) {
    i64 cap = 16;
    while (cap < n * 2) {
        cap = cap * 2;
    }
    std::vector<i64> keys(cap);
    std::vector<i64> vals(cap);
    i64 i = 1;
    while (i <= n) {
        i64 k = (i * 2654435761) % 4294967296;
        i64 h = k & (cap - 1);
        while (keys[h] != 0 && keys[h] != k) {
            h = (h + 1) & (cap - 1);
        }
        keys[h] = k;
        vals[h] = vals[h] + i;
        i = i + 1;
    }
    i64 total = 0;
    i = 1;
    while (i <= n) {
        i64 k = (i * 2654435761) % 4294967296;
        i64 h = k & (cap - 1);
        while (keys[h] != k) {
            h = (h + 1) & (cap - 1);
        }
        total = (total + vals[h]) % 1000000007;
        i = i + 1;
    }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(hashtab(n)));
    return 0;
}
