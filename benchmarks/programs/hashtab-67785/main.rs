#![allow(warnings)]

fn hashtab(n: i64) -> i64 {
    let mut cap: i64 = 16;
    while cap < n * 2 {
        cap = cap * 2;
    }
    let mut keys: Vec<i64> = vec![0i64; (cap) as usize];
    let mut vals: Vec<i64> = vec![0i64; (cap) as usize];
    let mut i: i64 = 1;
    while i <= n {
        let mut k: i64 = (i * 2654435761) % 4294967296;
        let mut h: i64 = k & (cap - 1);
        while keys[(h) as usize] != 0 && keys[(h) as usize] != k {
            h = (h + 1) & (cap - 1);
        }
        keys[(h) as usize] = k;
        vals[(h) as usize] = vals[(h) as usize] + i;
        i = i + 1;
    }
    let mut total: i64 = 0;
    i = 1;
    while i <= n {
        let mut k: i64 = (i * 2654435761) % 4294967296;
        let mut h: i64 = k & (cap - 1);
        while keys[(h) as usize] != k {
            h = (h + 1) & (cap - 1);
        }
        total = (total + vals[(h) as usize]) % 1000000007;
        i = i + 1;
    }
    return total;
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", hashtab(n));
}
