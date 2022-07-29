#![allow(dead_code)]
use std::mem::size_of;
mod b_tree;
mod bp_tree;
mod hash_table;

pub use b_tree::BTree;
pub use bp_tree::BPTree;
pub use hash_table::HashTable;

// change tensor precision here
pub type Value = f32;
pub type Mode = u16;
pub type Coords = Vec<Mode>;
pub type Key = u64;

// apparently there's no way to generically say a trait requires .iter()
// so instead I wrapped the options into an enum. Seems to work fine
// but I hope it's as performant as a straight .iter()
pub enum ContainerIterator<'a> {
    BTree(std::collections::btree_map::Iter<'a, Coords, Value>),
    BPTree(bp_tree::Iter<'a>),
    HashTable(std::collections::hash_map::Iter<'a, Coords, Value>),
}

impl<'a> Iterator for ContainerIterator<'a> {
    type Item = (&'a Coords, &'a Value);
    fn next(&mut self) -> Option<(&'a Coords, &'a Value)> {
        match self {
            ContainerIterator::BTree(i) => i.next(),
            ContainerIterator::BPTree(i) => i.next(),
            ContainerIterator::HashTable(i) => i.next(),
        }
    }
}

pub trait KeyVal {
    fn new() -> Self;
    fn insert(&mut self, key: Coords, val: Value) -> ();
    fn get(&self, key: &Coords) -> Option<&Value>;
    fn iter(&self) -> ContainerIterator;
}

fn serialize_coords(coords: Coords) -> Key {
    assert!(coords.len() <= size_of::<Key>() / size_of::<Mode>());
    let mut out: u64 = 0;
    for c in coords {
        out <<= size_of::<Mode>();
        out |= c as u64;
    }
    out
}

fn deserialize_coords(order: usize, key: Key) -> Coords {
    let mut tmp = key;
    let mut coords: Coords = Vec::new();
    let mask = !0 as Mode;
    for _ in 0..order {
        coords.push((tmp & (mask as Key)) as Mode);
        tmp >>= size_of::<Mode>();
    }
    coords
}
