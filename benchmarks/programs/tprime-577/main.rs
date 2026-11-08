#![allow(warnings)]

fn tprime_worker(lo: i64, hi: i64) -> i64 {
    let mut s: i64 = 0;
    for x in lo..hi {
        if x >= 2 {
            let mut prime = 1;
            let mut d = 2;
            while d * d <= x && prime == 1 {
                if x % d == 0 { prime = 0; }
                d += 1;
            }
            s += prime;
        }
    }
    s
}
fn tprime(n: i64) -> i64 {
    let mut handles = Vec::new();
    for t in 0..8 as i64 {
        let lo = n * t / 8; let hi = n * (t + 1) / 8;
        handles.push(std::thread::spawn(move || tprime_worker(lo, hi)));
    }
    let mut total = 0;
    for h in handles { total = (total + h.join().unwrap()) % 1000000007; }
    total
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", tprime(n));
}
