#![allow(warnings)]

struct Cell { value: i64, _other: i64 }
fn churn(n: i64) -> i64 {
    let mut slots: Vec<Option<Box<Cell>>> = (0..64).map(|_| None).collect();
    let mut total = 0;
    for i in 0..n {
        let cell = Box::new(Cell { value: i, _other: 0 });
        let k = (i % 64) as usize;
        if let Some(old) = slots[k].take() { total = (total + old.value) % 1000000007; }
        slots[k] = Some(cell);
    }
    for slot in slots.iter_mut() {
        if let Some(old) = slot.take() { total = (total + old.value) % 1000000007; }
    }
    total
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", churn(n));
}
