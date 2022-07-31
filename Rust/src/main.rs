mod containers;
mod tensor;
use crate::containers::*;
//use crate::tensor::Tensor;

fn main() {
    /*
    let a: Tensor<BPTree> = Tensor::read("../T.coo").expect("import tensor from file");
    let b = Tensor::read("../T.coo").expect("import tensor from file");

    println!("{}", a);
    println!("Trace with 0, 1 is\n{}", a.trace(0, 1));
    println!("Trace with 0, 2 is\n{}", a.trace(0, 2));
    println!("Trace with 1, 2 is\n{}", a.trace(1, 2));

    let c = Tensor::contract(&a, 0, &b, 1);
    print!("contracted with a clone on modes 0, 1 yields\n{}", c);
    c.write("C.coo").expect("export tensor to file");
    */
    let bpt: BPTree = BPTree::new();
    let c: Coords = vec![0];
    assert!(bpt.get(&c) == None)
}
