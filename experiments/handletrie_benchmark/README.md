# HandleTrie C++ and Python benchmarks


### Time average in seconds

|  Impl |  1,000n  | 100,000n  | 1,000,000n  | 10,000,000n |60,000,000* |
|---|---|---|---|---|---|
|C++ Handle Trie      |0.00129|0.15719|1.86882|23.91808|149.15487*|
|C++ unordered map std|0.00054|0.15927|2.12357|22.52662|-|
|C++ map std          |0.00151|0.32405|5.21409|71.53995|561.42147*|
|Python's Dict        |0.00037|0.06631|0.89053|11.75517|79.204370*|
|CPython Handle Trie  |0.00130|0.38828|5.00333|63.05321|510.58422*| 
|NanoBind Handle Trie |0.00281|0.52542|6.52422|84.37761|651.15217*| 
|PyBind Handle Trie   |0.01159|1.41426|15.5277|168.3574|1159,28535*|

> \* The 60,000,000 column values may not be accurate, as the test fill up all the available memory on the test machine, leading to inaccurate results.

> **Note:** Although both the C++ and Python implementations account for overhead, the operations being measured are not exactly the same. To minimize this discrepancy, the time spent on random number generation for populating keys has been estimated and excluded. This discrepancy could affect performance comparisons. For the complete runtime, please check the "runs" folder.



### Memory average in GB

|  Impl |  1,000n  | 100,000n  | 1,000,000n  | 10,000,000n |60,000,000n |
|---|---|---|---|---|---|
|C++ Handle Trie      |0.01428|0.01597|0.23844|3.01968| - | 
|C++ map std          |0.01587|0.01600|0.47400|3.19017| - |
|Python's Dict        |0.00448|0.02477|0.26700|3.10800|20.59636|
|CPython Handle Trie  |0.00448|0.06898|0.59080|5.45343|33.04674|
|NanoBind Handle Trie |0.00448|0.07680|0.61002|5.70955|34.56202|
|PyBind Handle Trie   |0.00967|0.07858|0.59319|5.56300|35.54603|


### Setup Python

```
python3 -m venv venv
source venv/bin/activate
pip install pybind11
pip install nanobind
```

### Build C++

```
make build
```

### Build CPython
```
make build-cpython
```


### Build PyBind

```
make build-pybind
```


### Build NanoBind

```
make build-nanobind
```


### Run

```
python main.py
```