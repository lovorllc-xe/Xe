#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", gcdsum(n));
}
