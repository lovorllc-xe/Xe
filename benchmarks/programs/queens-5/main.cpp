#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 queens_rec(i64 n, i64 row, i64 cols, i64 d1, i64 d2) {
    if (row == n) {
        return 1;
    }
    i64 count = 0;
    i64 c = 0;
    while (c < n) {
        i64 b1 = 1 << c;
        i64 b2 = 1 << (row + c);
        i64 b3 = 1 << (row - c + n);
        if ((cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0) {
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3);
        }
        c = c + 1;
    }
    return count;
}

static i64 queens(i64 n) {
    return queens_rec(n, 0, 0, 0, 0);
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(queens(n)));
    return 0;
}
