#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", editdist(n));
}
