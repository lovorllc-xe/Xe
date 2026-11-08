package main

import (
    "fmt"
    "os"
    "strconv"
)

func prefix(n int64) int64 {
    a := make([]int64, n + 1)
    var i int64 = 1
    for i <= n {
        a[i] = a[i - 1] + (i * i) % 13
        i = i + 1
    }
    var q int64 = 0
    i = 1
    for i <= n {
        var lo int64 = (i * 7) % n
        var hi int64 = lo + (i % 50)
        if hi > n {
            hi = n
        }
        q = (q + a[hi] - a[lo]) % 1000000007
        i = i + 1
    }
    return q
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(prefix(n))
}
