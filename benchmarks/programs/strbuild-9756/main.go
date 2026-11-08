package main

import (
    "fmt"
    "os"
    "strconv"
    "strings"
)

func strbuild(n int64) int64 {
    var b strings.Builder
    for i := int64(0); i < n; i++ {
        b.WriteByte(byte(97 + i%26))
    }
    s := b.String()
    var c int64
    for i := int64(0); i < n; i++ {
        if s[i] == 97 {
            c++
        }
    }
    return c
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println(strbuild(n))
}
