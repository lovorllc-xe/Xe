package main

import (
    "fmt"
    "os"
    "strconv"
)

func popcnt(n int64) int64 {
    var t int64 = 0
    var i int64 = 0
    for i < n {
        var x int64 = i
        for x != 0 {
            x = x & (x - 1)
            t = t + 1
        }
        i = i + 1
    }
    return t
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(popcnt(n))
}
