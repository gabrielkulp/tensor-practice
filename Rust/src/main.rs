mod tensor;

fn main() {
    let a = match tensor::read("../T.coo") {
		Ok(val) => val,
		Err(e) => panic!("Failed to read tensor: {}", e),
	};
    let b = a.clone();

    println!("{}", a);
    println!("Trace with 0, 1 is\n{}", a.trace(0, 1));
    println!("Trace with 0, 2 is\n{}", a.trace(0, 2));
    println!("Trace with 1, 2 is\n{}", a.trace(1, 2));

    let c = a.contract(0, &b, 1);
    println!("contracted with a clone on 0,1 yields\n{}", c);
    c.write("C.coo").unwrap();
}
