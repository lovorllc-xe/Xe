#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

struct Node { i64 value; Node* next; };
static i64 listalloc(i64 n) {
    Node* head = nullptr;
    for (i64 i = 0; i < n; i++) head = new Node{i, head};
    i64 total = 0;
    while (head) { total = (total + head->value) % 1000000007; Node* nx = head->next; delete head; head = nx; }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(listalloc(n)));
    return 0;
}
