package main

import (
    "fmt"
    "os"
    "strconv"
)

func gcd(a int64, b int64) int64 {
    var x int64 = a
    var y int64 = b
    for y != 0 {
        var t int64 = x % y
        x = y
        y = t
    }
    return x
}

func gcdsum(n int64) int64 {
    var s int64 = 0
    var i int64 = 1
    for i <= n {
        s = s + gcd(i, n - i + 1)
        i = i + 1
    }
    return s
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(gcdsum(n))
}
