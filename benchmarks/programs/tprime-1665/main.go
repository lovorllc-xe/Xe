package main

import (
    "fmt"
    "os"
    "strconv"
    "sync"
)

func tprime_worker(lo, hi int64, out *int64, wg *sync.WaitGroup) {
    defer wg.Done()
    var s int64
    for x := lo; x < hi; x++ {
        if x >= 2 {
            prime := int64(1)
            for d := int64(2); d*d <= x && prime == 1; d++ {
                if x%d == 0 {
                    prime = 0
                }
            }
            s += prime
        }
    }
    *out = s
}

func tprime(n int64) int64 {
    var wg sync.WaitGroup
    var out [8]int64
    for t := int64(0); t < 8; t++ {
        wg.Add(1)
        go tprime_worker(n*t/8, n*(t+1)/8, &out[t], &wg)
    }
    wg.Wait()
    var total int64
    for t := 0; t < 8; t++ {
        total = (total + out[t]) % 1000000007
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(tprime(n))
}
