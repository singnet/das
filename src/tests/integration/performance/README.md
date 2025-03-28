## DAS Performance Test

### How to run it

1. SSH to the test server on Vultr (`149.28.214.107`) with a user that has `sudo` and `docker` permissions.

2. Clone `das` repo (https://github.com/singnet/das).

3. From the `das` repo root, run the following command to build the services:

Note: _Remember to checkout the branch you want to test._

```bash
make bazel clean
make build-all
```

4. Start MongoDB and Redis using the scripts in `src/tests/integration/performance/start_mongo_and_redis.sh` (it requires sudo permissions):

```bash
sudo ./src/tests/integration/performance/start_mongo_and_redis.sh
```

5. Run the performance test:

```bash
make performance-test
```

6. Save the results in a file named `performance_results_$(date +%Y-%m-%d_%H-%M-%S).txt` if you want to keep it and commit it to the repository.

