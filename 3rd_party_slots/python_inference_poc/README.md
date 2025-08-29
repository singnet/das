# Toy problem

## Running on docker

### Dependencies:

* das-cli

### Running:

Create a new scenario on `3rd_party_slots/python_inference_poc/config/` or use the default `scenario_1.json`.

To run the setup and agents, `cd` to das root folder and run:
``` 
make run-inference-toy-problem 3rd_party_slots/python_inference_poc/config/scenario_1.json
```
To use other MeTTa files (already generated), add the path of the base and bias after the scenario, the script will skip the generation process and load the given files:
```
make run-inference-toy-problem 3rd_party_slots/python_inference_poc/config/scenario_1.json /tmp/mybase.metta /tmp/mybias.metta
```
To run the problem `feature_a -> feature_d`, `cd` to das root folder and run:

```
make run-inference-agent-client OPTIONS="localhost:5030 localhost:4000 19000:20000 PROOF_OF_IMPLICATION 27670bc9a72fb87d431388ac75fa7fb6 3265822e4ee269dee41074547cd00f3d 1 TOY"
```


## Running locally

### Dependencies:

* das-cli
* python3.10

### Running:

Create a new scenario on `3rd_party_slots/python_inference_poc/config/` or use the default `scenario_1.json`.

To run the setup, `cd` to `3rd_party_slots/python_inference_poc/` and create/activate a python venv:
```
python3.10 -m venv venv
source venv/bin/activate
```
 Run the sentence generator using the config in your json scenario:

 ```
 python3 metta_file_generator.py --sentence-node-count <json_sentence_node_count> --word-count <json_word_count> --word-length <json_word_length> --alphabet-range <json_alphabet_range>
 ```

Run the predicate and bias generator:

```
python3 predicate_generator.py  output.metta --config-file config/your_scenario.json
```

Copy the MeTTa output files to `/tmp` and run `das-cli`:

```
das-cli db start
das-cli metta load /tmp/output.metta
das-cli metta load /tmp/biased_predicates.metta
```

Start the agents:
`cd` to das root folder and run:
```
export LINK_CREATION_REQUESTS_INTERVAL_SECONDS=5
make agents start
```


To run the problem `feature_a -> feature_d`, `cd` to das root folder and run:

```
make run-inference-agent-client OPTIONS="localhost:5030 localhost:4000 19000:20000 PROOF_OF_IMPLICATION 27670bc9a72fb87d431388ac75fa7fb6 3265822e4ee269dee41074547cd00f3d 1 TOY"
```