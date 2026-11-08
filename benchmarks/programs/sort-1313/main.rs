#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", sort(n));
}
