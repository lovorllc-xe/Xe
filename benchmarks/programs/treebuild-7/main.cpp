#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

struct Tree { Tree* left; Tree* right; };
static Tree* treemake(i64 depth) {
    Tree* t = new Tree{nullptr, nullptr};
    if (depth > 0) { t->left = treemake(depth - 1); t->right = treemake(depth - 1); }
    return t;
}
static i64 treecount(Tree* t) { return t->left ? 1 + treecount(t->left) + treecount(t->right) : 1; }
static void treefree(Tree* t) { if (t->left) { treefree(t->left); treefree(t->right); } delete t; }
static i64 treebuild(i64 n) {
    i64 total = 0;
    for (int r = 0; r < 4; r++) { Tree* root = treemake(n); total += treecount(root); treefree(root); }
    return total;
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)(treebuild(n)));
    return 0;
}
