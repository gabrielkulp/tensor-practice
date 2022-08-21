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
    println!(
        "{}",
        coords_cmp(&vec![3, 5, 6], |a, b| a < b, &vec![4, 5, 6])
    );
    let mut bpt: BPTree = BPTree::new();
    assert!(bpt.get(&vec![0]) == None);
    bpt.insert(vec![0], 3.0);
    bpt.insert(vec![1], 4.0);
    bpt.insert(vec![2], 8.0);
    bpt.insert(vec![3], 12.0);
    bpt.insert(vec![4], 99.0);
    bpt.insert(vec![5], 99.0);
}
