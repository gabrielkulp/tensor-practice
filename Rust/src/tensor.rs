// trait that a tensor's key-value store must implement
use crate::containers::KeyVal;

// file IO stuff
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;

// change tensor precision here
pub type Value = f32;
pub type Coords = Vec<u32>;

#[derive(Clone)]
pub struct Tensor<T: KeyVal> {
    shape: Coords,
    values: T,
}

#[allow(dead_code)]
impl<T: KeyVal> Tensor<T> {
    pub fn order(&self) -> usize {
        self.shape.len()
    }

    pub fn zero(shape: &Coords) -> Tensor<T> {
        Tensor {
            shape: shape.clone(),
            values: T::new(),
        }
    }

    pub fn scalar(value: Value) -> Tensor<T> {
        let mut t = Tensor::zero(&vec![]);
        t.insert(&vec![], value);
        t
    }

    pub fn insert(&mut self, coords: &Coords, value: Value) {
        // note that this won't let you set an existing entry to 0
        assert!(coords.len() == self.order());
        for (c, s) in coords.iter().zip(&self.shape) {
            assert!(c < &s);
        }
        if value != 0.0 {
            self.values.insert(coords.clone(), value);
        }
    }

    pub fn get(&self, coords: &Coords) -> Value {
        for (c, s) in coords.iter().zip(&self.shape) {
            assert!(c < &s);
        }
        self.values.get(coords).unwrap_or(&(0 as Value)).clone()
    }

    pub fn add(&mut self, coords: &Coords, value: Value) {
        assert!(coords.len() == self.order());
        for (c, s) in coords.iter().zip(&self.shape) {
            assert!(c < &s);
        }
        if value != 0.0 {
            self.values.insert(coords.clone(), self.get(coords) + value);
        }
    }

    pub fn trace(&self, idx_a: usize, idx_b: usize) -> Tensor<T> {
        assert!(idx_a < self.order(), "idx_a out of range");
        assert!(idx_b < self.order(), "idx_b out of range");
        assert!(idx_a != idx_b, "trace indices can't match");

        let mut c_coords = self.shape.clone();
        let k_max = c_coords.remove(std::cmp::max(idx_a, idx_b));
        assert!(
            k_max == c_coords.remove(std::cmp::min(idx_a, idx_b)),
            "trace modes don't match"
        );

        let mut c = Tensor::zero(&c_coords);

        let mut t_coords: Coords = self.shape.iter().map(|_| 0).collect();
        c_coords = c_coords.iter().map(|_| 0).collect();

        let mut done = c.order() == 0;
        loop {
            let mut acc = 0.0;
            for k in 0..k_max {
                t_coords[idx_a] = k;
                t_coords[idx_b] = k;
                acc += self.get(&t_coords);
            }
            c.insert(&c_coords, acc);

            // increment coordinates
            let mut c_mode = 0;
            let mut t_mode = 0;
            loop {
                if t_mode == idx_a || t_mode == idx_b {
                    t_mode += 1;
                    continue;
                }
                c_coords[c_mode] += 1;
                t_coords[t_mode] += 1;
                if t_coords[t_mode] == self.shape[t_mode] {
                    c_coords[c_mode] = 0;
                    t_coords[t_mode] = 0;
                    c_mode += 1;
                    t_mode += 1;
                    if c_mode == c.order() {
                        done = true;
                        break;
                    }
                    continue;
                }
                break;
            }
            if done {
                break;
            }
        }
        return c;
    }

