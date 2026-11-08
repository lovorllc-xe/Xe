#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

struct Cell { i64 value; i64 other; };
static i64 churn(i64 n) {
    Cell* slots[64] = {};
    i64 total = 0;
    for (i64 i = 0; i < n; i++) {
        Cell* cell = new Cell{i, 0};
        i64 k = i % 64;
        if (slots[k]) { total = (total + slots[k]->value) % 1000000007; delete slots[k]; }
        slots[k] = cell;
    }
    for (int i = 0; i < 64; i++) if (slots[i]) { total = (total + slots[i]->value) % 1000000007; delete slots[i]; }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(churn(n)));
    return 0;
}
