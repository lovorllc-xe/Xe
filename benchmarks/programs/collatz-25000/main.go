package main

import (
    "fmt"
    "os"
    "strconv"
)

func collatz(n int64) int64 {
    var total int64 = 0
    var i int64 = 1
    for i < n {
        var x int64 = i
        for x != 1 {
            if x % 2 == 0 {
                x = x / 2
            } else {
                x = 3 * x + 1
            }
            total = total + 1
        }
        i = i + 1
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(collatz(n))
}
