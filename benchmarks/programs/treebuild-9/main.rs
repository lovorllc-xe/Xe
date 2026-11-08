#![allow(warnings)]

struct Tree { left: Option<Box<Tree>>, right: Option<Box<Tree>> }
fn treemake(depth: i64) -> Box<Tree> {
    if depth > 0 { Box::new(Tree { left: Some(treemake(depth - 1)), right: Some(treemake(depth - 1)) }) }
    else { Box::new(Tree { left: None, right: None }) }
}
fn treecount(t: &Tree) -> i64 {
    match (&t.left, &t.right) {
        (Some(l), Some(r)) => 1 + treecount(l) + treecount(r),
        _ => 1,
    }
}
fn treebuild(n: i64) -> i64 {
    let mut total = 0;
    for _ in 0..4 { total += treecount(&treemake(n)); }
    total
}

fn main() {
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", treebuild(n));
}
