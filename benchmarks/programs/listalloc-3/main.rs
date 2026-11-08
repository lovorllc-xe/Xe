#![allow(warnings)]

struct Node { value: i64, next: Option<Box<Node>> }
fn listalloc(n: i64) -> i64 {
    let mut head: Option<Box<Node>> = None;
    for i in 0..n { head = Some(Box::new(Node { value: i, next: head.take() })); }
    let mut total = 0;
    let mut cur = head;
    while let Some(mut node) = cur {
        total = (total + node.value) % 1000000007;
        cur = node.next.take();
    }
    total
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", listalloc(n));
}
