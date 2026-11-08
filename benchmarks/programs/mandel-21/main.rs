#![allow(warnings)]

fn mandel(n: i64) -> i64 {
    let mut count: i64 = 0;
    let mut py: i64 = 0;
    while py < n {
        let mut px: i64 = 0;
        while px < n {
            let mut cx: f64 = (px) as f64 * 3.0 / (n) as f64 - 2.0;
            let mut cy: f64 = (py) as f64 * 2.0 / (n) as f64 - 1.0;
            let mut x: f64 = 0.0;
            let mut y: f64 = 0.0;
            let mut it: i64 = 0;
            let mut inside: i64 = 1;
            while it < 40 && inside == 1 {
                let mut t: f64 = x * x - y * y + cx;
                y = 2.0 * x * y + cy;
                x = t;
                if x * x + y * y > 4.0 {
                    inside = 0;
                }
                it = it + 1;
            }
            count = count + inside;
            px = px + 1;
        }
        py = py + 1;
    }
    return count;
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", mandel(n));
}
