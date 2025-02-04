# Simple and rudimentary  Metta Interpreter hook to DAS query

Idea is to explore the Metta Interpreter and write a function that issues a
query to a remote DAS, then print it's result.

## How to

First override the copies listed below with the files on this folder.

NOTE: `Cargo.toml` and `build.rs` are needed in both `lib` and `repl` folders.

```txt
lib/Cargo.toml
lib/build.rs
lib/src/metta/runner/stdlib/das.rs
lib/src/metta/runner/stdlib/mod.rs

repl/Cargo.toml
repl/build.rs
repl/src/main.rs

Dockerfile
```

After moving the files, build the Docker image using:

```sh
docker build --target build -t trueagi/hyperon .
```

Than run the image using the following command:
NOTE: We assume that the query agent is running according to the DAS
documentation. Please make sure that the query agent port is the same
configured in the das.rs file (default 31700)

```sh
docker run -it --rm \
  --volume .:/home/user/hyperon-experimental \
  --network host \
  trueagi/hyperon:latest
```

Inside the container start the metta-repl and invoke das function.
NOTE: the das! function will ignore any parameters passed to it, and will
always make the same query, see  `das.rs`.

```sh
cargo run
!(das!("H"))
```

Output example:

```log
Visit https://metta-lang.dev/ for tutorials.
Execute !(help!) to get list of the standard library functions.
Spawning DAS gRPC Server on port 9090.
Inside gRPC server thread
> !(das!("H"))
Sending Query to DAS
Response: Response { metadata: MetadataMap { headers: {"content-type": "application/grpc", "grpc-accept-encoding": "identity, deflate, gzip", "grpc-status": "0"} }, message: Empty, extensions: Extensions }
[()]
> MessageData: ["localhost:60016"]
Unknwon: "node_joined_network"
MessageData: ["0.0000000000 1 05bfd9bb16514f9f95784092431896cd 2 V1 ae7ab1a9791ca0dcbf54ca7955bbdbc9 V2 6872b1cd2cfbc483ac52687852e5f1ad ", "0.0000000000 1 06b1200d19402202e31e059c15c51c36 2 V1 25bdf4cba0b59adfa07dd103d033bca9 V2 1fc9300891b7a5d6583f0f85a83b9ddb ", "0.0000000000 1 289476dbf62faffc149e3658bc368b6b 2 V1 8860480382d0ddf62623abf5c860e51d V2 a408f6dd446cdd4fa56f82e77fe6c870 ", "0.0000000000 1 2e2f6ce7fc5cab22d57d38337bb5d255 2 V1 8860480382d0ddf62623abf5c860e51d V2 3225ea795289574ceee32e091ad54ef4 ", "0.0000000000 1 49e5f2b5d6580eec3a8d7014d542d6f0 2 V1 665509d366ac3c2821b3b6b266f996bd V2 25bdf4cba0b59adfa07dd103d033bca9 ", "0.0000000000 1 68ea071c32d4dbf0a7d8e8e00f2fb823 2 V1 3225ea795289574ceee32e091ad54ef4 V2 8860480382d0ddf62623abf5c860e51d ", "0.0000000000 1 7ec8526b8c8f15a6ac55273fedbf694f 2 V1 8860480382d0ddf62623abf5c860e51d V2 181a19436acef495c8039a610be59603 ", "0.0000000000 1 8cf06d71b3ddcd9e78b55b4257f7b1cf 2 V1 181a19436acef495c8039a610be59603 V2 8860480382d0ddf62623abf5c860e51d ", "0.0000000000 1 a2229df191806a5d97892754caae1535 2 V1 25bdf4cba0b59adfa07dd103d033bca9 V2 665509d366ac3c2821b3b6b266f996bd ", "0.0000000000 1 a44654b990acba107216f434d80d685c 2 V1 6872b1cd2cfbc483ac52687852e5f1ad V2 ae7ab1a9791ca0dcbf54ca7955bbdbc9 ", "0.0000000000 1 ba86c981af6ba28249c48d49381491a1 2 V1 181a19436acef495c8039a610be59603 V2 3225ea795289574ceee32e091ad54ef4 ", "0.0000000000 1 d610ca5aa289bf532010cb6fe79f754e 2 V1 a408f6dd446cdd4fa56f82e77fe6c870 V2 8860480382d0ddf62623abf5c860e51d ", "0.0000000000 1 ec100d52d081881fc5eca373af9e343c 2 V1 1fc9300891b7a5d6583f0f85a83b9ddb V2 25bdf4cba0b59adfa07dd103d033bca9 ", "0.0000000000 1 ecb646aa07a906dc3db6588a1681f13d 2 V1 3225ea795289574ceee32e091ad54ef4 V2 181a19436acef495c8039a610be59603 "]
TOKENS FLOW
MessageData: []
FINISHED
```
