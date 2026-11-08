package main

import (
    "fmt"
    "os"
    "strconv"
)

func editdist(n int64) int64 {
    a := make([]int64, n)
    b := make([]int64, n)
    var x int64 = 7
    var i int64 = 0
    for i < n {
        x = (x * 1103515245 + 12345) % 2147483648
        a[i] = (x >> 8) % 4
        x = (x * 1103515245 + 12345) % 2147483648
        b[i] = (x >> 8) % 4
        i = i + 1
    }
    prev := make([]int64, n + 1)
    cur := make([]int64, n + 1)
    i = 0
    for i <= n {
        prev[i] = i
        i = i + 1
    }
    i = 1
    for i <= n {
        cur[0] = i
        var j int64 = 1
        for j <= n {
            var cost int64 = 1
            if a[i - 1] == b[j - 1] {
                cost = 0
            }
            var best int64 = prev[j - 1] + cost
            if prev[j] + 1 < best {
                best = prev[j] + 1
            }
            if cur[j - 1] + 1 < best {
                best = cur[j - 1] + 1
            }
            cur[j] = best
            j = j + 1
        }
        j = 0
        for j <= n {
            prev[j] = cur[j]
            j = j + 1
        }
        i = i + 1
    }
    return prev[n]
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(editdist(n))
}
