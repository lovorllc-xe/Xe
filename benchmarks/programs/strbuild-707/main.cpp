#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 strbuild(i64 n) {
    std::string s;
    for (i64 i = 0; i < n; i++) s.push_back((char)(97 + i % 26));
    i64 c = 0;
    for (i64 i = 0; i < n; i++) if (s[i] == 97) c++;
    return c;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(strbuild(n)));
    return 0;
}
