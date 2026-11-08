#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", prefix(n));
}