    pub fn contract(&self, idx_a: usize, other: &Tensor<T>, idx_b: usize) -> Tensor<T> {
        let a = self;
        let b = other;
        assert!(idx_a < a.order(), "idx_a out of range");
        assert!(idx_b < b.order(), "idx_b out of range");

        let mut c_coords = a.shape.clone();
        c_coords.append(&mut b.shape.clone());

        let k_max = c_coords.remove(a.order() + idx_b);
        assert!(
            k_max == c_coords.remove(idx_a),
            "contract modes don't match"
        );
        let mut c = Tensor::zero(&c_coords);

        let mut a_coords: Coords = a.shape.iter().map(|_| 0).collect();
        let mut b_coords: Coords = b.shape.iter().map(|_| 0).collect();
        c_coords = c_coords.iter().map(|_| 0).collect();

        let mut done = c.order() == 0;
        loop {
            let mut acc = 0.0;
            for k in 0..k_max {
                a_coords[idx_a] = k;
                b_coords[idx_b] = k;
                acc += a.get(&a_coords) * b.get(&b_coords);
            }
            c.insert(&c_coords, acc);

            let mut a_mode = 0;
            let mut b_mode;
            let mut c_mode = 0;
            loop {
                if a_mode == idx_a || a_mode == a.order() + idx_b {
                    a_mode += 1;
                    continue;
                }
                c_coords[c_mode] += 1;
                if c_mode < a.order() - 1 {
                    a_coords[a_mode] += 1;
                    if c_coords[c_mode] == c.shape[c_mode] {
                        a_coords[a_mode] = 0;
                        c_coords[c_mode] = 0;
                        c_mode += 1;
                        a_mode += 1;
                        continue;
                    }
                    break;
                } else {
                    // increment B part of C coord
                    b_mode = a_mode - a.order();
                    b_coords[b_mode] += 1;
                    if c_coords[c_mode] == c.shape[c_mode] {
                        if c_mode == c.order() - 1 {
                            done = true;
                            break;
                        }
                        b_coords[b_mode] = 0;
                        c_coords[c_mode] = 0;
                        c_mode += 1;
                        a_mode += 1;
                        continue;
                    }
                    break;
                }
            }
            if done {
                break;
            }
        }
        c
    }

    pub fn read(path: &str) -> Result<Tensor<T>, std::io::Error> {
        let file = File::open(Path::new(path))?;
        let mut lines = std::io::BufReader::new(file).lines();

        // read order
        let mut line: String = lines.next().expect("read order")?;
        let order: usize = line.split(": ").collect::<Vec<&str>>()[1]
            .trim()
            .parse::<usize>()
            .expect("parse order");

        // read shape
        line = lines.next().expect("read shape")?;
        let shape_str = line.split(": ").collect::<Vec<&str>>()[1].trim();
        let mut shape: Coords = vec![];
        for s in shape_str.split(", ") {
            shape.push(s.parse().expect("parse shape"));
        }
        assert!(shape.len() == order, "tensor size metadata mismatch");

        // allocate new tensor
        let mut t = Tensor::zero(&shape);

        // read values line-by-line
        _ = lines.next().expect("read coordinates and value")?;
        let mut coords = shape.clone();
        for l in lines {
            let line = l?;
            let vals = line.split(", ").collect::<Vec<&str>>();
            for m in 0..order {
                coords[m] = vals[m].parse().expect("parse coordinates");
            }
            t.insert(&coords, vals[vals.len() - 1].parse().expect("parse value"));
        }
        Ok(t)
    }

    pub fn write(&self, path: &str) -> Result<(), std::io::Error> {
        let mut file = File::create(Path::new(path))?;
        write!(file, "order: {}\n", self.order())?;
        write!(
            file,
            "shape: {}\n",
            self.shape
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<String>>()
                .join(", ")
        )?;
        write!(file, "values:\n")?;
        for (k, v) in self.values.iter() {
            write!(
                file,
                "{}, {}\n",
                k.iter()
                    .map(|x| x.to_string())
                    .collect::<Vec<String>>()
                    .join(", "),
                v
            )?;
        }
        Ok(())
    }
}

#[allow(dead_code)]
impl<T: KeyVal> std::fmt::Display for Tensor<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "shape is {:?}\ncontents is:", self.shape)?;
        for (k, v) in self.values.iter() {
            write!(f, "\n  {:?} = {}", k, v)?;
        }
        write!(f, "\n")
    }
}
