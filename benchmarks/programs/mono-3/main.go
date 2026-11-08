package main

import (
    "fmt"
    "os"
    "strconv"
)

func sumsq(n int64) int64 {
    var s int64 = 0
    var i int64 = 0
    for i < n {
        s = s + (i * i) % 7
        i = i + 1
    }
    return s
}

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

func sieve(n int64) int64 {
    s := make([]int64, n + 1)
    var i int64 = 2
    for i * i <= n {
        if s[i] == 0 {
            var j int64 = i * i
            for j <= n {
                s[j] = 1
                j = j + i
            }
        }
        i = i + 1
    }
    var c int64 = 0
    i = 2
    for i <= n {
        if s[i] == 0 {
            c = c + 1
        }
        i = i + 1
    }
    return c
}

func matmul(n int64) int64 {
    a := make([]int64, n * n)
    b := make([]int64, n * n)
    c := make([]int64, n * n)
    var i int64 = 0
    for i < n * n {
        a[i] = (i * 7 + 3) % 10
        b[i] = (i * 5 + 1) % 10
        i = i + 1
    }
    i = 0
    for i < n {
        var j int64 = 0
        for j < n {
            var s int64 = 0
            var k int64 = 0
            for k < n {
                s = s + a[i * n + k] * b[k * n + j]
                k = k + 1
            }
            c[i * n + j] = s
            j = j + 1
        }
        i = i + 1
    }
    var t int64 = 0
    i = 0
    for i < n * n {
        t = (t + c[i]) % 1000003
        i = i + 1
    }
    return t
}

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

func hashtab(n int64) int64 {
    var cap int64 = 16
    for cap < n * 2 {
        cap = cap * 2
    }
    keys := make([]int64, cap)
    vals := make([]int64, cap)
    var i int64 = 1
    for i <= n {
        var k int64 = (i * 2654435761) % 4294967296
        var h int64 = k & (cap - 1)
        for keys[h] != 0 && keys[h] != k {
            h = (h + 1) & (cap - 1)
        }
        keys[h] = k
        vals[h] = vals[h] + i
        i = i + 1
    }
    var total int64 = 0
    i = 1
    for i <= n {
        var k int64 = (i * 2654435761) % 4294967296
        var h int64 = k & (cap - 1)
        for keys[h] != k {
            h = (h + 1) & (cap - 1)
        }
        total = (total + vals[h]) % 1000000007
        i = i + 1
    }
    return total
}

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

func gcd(a int64, b int64) int64 {
    var x int64 = a
    var y int64 = b
    for y != 0 {
        var t int64 = x % y
        x = y
        y = t
    }
    return x
}

func gcdsum(n int64) int64 {
    var s int64 = 0
    var i int64 = 1
    for i <= n {
        s = s + gcd(i, n - i + 1)
        i = i + 1
    }
    return s
}

func lcg(n int64) int64 {
    var x int64 = 1
    var s int64 = 0
    var i int64 = 0
    for i < n {
        x = (x * 1103515245 + 12345) % 2147483648
        s = (s + (x >> 16)) % 1000000007
        i = i + 1
    }
    return s
}

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

func prefix(n int64) int64 {
    a := make([]int64, n + 1)
    var i int64 = 1
    for i <= n {
        a[i] = a[i - 1] + (i * i) % 13
        i = i + 1
    }
    var q int64 = 0
    i = 1
    for i <= n {
        var lo int64 = (i * 7) % n
        var hi int64 = lo + (i % 50)
        if hi > n {
            hi = n
        }
        q = (q + a[hi] - a[lo]) % 1000000007
        i = i + 1
    }
    return q
}

func popcnt(n int64) int64 {
    var t int64 = 0
    var i int64 = 0
    for i < n {
        var x int64 = i
        for x != 0 {
            x = x & (x - 1)
            t = t + 1
        }
        i = i + 1
    }
    return t
}

func fib(n int64) int64 {
    if n < 2 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

func queens_rec(n int64, row int64, cols int64, d1 int64, d2 int64) int64 {
    if row == n {
        return 1
    }
    var count int64 = 0
    var c int64 = 0
    for c < n {
        var b1 int64 = 1 << c
        var b2 int64 = 1 << (row + c)
        var b3 int64 = 1 << (row - c + n)
        if (cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0 {
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3)
        }
        c = c + 1
    }
    return count
}

func queens(n int64) int64 {
    return queens_rec(n, 0, 0, 0, 0)
}

func main() {
    n, _ := strconv.ParseInt(os.Args[1], 10, 64)
    fmt.Println((sumsq(n * 30000) + collatz(n * 400) + sieve(n * 15000) + matmul(n * 1) + sort(n * 40) + hashtab(n * 3000) + editdist(n * 6) + gcdsum(n * 1500) + lcg(n * 30000) + binsearch(n * 1500) + prefix(n * 30000) + popcnt(n * 3000) + fib(12) + queens(6)) % 1000000007)
}
