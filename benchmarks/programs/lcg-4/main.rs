#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", lcg(n));
}
