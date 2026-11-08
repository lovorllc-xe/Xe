#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
typedef long long i64;

static void tprime_worker(i64 lo, i64 hi, i64* out) {
    i64 s = 0;
    for (i64 x = lo; x < hi; x++) {
        if (x >= 2) {
            i64 prime = 1;
            for (i64 d = 2; d * d <= x && prime; d++) if (x % d == 0) prime = 0;
            s += prime;
        }
    }
    *out = s;
}
static i64 tprime(i64 n) {
    std::thread th[8]; i64 out[8];
    for (int t = 0; t < 8; t++) th[t] = std::thread(tprime_worker, n * t / 8, n * (t + 1) / 8, &out[t]);
    i64 total = 0;
    for (int t = 0; t < 8; t++) { th[t].join(); total = (total + out[t]) % 1000000007; }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(tprime(n)));
    return 0;
}
