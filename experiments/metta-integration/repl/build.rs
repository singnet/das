fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::compile_protos("/home/user/das-proto/atom_space_node.proto")?;
    Ok(())
}

