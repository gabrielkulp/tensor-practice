#![allow(dead_code)]
use super::{coords_lt, coords_gt, coords_eq, coords_le};
use super::{ContainerIterator, Coords, KeyVal, Value};
use std::borrow::Borrow;

const ORDER: usize = 4; //32; // make it small for testing
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
                    if coords_lt(key, c) {
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
            // Base case: we've reached a leaf node
            Node::Leaf(leaf) => {
                if leaf.child_count < ORDER {
                    leaf.child_count += 1;
                    // There's room to insert, but it might require shifting existing entries.
                    // Do one pass through children, inserting the new value at the right spot,
                    // then shifting over all the remaining children entries to make room.
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
                            if coords_gt(key, &coords) {
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
                    println!("T1");
                    return None;
                } else {
                    // There is no room to add the value to the current leaf node,
                    // so instead make a new leaf node and divide all the entries between them.
                    // Then return this new leaf up the call stack to be added to a parent.
                    let half = ORDER / 2;
                    let mut new_leaf = Leaf {
                        child_count: half,
                        values: [INIT_VALUE_ITEM; ORDER],
                    };

                    // copy half of old entries to new leaf
                    new_leaf.values[0] = Some((key.clone(), value));
                    for i in 0..half {
                        new_leaf.values[i + 1] = leaf.values[half + i].take()
                    }
                    leaf.child_count = ORDER - half;

                    // return pointer to this new leaf
                    return Some((key.clone(), Box::new(Node::Leaf(new_leaf))));
                }
            }

            // Recursive case: node isn't a leaf
            Node::Internal(parent) => {
                // first check which child to insert to
                
                let left = 0;
                let right = parent.child_count-1;
                let child_idx = 0;
                while left <= right {
                    child_idx = (left + right)/2;
                    if let Some(child) = parent.children[child_idx] {
                        if coords_lt(&child.0, key) {
                            left = child_idx + 1;
                        } else if coords_gt(&child.0, key) {
                            right = child_idx - 1;
                        } else {
                            break;
                        }
                    } else {
                        panic!("node has unexpected 'None' child");
                    }
                }
                
                // if insertion returned a new node, add it as a child
                let (node_coords, node_box) = parent.children[child_idx].unwrap();
                if let Node::
                if node.child_count < ORDER {
                    node.child_count += 1;
                    // There's room to insert, but we'll need to shift entries
                    let mut shift = false;
                    let mut temp
                    
                } else {
                    // There is no room, so split to a new node
                }
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
        let new_node = Node::leaf_insert(&mut self.root, &key, value);
        if let Some((k, v)) = new_node {
            println!("new node with k={:?}", k);
            let mut new_root = Internal {
                child_count: 0,
                children: [INIT_NODE_ITEM; ORDER],
            };
            // assign a temporary value so we can "move" the root with a swap
            let mut old_root = Box::new(Node::Leaf(Leaf {
                child_count: 0,
                values: [INIT_VALUE_ITEM; ORDER],
            }));
            std::mem::swap(&mut old_root, &mut self.root);

            let old_extent = match &*old_root {
                Node::Leaf(l) => l.values[0].as_ref().unwrap().0.clone(),
                Node::Internal(i) => i.children[0].as_ref().unwrap().0.clone(),
            };
            new_root.children[0] = Some((old_extent, old_root));
            new_root.children[1] = Some((k.clone(), v));
            new_root.child_count = 2;
            self.root = Box::new(Node::Internal(new_root));
        } else {
            println!("same root");
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
