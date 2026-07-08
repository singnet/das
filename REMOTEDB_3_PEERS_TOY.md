## Split the KB into 2 parts:
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
# Part1 to Peer1
make run-db-loader OPTIONS="--config=config/peer1_loader.json --file=/tmp/inference_toy_10000_5_3_abcde_500_400_part1.metta --threads=8 --chunk=5000"

# Part2 to Peer2
make run-db-loader OPTIONS="--config=config/peer2_loader.json --file=/tmp/inference_toy_10000_5_3_abcde_500_400_part2.metta --threads=8 --chunk=5000"
```

## 