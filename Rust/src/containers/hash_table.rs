use super::{ContainerIterator, Coords, KeyVal, Value};
use std::collections::HashMap;
pub type HashTable = HashMap<Coords, Value>;

impl KeyVal for HashTable {
    fn new() -> Self {
        HashMap::new()
    }
    fn insert(&mut self, k: Coords, v: Value) -> () {
        HashMap::insert(self, k, v);
    }
    fn get(&self, k: &Coords) -> Option<&Value> {
        HashMap::get(&self, k)
    }
    fn iter(&self) -> ContainerIterator {
        ContainerIterator::HashTable(HashMap::iter(self))
    }
}
