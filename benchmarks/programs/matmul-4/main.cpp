#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 matmul(i64 n) {
    std::vector<i64> a(n * n);
    std::vector<i64> b(n * n);
    std::vector<i64> c(n * n);
    i64 i = 0;
    while (i < n * n) {
        a[i] = (i * 7 + 3) % 10;
        b[i] = (i * 5 + 1) % 10;
        i = i + 1;
    }
    i = 0;
    while (i < n) {
        i64 j = 0;
        while (j < n) {
            i64 s = 0;
            i64 k = 0;
            while (k < n) {
                s = s + a[i * n + k] * b[k * n + j];
                k = k + 1;
            }
            c[i * n + j] = s;
            j = j + 1;
        }
        i = i + 1;
    }
    i64 t = 0;
    i = 0;
    while (i < n * n) {
        t = (t + c[i]) % 1000003;
        i = i + 1;
    }
    return t;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(matmul(n)));
    return 0;
}
