package main

import (
    "fmt"
    "os"
    "strconv"
)

type Node struct {
    value int64
    next  *Node
}

func listalloc(n int64) int64 {
    var head *Node
    for i := int64(0); i < n; i++ {
        head = &Node{i, head}
    }
    var total int64
    for cur := head; cur != nil; cur = cur.next {
        total = (total + cur.value) % 1000000007
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(listalloc(n))
}
