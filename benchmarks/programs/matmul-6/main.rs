#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", matmul(n));
}
