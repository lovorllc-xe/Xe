#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", sieve(n));
}
