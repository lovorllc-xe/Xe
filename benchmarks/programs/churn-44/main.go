package main

import (
    "fmt"
    "os"
    "strconv"
)

type Cell struct {
    value int64
    other int64
}

func churn(n int64) int64 {
    var slots [64]*Cell
    var total int64
    for i := int64(0); i < n; i++ {
        cell := &Cell{i, 0}
        k := i % 64
        if slots[k] != nil {
            total = (total + slots[k].value) % 1000000007
        }
        slots[k] = cell
    }
    for i := 0; i < 64; i++ {
        if slots[i] != nil {
            total = (total + slots[i].value) % 1000000007
        }
    }
    return total
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(churn(n))
}
