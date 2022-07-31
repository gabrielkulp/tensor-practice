use super::{ContainerIterator, Coords, KeyVal, Value};
use std::collections::BTreeMap;
pub type BTree = BTreeMap<Coords, Value>;

impl KeyVal for BTree {
    fn new() -> Self {
        BTreeMap::new()
    }
    fn insert(&mut self, key: Coords, value: Value) -> () {
        BTreeMap::insert(self, key, value);
    }
    fn get(&self, key: &Coords) -> Option<&Value> {
        BTreeMap::get(&self, key)
    }
    fn iter(&self) -> ContainerIterator {
        ContainerIterator::BTree(BTreeMap::iter(self))
    }
}
