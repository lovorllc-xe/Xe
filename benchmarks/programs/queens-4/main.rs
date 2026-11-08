#![allow(warnings)]

fn queens_rec(n: i64, row: i64, cols: i64, d1: i64, d2: i64) -> i64 {
    if row == n {
        return 1;
    }
    let mut count: i64 = 0;
    let mut c: i64 = 0;
    while c < n {
        let mut b1: i64 = 1 << c;
        let mut b2: i64 = 1 << (row + c);
        let mut b3: i64 = 1 << (row - c + n);
        if (cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0 {
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3);
        }
        c = c + 1;
    }
    return count;
}

fn queens(n: i64) -> i64 {
    return queens_rec(n, 0, 0, 0, 0);
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", queens(n));
}
