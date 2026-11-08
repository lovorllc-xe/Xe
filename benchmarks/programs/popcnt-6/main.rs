#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", popcnt(n));
}
