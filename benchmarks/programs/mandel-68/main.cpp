#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 mandel(i64 n) {
    i64 count = 0;
    i64 py = 0;
    while (py < n) {
        i64 px = 0;
        while (px < n) {
            double cx = (double)(px) * 3.0 / (double)(n) - 2.0;
            double cy = (double)(py) * 2.0 / (double)(n) - 1.0;
            double x = 0.0;
            double y = 0.0;
            i64 it = 0;
            i64 inside = 1;
            while (it < 40 && inside == 1) {
                double t = x * x - y * y + cx;
                y = 2.0 * x * y + cy;
                x = t;
                if (x * x + y * y > 4.0) {
                    inside = 0;
                }
                it = it + 1;
            }
            count = count + inside;
            px = px + 1;
        }
        py = py + 1;
    }
    return count;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(mandel(n)));
    return 0;
}
