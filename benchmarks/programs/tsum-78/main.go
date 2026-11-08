package main

import (
    "fmt"
    "os"
    "strconv"
    "sync"
)

func tsum_worker(lo, hi int64, out *int64, wg *sync.WaitGroup) {
    defer wg.Done()
    var s int64
    for i := lo; i < hi; i++ {
        s += (i * i) % 7
    }
    *out = s
}

func tsum(n int64) int64 {
    var wg sync.WaitGroup
    var out [4]int64
    for t := int64(0); t < 4; t++ {
        wg.Add(1)
        go tsum_worker(n*t/4, n*(t+1)/4, &out[t], &wg)
    }
    wg.Wait()
    var total int64
    for t := 0; t < 4; t++ {
        total = (total + out[t]) % 1000000007
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(tsum(n))
}
