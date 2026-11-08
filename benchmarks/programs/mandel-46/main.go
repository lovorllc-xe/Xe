package main

import (
    "fmt"
    "os"
    "strconv"
)

func mandel(n int64) int64 {
    var count int64 = 0
    var py int64 = 0
    for py < n {
        var px int64 = 0
        for px < n {
            var cx float64 = float64(px) * 3.0 / float64(n) - 2.0
            var cy float64 = float64(py) * 2.0 / float64(n) - 1.0
            var x float64 = 0.0
            var y float64 = 0.0
            var it int64 = 0
            var inside int64 = 1
            for it < 40 && inside == 1 {
                var t float64 = x * x - y * y + cx
                y = 2.0 * x * y + cy
                x = t
                if x * x + y * y > 4.0 {
                    inside = 0
                }
                it = it + 1
            }
            count = count + inside
            px = px + 1
        }
        py = py + 1
    }
    return count
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(mandel(n))
}
