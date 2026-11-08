#![allow(warnings)]

fn tsum_worker(lo: i64, hi: i64) -> i64 {
    let mut s: i64 = 0;
    for i in lo..hi { s += (i * i) % 7; }
    s
}
fn tsum(n: i64) -> i64 {
    let mut handles = Vec::new();
    for t in 0..4 as i64 {
        let lo = n * t / 4; let hi = n * (t + 1) / 4;
        handles.push(std::thread::spawn(move || tsum_worker(lo, hi)));
    }
    let mut total = 0;
    for h in handles { total = (total + h.join().unwrap()) % 1000000007; }
    total
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", tsum(n));
}
