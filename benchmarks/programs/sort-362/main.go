package main

import (
    "fmt"
    "os"
    "strconv"
)

func sort(n int64) int64 {
    a := make([]int64, n)
    var x int64 = 12345
    var i int64 = 0
    for i < n {
        x = (x * 1103515245 + 12345) % 2147483648
        a[i] = x % 100000
        i = i + 1
    }
    i = 1
    for i < n {
        var v int64 = a[i]
        var j int64 = i - 1
        for j >= 0 && a[j] > v {
            a[j + 1] = a[j]
            j = j - 1
        }
        a[j + 1] = v
        i = i + 1
    }
    var h int64 = 0
    i = 0
    for i < n {
        h = (h * 31 + a[i]) % 1000000007
        i = i + 1
    }
    return h
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(sort(n))
}
