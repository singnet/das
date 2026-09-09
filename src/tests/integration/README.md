## Integration tests

### Pre-requisites

1. `das-cli` in its latest version available to call from the command line. `das-cli` can be installed via a debian package or via `pip install`. See instructions here: [das-cli repo](https://github.com/singnet/das-toolbox/tree/master/das-cli)
2. DAS fully built using `make build-all` (see instructions here: [DAS repo](https://github.com/singnet/das)

### Running

From the DAS repo base dir.

```bash
./src/tests/scripts/all_integration_tests.sh [-y]
```

The optional `-y` flag suppresses a warning message about the test shutting down and pruning any docker container running in the machine.

Each integration test can be run individually as well:

```bash
./src/tests/scripts/lca_integration_test.sh [-y]
```
