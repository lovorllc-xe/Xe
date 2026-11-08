#![allow(warnings)]

fn sumsq(n: i64) -> i64 {
    let mut s: i64 = 0;
    let mut i: i64 = 0;
    while i < n {
        s = s + (i * i) % 7;
        i = i + 1;
    }
    return s;
}

fn collatz(n: i64) -> i64 {
    let mut total: i64 = 0;
    let mut i: i64 = 1;
    while i < n {
        let mut x: i64 = i;
        while x != 1 {
            if x % 2 == 0 {
                x = x / 2;
            } else {
                x = 3 * x + 1;
            }
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

fn sieve(n: i64) -> i64 {
    let mut s: Vec<i64> = vec![0i64; (n + 1) as usize];
    let mut i: i64 = 2;
    while i * i <= n {
        if s[(i) as usize] == 0 {
            let mut j: i64 = i * i;
            while j <= n {
                s[(j) as usize] = 1;
                j = j + i;
            }
        }
        i = i + 1;
    }
    let mut c: i64 = 0;
    i = 2;
    while i <= n {
        if s[(i) as usize] == 0 {
            c = c + 1;
        }
        i = i + 1;
    }
    return c;
}

fn matmul(n: i64) -> i64 {
    let mut a: Vec<i64> = vec![0i64; (n * n) as usize];
    let mut b: Vec<i64> = vec![0i64; (n * n) as usize];
    let mut c: Vec<i64> = vec![0i64; (n * n) as usize];
    let mut i: i64 = 0;
    while i < n * n {
        a[(i) as usize] = (i * 7 + 3) % 10;
        b[(i) as usize] = (i * 5 + 1) % 10;
        i = i + 1;
    }
    i = 0;
    while i < n {
        let mut j: i64 = 0;
        while j < n {
            let mut s: i64 = 0;
            let mut k: i64 = 0;
            while k < n {
                s = s + a[(i * n + k) as usize] * b[(k * n + j) as usize];
                k = k + 1;
            }
            c[(i * n + j) as usize] = s;
            j = j + 1;
        }
        i = i + 1;
    }
    let mut t: i64 = 0;
    i = 0;
    while i < n * n {
        t = (t + c[(i) as usize]) % 1000003;
        i = i + 1;
    }
    return t;
}

fn sort(n: i64) -> i64 {
    let mut a: Vec<i64> = vec![0i64; (n) as usize];
    let mut x: i64 = 12345;
    let mut i: i64 = 0;
    while i < n {
        x = (x * 1103515245 + 12345) % 2147483648;
        a[(i) as usize] = x % 100000;
        i = i + 1;
    }
    i = 1;
    while i < n {
        let mut v: i64 = a[(i) as usize];
        let mut j: i64 = i - 1;
        while j >= 0 && a[(j) as usize] > v {
            a[(j + 1) as usize] = a[(j) as usize];
            j = j - 1;
        }
        a[(j + 1) as usize] = v;
        i = i + 1;
    }
    let mut h: i64 = 0;
    i = 0;
    while i < n {
        h = (h * 31 + a[(i) as usize]) % 1000000007;
        i = i + 1;
    }
    return h;
}

fn hashtab(n: i64) -> i64 {
    let mut cap: i64 = 16;
    while cap < n * 2 {
        cap = cap * 2;
    }
    let mut keys: Vec<i64> = vec![0i64; (cap) as usize];
    let mut vals: Vec<i64> = vec![0i64; (cap) as usize];
    let mut i: i64 = 1;
    while i <= n {
        let mut k: i64 = (i * 2654435761) % 4294967296;
        let mut h: i64 = k & (cap - 1);
        while keys[(h) as usize] != 0 && keys[(h) as usize] != k {
            h = (h + 1) & (cap - 1);
        }
        keys[(h) as usize] = k;
        vals[(h) as usize] = vals[(h) as usize] + i;
        i = i + 1;
    }
    let mut total: i64 = 0;
    i = 1;
    while i <= n {
        let mut k: i64 = (i * 2654435761) % 4294967296;
        let mut h: i64 = k & (cap - 1);
        while keys[(h) as usize] != k {
            h = (h + 1) & (cap - 1);
        }
        total = (total + vals[(h) as usize]) % 1000000007;
        i = i + 1;
    }
    return total;
}

fn editdist(n: i64) -> i64 {
    let mut a: Vec<i64> = vec![0i64; (n) as usize];
    let mut b: Vec<i64> = vec![0i64; (n) as usize];
    let mut x: i64 = 7;
    let mut i: i64 = 0;
    while i < n {
        x = (x * 1103515245 + 12345) % 2147483648;
        a[(i) as usize] = (x >> 8) % 4;
        x = (x * 1103515245 + 12345) % 2147483648;
        b[(i) as usize] = (x >> 8) % 4;
        i = i + 1;
    }
    let mut prev: Vec<i64> = vec![0i64; (n + 1) as usize];
    let mut cur: Vec<i64> = vec![0i64; (n + 1) as usize];
    i = 0;
    while i <= n {
        prev[(i) as usize] = i;
        i = i + 1;
    }
    i = 1;
    while i <= n {
        cur[(0) as usize] = i;
        let mut j: i64 = 1;
        while j <= n {
            let mut cost: i64 = 1;
            if a[(i - 1) as usize] == b[(j - 1) as usize] {
                cost = 0;
            }
            let mut best: i64 = prev[(j - 1) as usize] + cost;
            if prev[(j) as usize] + 1 < best {
                best = prev[(j) as usize] + 1;
            }
            if cur[(j - 1) as usize] + 1 < best {
                best = cur[(j - 1) as usize] + 1;
            }
            cur[(j) as usize] = best;
            j = j + 1;
        }
        j = 0;
        while j <= n {
            prev[(j) as usize] = cur[(j) as usize];
            j = j + 1;
        }
        i = i + 1;
    }
    return prev[(n) as usize];
}

fn gcd(a: i64, b: i64) -> i64 {
    let mut x: i64 = a;
    let mut y: i64 = b;
    while y != 0 {
        let mut t: i64 = x % y;
        x = y;
        y = t;
    }
    return x;
}

fn gcdsum(n: i64) -> i64 {
    let mut s: i64 = 0;
    let mut i: i64 = 1;
    while i <= n {
        s = s + gcd(i, n - i + 1);
        i = i + 1;
    }
    return s;
}

fn lcg(n: i64) -> i64 {
    let mut x: i64 = 1;
    let mut s: i64 = 0;
    let mut i: i64 = 0;
    while i < n {
        x = (x * 1103515245 + 12345) % 2147483648;
        s = (s + (x >> 16)) % 1000000007;
        i = i + 1;
    }
    return s;
}

fn binsearch(n: i64) -> i64 {
    let mut a: Vec<i64> = vec![0i64; (n) as usize];
    let mut i: i64 = 0;
    while i < n {
        a[(i) as usize] = i * 3;
        i = i + 1;
    }
    let mut found: i64 = 0;
    i = 0;
    while i < n {
        let mut key: i64 = (i * 7) % (n * 3);
        let mut lo: i64 = 0;
        let mut hi: i64 = n - 1;
        while lo <= hi {
            let mut mid: i64 = (lo + hi) / 2;
            if a[(mid) as usize] == key {
                found = found + 1;
                lo = hi + 1;
            } else {
                if a[(mid) as usize] < key {
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
        }
        i = i + 1;
    }
    return found;
}

fn prefix(n: i64) -> i64 {
    let mut a: Vec<i64> = vec![0i64; (n + 1) as usize];
    let mut i: i64 = 1;
    while i <= n {
        a[(i) as usize] = a[(i - 1) as usize] + (i * i) % 13;
        i = i + 1;
    }
    let mut q: i64 = 0;
    i = 1;
    while i <= n {
        let mut lo: i64 = (i * 7) % n;
        let mut hi: i64 = lo + (i % 50);
        if hi > n {
            hi = n;
        }
        q = (q + a[(hi) as usize] - a[(lo) as usize]) % 1000000007;
        i = i + 1;
    }
    return q;
}

fn popcnt(n: i64) -> i64 {
    let mut t: i64 = 0;
    let mut i: i64 = 0;
    while i < n {
        let mut x: i64 = i;
        while x != 0 {
            x = x & (x - 1);
            t = t + 1;
        }
        i = i + 1;
    }
    return t;
}

fn fib(n: i64) -> i64 {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn queens_rec(n: i64, row: i64, cols: i64, d1: i64, d2: i64) -> i64 {
    if row == n {
        return 1;
    }
    let mut count: i64 = 0;
    let mut c: i64 = 0;
    while c < n {
        let mut b1: i64 = 1 << c;
        let mut b2: i64 = 1 << (row + c);
        let mut b3: i64 = 1 << (row - c + n);
        if (cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0 {
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3);
        }
        c = c + 1;
    }
    return count;
}

fn queens(n: i64) -> i64 {
    return queens_rec(n, 0, 0, 0, 0);
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", (sumsq(n * 30000) + collatz(n * 400) + sieve(n * 15000) + matmul(n * 1) + sort(n * 40) + hashtab(n * 3000) + editdist(n * 6) + gcdsum(n * 1500) + lcg(n * 30000) + binsearch(n * 1500) + prefix(n * 30000) + popcnt(n * 3000) + fib(12) + queens(6)) % 1000000007);
}
