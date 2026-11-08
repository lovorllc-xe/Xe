package main

import (
    "fmt"
    "os"
    "strconv"
)

func binsearch(n int64) int64 {
    a := make([]int64, n)
    var i int64 = 0
    for i < n {
        a[i] = i * 3
        i = i + 1
    }
    var found int64 = 0
    i = 0
    for i < n {
        var key int64 = (i * 7) % (n * 3)
        var lo int64 = 0
        var hi int64 = n - 1
        for lo <= hi {
            var mid int64 = (lo + hi) / 2
            if a[mid] == key {
                found = found + 1
                lo = hi + 1
            } else {
                if a[mid] < key {
                    lo = mid + 1
                } else {
                    hi = mid - 1
                }
            }
        }
        i = i + 1
    }
    return found
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(binsearch(n))
}
