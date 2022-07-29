#![allow(dead_code)]
use crate::{ContainerIterator, Coords, KeyVal, Value};

const ORDER: usize = 32;

enum Node {
    Internal(Internal),
    Leaf(Leaf),
    Nil,
}

struct Internal {
    child_count: usize,
    children: [Box<Node>; ORDER],
    keys: [u8; ORDER],
}

struct Leaf {
    value: Value,
}

pub struct BPTree {
    node_count: usize,
    root: Box<Node>,
}

pub struct Iter<'a> {
    tree: &'a BPTree,
}

impl<'a> Iterator for Iter<'a> {
    type Item = (&'a Coords, &'a Value);
    fn next(&mut self) -> Option<Self::Item> {
        todo!("B+ tree iterator");
    }
}

impl<'a> KeyVal for BPTree {
    fn new() -> Self {
        BPTree {
            node_count: 0,
            root: Box::new(Node::Nil),
        }
    }
    fn insert(&mut self, key: Coords, value: Value) -> () {
        todo!("b+ tree insert")
    }
    fn get(&self, key: &Coords) -> Option<&Value> {
        todo!("b+ tree get")
    }
    fn iter(&self) -> ContainerIterator<'_> {
        ContainerIterator::BPTree(Iter { tree: self })
    }
}
