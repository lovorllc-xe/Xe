package main

import (
    "fmt"
    "os"
    "strconv"
)

func sumsq(n int64) int64 {
    var s int64 = 0
    var i int64 = 0
    for i < n {
        s = s + (i * i) % 7
        i = i + 1
    }
    return s
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(sumsq(n))
}
