#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
typedef long long i64;

static void tlcg_worker(i64 lo, i64 hi, i64* out) {
    i64 s = 0;
    i64 x = lo % 1000 + 1;
    for (i64 i = lo; i < hi; i++) {
        x = (x * 1103515245 + 12345) % 2147483648;
        s = (s + (x >> 16)) % 1000000007;
    }
    *out = s;
}
static i64 tlcg(i64 n) {
    std::thread th[4]; i64 out[4];
    for (int t = 0; t < 4; t++) th[t] = std::thread(tlcg_worker, n * t / 4, n * (t + 1) / 4, &out[t]);
    i64 total = 0;
    for (int t = 0; t < 4; t++) { th[t].join(); total = (total + out[t]) % 1000000007; }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(tlcg(n)));
    return 0;
}
