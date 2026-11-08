package main

import (
    "fmt"
    "os"
    "strconv"
)

func matmul(n int64) int64 {
    a := make([]int64, n * n)
    b := make([]int64, n * n)
    c := make([]int64, n * n)
    var i int64 = 0
    for i < n * n {
        a[i] = (i * 7 + 3) % 10
        b[i] = (i * 5 + 1) % 10
        i = i + 1
    }
    i = 0
    for i < n {
        var j int64 = 0
        for j < n {
            var s int64 = 0
            var k int64 = 0
            for k < n {
                s = s + a[i * n + k] * b[k * n + j]
                k = k + 1
            }
            c[i * n + j] = s
            j = j + 1
        }
        i = i + 1
    }
    var t int64 = 0
    i = 0
    for i < n * n {
        t = (t + c[i]) % 1000003
        i = i + 1
    }
    return t
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(matmul(n))
}
