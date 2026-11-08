package main

import (
    "fmt"
    "os"
    "strconv"
)

func lcg(n int64) int64 {
    var x int64 = 1
    var s int64 = 0
    var i int64 = 0
    for i < n {
        x = (x * 1103515245 + 12345) % 2147483648
        s = (s + (x >> 16)) % 1000000007
        i = i + 1
    }
    return s
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(lcg(n))
}
