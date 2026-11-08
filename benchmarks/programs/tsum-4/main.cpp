#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
typedef long long i64;

static void tsum_worker(i64 lo, i64 hi, i64* out) {
    i64 s = 0;
    for (i64 i = lo; i < hi; i++) s += (i * i) % 7;
    *out = s;
}
static i64 tsum(i64 n) {
    std::thread th[4]; i64 out[4];
    for (int t = 0; t < 4; t++) th[t] = std::thread(tsum_worker, n * t / 4, n * (t + 1) / 4, &out[t]);
    i64 total = 0;
    for (int t = 0; t < 4; t++) { th[t].join(); total = (total + out[t]) % 1000000007; }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(tsum(n)));
    return 0;
}
