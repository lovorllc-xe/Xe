#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 sort(i64 n) {
    std::vector<i64> a(n);
    i64 x = 12345;
    i64 i = 0;
    while (i < n) {
        x = (x * 1103515245 + 12345) % 2147483648;
        a[i] = x % 100000;
        i = i + 1;
    }
    i = 1;
    while (i < n) {
        i64 v = a[i];
        i64 j = i - 1;
        while (j >= 0 && a[j] > v) {
            a[j + 1] = a[j];
            j = j - 1;
        }
        a[j + 1] = v;
        i = i + 1;
    }
    i64 h = 0;
    i = 0;
    while (i < n) {
        h = (h * 31 + a[i]) % 1000000007;
        i = i + 1;
    }
    return h;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(sort(n)));
    return 0;
}
