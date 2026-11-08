package main

import (
    "fmt"
    "os"
    "strconv"
)

type Tree struct {
    left  *Tree
    right *Tree
}

func treemake(depth int64) *Tree {
    t := &Tree{}
    if depth > 0 {
        t.left = treemake(depth - 1)
        t.right = treemake(depth - 1)
    }
    return t
}

func treecount(t *Tree) int64 {
    if t.left == nil {
        return 1
    }
    return 1 + treecount(t.left) + treecount(t.right)
}

func treebuild(n int64) int64 {
    var total int64
    for r := 0; r < 4; r++ {
        total += treecount(treemake(n))
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(treebuild(n))
}
