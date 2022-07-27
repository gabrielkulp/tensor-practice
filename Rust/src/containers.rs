use crate::tensor::{Coords, Value};

use std::collections::BTreeMap;
use std::collections::HashMap;
pub enum ContainerIterator<'a> {
    BTreeMap(std::collections::btree_map::Iter<'a, Coords, Value>),
    HashMap(std::collections::hash_map::Iter<'a, Coords, Value>),
}

impl<'a> Iterator for ContainerIterator<'a> {
    type Item = (&'a Coords, &'a Value);
    fn next(&mut self) -> Option<(&'a Coords, &'a Value)> {
        match self {
            ContainerIterator::BTreeMap(i) => i.next(),
            ContainerIterator::HashMap(i) => i.next(),
        }
    }
}

pub trait KeyVal {
    fn new() -> Self;
    fn insert(&mut self, key: Coords, val: Value) -> ();
    fn get(&self, key: &Coords) -> Option<&Value>;
    fn iter(&self) -> ContainerIterator;
}

impl KeyVal for BTreeMap<Coords, Value> {
    fn new() -> Self {
        BTreeMap::new()
    }
    fn insert(&mut self, key: Coords, val: Value) -> () {
        BTreeMap::insert(self, key, val);
    }
    fn get(&self, key: &Coords) -> Option<&Value> {
        BTreeMap::get(&self, key)
    }
    fn iter(&self) -> ContainerIterator {
        ContainerIterator::BTreeMap(BTreeMap::iter(self))
    }
}

impl KeyVal for HashMap<Coords, Value> {
    fn new() -> Self {
        HashMap::new()
    }
    fn insert(&mut self, key: Coords, val: Value) -> () {
        HashMap::insert(self, key, val);
    }
    fn get(&self, key: &Coords) -> Option<&Value> {
        HashMap::get(&self, key)
    }
    fn iter(&self) -> ContainerIterator {
        ContainerIterator::HashMap(HashMap::iter(self))
    }
}
