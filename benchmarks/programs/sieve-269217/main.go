package main

import (
    "fmt"
    "os"
    "strconv"
)

func sieve(n int64) int64 {
    s := make([]int64, n + 1)
    var i int64 = 2
    for i * i <= n {
        if s[i] == 0 {
            var j int64 = i * i
            for j <= n {
                s[j] = 1
                j = j + i
            }
        }
        i = i + 1
    }
    var c int64 = 0
    i = 2
    for i <= n {
        if s[i] == 0 {
            c = c + 1
        }
        i = i + 1
    }
    return c
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(sieve(n))
}
