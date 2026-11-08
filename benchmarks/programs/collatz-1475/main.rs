#![allow(warnings)]

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

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", collatz(n));
}
