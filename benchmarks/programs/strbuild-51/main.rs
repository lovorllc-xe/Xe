#![allow(warnings)]

fn strbuild(n: i64) -> i64 {
    let mut s = String::new();
    for i in 0..n {
        s.push((97 + (i % 26) as u8) as char);
    }
    let mut c = 0;
    for b in s.bytes() {
        if b == 97 { c += 1; }
    }
    c
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", strbuild(n));
}
