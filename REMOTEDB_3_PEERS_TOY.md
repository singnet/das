## Topology
- peer1 (readonly, base KB part1)  -> RedisMongoDB at 40021+40020
- peer2 (readonly, base KB part2)  -> RedisMongoDB at 40031+40030
- peer3 (writable, starts empty)   -> InMemoryDB + local_persistence RedisMongoDB at 40041+40040

peer1/peer2 serve federated reads of the base KB. peer3 is the only writable peer and starts empty.
On flush, peer3 lazily pulls the dependency closure of staged writes from peer1/peer2 into its
local_persistence (lazy closure replication), then upserts the new/updated links. Evolution's
strength updates and new links land there and are read back writable-peer-first.

## Split the KB into 2 parts (for the readonly base peers):
```
KB=/tmp/inference_toy_10000_5_3_abcde_500_400.metta

HALF=$(( $(wc -l < "$KB") / 2 ))

head -n "$HALF" "$KB" > /tmp/inference_toy_10000_5_3_abcde_500_400_part1.metta
tail -n +$((HALF + 1)) "$KB" > /tmp/inference_toy_10000_5_3_abcde_500_400_part2.metta
```

## Start Databases:
```
# Config das-cli to use MongoDB+Redis at 40021 and 40020
das-cli config set

# Start both DBs
das-cli db start

# Do the same but use these ports 40031+40030 and 40041+40040
```

## Load KB parts
```
# Part1 to Peer1 at 40021+40020
make run-db-loader OPTIONS="--config=config/peer1_loader.json --file=/tmp/inference_toy_10000_5_3_abcde_500_400_part1.metta --threads=8 --chunk=5000"

# Part2 to Peer2 at 40031+40030
make run-db-loader OPTIONS="--config=config/peer2_loader.json --file=/tmp/inference_toy_10000_5_3_abcde_500_400_part2.metta --threads=8 --chunk=5000"

# Peer3 stays empty; its local_persistence accumulates only the working set at flush time.
```

## Run Toy (`evaluation_evolution.cc`):
```
./src/tests/scripts/run_toy.sh /tmp/inference_toy_10000_5_3_abcde_500_400.metta inference_toy_10k '(Predicate "sort")' '(Concept "bac bbb cab ccc ddd")' 0.2 0.2 0.2 0.03 0.10
```
