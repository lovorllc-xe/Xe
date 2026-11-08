#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", binsearch(n));
}
