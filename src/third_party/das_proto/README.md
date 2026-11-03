# das-proto

### Build
Install `bazelisk` and run the following command from the repo root:
```bash
bazelisk build //:all
```

### Check
Check the results:
```bash
ls -1 bazel-bin/*.(h|cc)

bazel-bin/atom_space_node.grpc.pb.cc
bazel-bin/atom_space_node.grpc.pb.h
bazel-bin/atom_space_node.pb.cc
bazel-bin/atom_space_node.pb.h
bazel-bin/attention_broker.grpc.pb.cc
bazel-bin/attention_broker.grpc.pb.h
bazel-bin/attention_broker.pb.cc
bazel-bin/attention_broker.pb.h
bazel-bin/common.grpc.pb.cc
bazel-bin/common.grpc.pb.h
bazel-bin/common.pb.cc
bazel-bin/common.pb.h
```
