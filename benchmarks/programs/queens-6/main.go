package main

import (
    "fmt"
    "os"
    "strconv"
)

func queens_rec(n int64, row int64, cols int64, d1 int64, d2 int64) int64 {
    if row == n {
        return 1
    }
    var count int64 = 0
    var c int64 = 0
    for c < n {
        var b1 int64 = 1 << c
        var b2 int64 = 1 << (row + c)
        var b3 int64 = 1 << (row - c + n)
        if (cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0 {
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3)
        }
        c = c + 1
    }
    return count
}

func queens(n int64) int64 {
    return queens_rec(n, 0, 0, 0, 0)
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(queens(n))
}
