import sys

def treemake(depth):
    if depth > 0:
        return (treemake(depth - 1), treemake(depth - 1))
    return None

def treecount(t):
    if t is None:
        return 1
    return 1 + treecount(t[0]) + treecount(t[1])

def treebuild(n):
    total = 0
    for _ in range(4):
        total += treecount(treemake(n))
    return total

def main():
    n = int(sys.argv[1])
    print(treebuild(n))

main()
