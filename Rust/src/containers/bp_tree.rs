#![allow(dead_code)]
use super::{coords_eq, coords_le};
use super::{ContainerIterator, Coords, KeyVal, Value};
use std::borrow::Borrow;

const ORDER: usize = 32;
type NodeItem = (Coords, Box<Node>);
type ValueItem = (Coords, Value);
const INIT_NODE_ITEM: Option<NodeItem> = None;
const INIT_VALUE_ITEM: Option<ValueItem> = None;

enum Node {
    Internal(Internal),
    Leaf(Leaf),
}

struct Internal {
    child_count: usize,
    children: [Option<NodeItem>; ORDER],
}

struct Leaf {
    child_count: usize,
    values: [Option<ValueItem>; ORDER],
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
            Node::Leaf(leaf) => {
                for (coords, value) in leaf.values.iter().flatten() {
                    if coords_eq(coords, key) {
                        return Some(&value);
                    }
                }
                None
            }
            Node::Internal(i) => {
                for (c, b) in i.children.iter().flatten() {
                    if coords_le(key, c) {
                        return Node::leaf_search(b, key);
                    }
                }
                None
            }
        }
    }

    fn leaf_insert(node: &'a mut Box<Node>, key: &Coords, value: Value) -> Option<NodeItem> {
        /*
        first: recursive search for the right node to add value to.
           takes: value, node to check (if it's last or a step)
           must stop recursing on leaf
           replace leaf with new value? Maybe add it.
        */

        /*
        second: tail of recursion might split node
             which requires adding the new node to the parent
             which could cause the parent to split!
           returns entry to add to parent up the call stack
        */
        match &mut **node {
            Node::Leaf(leaf) => {
                // base case
                if leaf.child_count < ORDER {
                    // there's room to insert, but it might require shifting existing entries
                    let mut shift = false;
                    let mut temp: Option<ValueItem> = None;

                    for ovi in leaf.values.iter_mut() {
                        if shift {
                            if let None = ovi {
                                break;
                            }
                            temp = ovi.replace(temp.unwrap());
                            continue;
                        }
                        if let Some((coords, _)) = ovi {
                            if !coords_le(key, &coords) {
                                // insert here, and shift other entries
                                shift = true;
                                temp = ovi.replace((key.clone(), value));
                            }
                            continue;
                        } else {
                            // insert it right here
                            ovi.replace((key.clone(), value));
                            break;
                        }
                    }
                    None
                } else {
                    // already full so make a new leaf node
                    let mut new_leaf = Leaf {
                        child_count: ORDER / 2,
                        values: [INIT_VALUE_ITEM; ORDER],
                    };

                    // copy half of old entries to new leaf
                    new_leaf.values[0] = Some((key.clone(), value));
                    let half = ORDER / 2;
                    for i in 0..half {
                        new_leaf.values[i + 1] = leaf.values[half + i].take()
                    }

                    // return pointer to this new leaf
                    return Some((key.clone(), Box::new(Node::Leaf(new_leaf))));
                }
            }
            Node::Internal(_) => {
                todo!()
            }
        }
    }
}

impl<'a> KeyVal for BPTree {
    fn new() -> Self {
        BPTree {
            node_count: 0,
            root: Box::new(Node::Leaf(Leaf {
                child_count: 0,
                values: [INIT_VALUE_ITEM; ORDER],
            })),
        }
    }
    fn insert(&mut self, key: Coords, value: Value) -> () {
        let new_root = Node::leaf_insert(&mut self.root, &key, value);
        match new_root {
            None => println!("new root is same as old root"),
            Some((k, v)) => {
                println!("new root with k={:?}", k);
                self.root = v;
            }
        }
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
