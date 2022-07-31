#![allow(dead_code)]
use super::coords_le;
use super::{ContainerIterator, Coords, KeyVal, Value};
use std::borrow::Borrow;

const ORDER: usize = 32;

enum Node {
    Internal(Internal),
    Leaf(Value),
    Nil,
}

struct Internal {
    child_count: usize,
    children: [Option<(Coords, Box<Node>)>; ORDER],
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

impl<'a> Node {
    fn leaf_search(node: &'a Node, key: &Coords) -> Option<&'a Value> {
        match node {
            Node::Nil => None,
            Node::Leaf(v) => Some(v),
            Node::Internal(i) => {
                for n in i.children.iter() {
                    match n {
                        None => continue,
                        Some((c, b)) => {
                            if coords_le(key, c) {
                                return Node::leaf_search(b, key);
                            }
                        }
                    }
                }
                None
            }
        }
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
        match Node::leaf_search(self.root.borrow(), key) {
            Some(v) => Some(&(*v)),
            None => None,
        }
    }
    fn iter(&self) -> ContainerIterator<'_> {
        ContainerIterator::BPTree(Iter { tree: self })
    }
}
