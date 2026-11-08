package main

import (
    "fmt"
    "os"
    "strconv"
    "sync"
)

func tlcg_worker(lo, hi int64, out *int64, wg *sync.WaitGroup) {
    defer wg.Done()
    var s int64
    x := lo%1000 + 1
    for i := lo; i < hi; i++ {
        x = (x*1103515245 + 12345) % 2147483648
        s = (s + (x >> 16)) % 1000000007
    }
    *out = s
}

func tlcg(n int64) int64 {
    var wg sync.WaitGroup
    var out [4]int64
    for t := int64(0); t < 4; t++ {
        wg.Add(1)
        go tlcg_worker(n*t/4, n*(t+1)/4, &out[t], &wg)
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
    fmt.Println(tlcg(n))
}
