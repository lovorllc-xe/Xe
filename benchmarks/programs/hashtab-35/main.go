package main

import (
    "fmt"
    "os"
    "strconv"
)

func hashtab(n int64) int64 {
    var cap int64 = 16
    for cap < n * 2 {
        cap = cap * 2
    }
    keys := make([]int64, cap)
    vals := make([]int64, cap)
    var i int64 = 1
    for i <= n {
        var k int64 = (i * 2654435761) % 4294967296
        var h int64 = k & (cap - 1)
        for keys[h] != 0 && keys[h] != k {
            h = (h + 1) & (cap - 1)
        }
        keys[h] = k
        vals[h] = vals[h] + i
        i = i + 1
    }
    var total int64 = 0
    i = 1
    for i <= n {
        var k int64 = (i * 2654435761) % 4294967296
        var h int64 = k & (cap - 1)
        for keys[h] != k {
            h = (h + 1) & (cap - 1)
        }
        total = (total + vals[h]) % 1000000007
        i = i + 1
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(hashtab(n))
}
