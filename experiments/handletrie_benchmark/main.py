import time
import json
import statistics
import os
import threading
import subprocess
import gc

from functools import reduce
from cxx import handletrie_cpython, handletrie_nanobind, handletrie_pybind
from ctypes import CDLL

libs = CDLL("libc.so.6")
MEMORY = 0
RUN = "python"
OROUND = 5  # output round
R = None
HANDLE_HASH_SIZE = 33
TIME = 0
R_TLB = [
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "a",
    "b",
    "c",
    "d",
    "e",
    "f",
]


def lcg(modulus: int, a: int, c: int, seed: int):
    """Linear congruential generator."""
    while True:
        seed = (a * seed + c) % modulus
        yield seed


def lcg_to_interval(generator, interval_min: int, interval_max: int):
    range_size = interval_max - interval_min + 1

    for value in generator:
        # Map value to the interval [interval_min, interval_max]
        yield interval_min + (value % range_size)


def measure(func):
    def wrapper(*args, **kwargs):
        global TIME, RUN
        start_cxx = time.perf_counter_ns()
        start = time.process_time_ns()
        func(*args, **kwargs)
        stop_cxx = time.perf_counter_ns()
        stop = time.process_time_ns()
        if RUN == "c++":
            TIME = ((stop_cxx - start_cxx) - (stop - start)) / 1000000000
        else:
            TIME = (stop - start) / 1000000000
        # print("=======================================================")
        # print(f"{func.__name__}: {TIME}\t Memory: {MEMORY}GB")
        # print("=======================================================")

    return wrapper


def diff(values):
    return round(abs(reduce(lambda a, b: a - b, values)), OROUND)


def mean_round(values):
    return round(statistics.mean(values), OROUND)


def stdev_round(values):
    return round(statistics.stdev(values), OROUND)


def repeat(f, params, n=2):
    check = {}
    for i in range(n):
        for p in params:
            f(**p)
            m = MEMORY
            key = f"{p.get('name')}_{p.get('f').__name__}"
            if key not in check:
                check[key] = {"time": [TIME], "memory": [m]}
            else:
                check[key]["time"].append(TIME)
                check[key]["memory"].append(m)
            gc.collect()

    print("=======================================================")
    diff_m = []
    diff_sdv = []
    memory_list = []
    for k, v in check.items():
        vv = v["time"]
        print(
            f"{k} = Average: {mean_round(vv)}, STDEV: {stdev_round(vv)}, Memory: {mean_round(v['memory'])} GB"
        )
        diff_m.append(statistics.mean(vv))
        diff_sdv.append(statistics.stdev(vv))
        memory_list.append(statistics.mean(v["memory"]))

    print(
        f"Difference = Average: {diff(diff_m)}, STDEV: {diff(diff_sdv)}, Memory: {diff(memory_list)} GB"
    )
    print("=======================================================")


def save_json(data, append=""):
    with open(append + "data.json", "w") as f:
        json.dump(data, f)


def load_json(append=""):
    try:
        with open(append + "data.json", "r") as f:
            return json.load(f)
    except FileNotFoundError:
        return {}


@measure
def generate_random(interactions=1000000):
    global R
    R = load_json(append=str(interactions))
    if not R:
        for key_count in {1, 2, 5}:
            key_size: int = (HANDLE_HASH_SIZE - 1) * key_count
            key_count = str(key_count)
            R[key_count] = []
            for i in range(interactions):
                R[key_count].append([])
                for j in range(key_size):
                    R[key_count][i].append(libs.rand() % 16)
        save_json(R, append=str(interactions))


def test_dict(baseline, s):
    if not baseline.get(s):
        baseline[s] = 0
    else:
        baseline[s] += 1


def none(baseline, s):
    pass


@measure
def benchmark_python_dict(f, n_insertions: int = 1000000, **kwargs):
    for key_count in {1, 2, 5}:
        # str_key = str(key_count)
        key_size: int = (HANDLE_HASH_SIZE - 1) * key_count
        baseline = {}
        for i in range(n_insertions):
            # s = "".join([R_TLB[libs.rand() % 16] for j in range(key_size)])
            # s = "".join([R_TLB[R[str_key][i][j]] for j in range(key_size)])
            # s = s[:key_size] + '0' + s[key_size + 1:]
            s = handletrie_cpython.generate_word(key_size)

            f(baseline, s)


def test_handle_trie(baseline, s):
    v = baseline.lookup(s)
    if not v:
        baseline.insert(s, 0)
    else:
        v += 1
        baseline.insert(s, v)


@measure
def benchmark_python_handle_trie(f, n_insertions: int = 1000000, **kwargs):
    for key_count in {1, 2, 5}:
        # str_key = str(key_count)
        key_size: int = (HANDLE_HASH_SIZE - 1) * key_count
        if kwargs.get("module"):
            baseline = kwargs["module"].HandleTrie(key_size)
        else:
            baseline = None
        for i in range(n_insertions):
            # s = "".join([R_TLB[libs.rand() % 16] for j in range(key_size)])
            # s = "".join([R_TLB[R[str_key][i][j]] for j in range(key_size)])
            # s = s[:key_size] + '0' + s[key_size + 1:]
            s = handletrie_cpython.generate_word(key_size)

            f(baseline, s)


def cxx_none():
    return "none"


def cxx_handletrie():
    return "handletrie"


def cxx_map():
    return "map"


def cxx_unordered_map():
    return "unordered_map"


@measure
def benchmark_cxx(f, n_insertions: int = 1000000, **kwargs):
    subprocess.run(["build/HandleTrie", f(), str(n_insertions)])


def get_pid():
    global RUN
    if RUN == "c++":
        ps = subprocess.run("ps -a".split(" "), stdout=subprocess.PIPE)
        s = str(ps.stdout)
        if "HandleTrie" in s:
            return next((f for f in s.split("\\n") if "HandleTrie" in f)).split(" ")[1]
    return os.getpid()


def memory_count(run_event, delay=1):
    global MEMORY
    while run_event.is_set():
        pid = get_pid()
        if pid:
            out = subprocess.run(
                f"ps -p {pid} -o rss=".split(" "), stdout=subprocess.PIPE
            )
            try:
                MEMORY = round(int(out.stdout) / 1000000.0, 5)
            except:
                MEMORY = 0
                pass
            time.sleep(delay)


def add_comma(number):
    s = str(number)[::-1]
    return ",".join([s[i : i + 3] for i in range(0, len(s), 3)])[::-1]


def main():
    global RUN
    run_event = threading.Event()
    thread = threading.Thread(target=memory_count, args=(run_event, 1))
    run_event.set()
    thread.start()

    # max 6.24×10^10
    try:
        for i in [1000, 100000, 1000000, 10000000, 60000000]:
            print(f"Testing {add_comma(i)} nodes")
            RUN = "c++"
            repeat(
                benchmark_cxx,
                [
                    {"f": cxx_handletrie, "name": "cxx_handletrie", "n_insertions": i},
                    {"f": cxx_none, "name": "cxx_none", "n_insertions": i},
                ],
                10,
            )

            print(f"Testing {add_comma(i)} nodes")
            repeat(
                benchmark_cxx,
                [
                    {"f": cxx_map, "name": "cxx_none", "n_insertions": i},
                    {"f": cxx_none, "name": "cxx_none", "n_insertions": i},
                ],
                10,
            )


            print(f"Testing {add_comma(i)} nodes")
            repeat(
                benchmark_cxx,
                [
                    {"f": cxx_unordered_map, "name": "cxx_none", "n_insertions": i},
                    {"f": cxx_none, "name": "cxx_none", "n_insertions": i},
                ],
                10,
            )


        for i in [1000, 100000, 1000000, 10000000, 60000000]:
            RUN = "python"
            print(f"Testing {add_comma(i)} nodes")

            repeat(
                benchmark_python_dict,
                [
                    {
                        "f": test_dict,
                        "name": "benchmark_python_dict",
                        "n_insertions": i,
                    },
                    {"f": none, "name": "benchmark_python_dict", "n_insertions": i},
                ],
                10,
            )

            print(f"Testing {add_comma(i)} nodes")
            repeat(
                benchmark_python_handle_trie,
                [
                    {
                        "f": test_handle_trie,
                        "name": "benchmark_handle_trie_cpython",
                        "n_insertions": i,
                        "module": handletrie_cpython,
                    },
                    {
                        "f": none,
                        "name": "benchmark_handle_trie_cpython",
                        "n_insertions": i,
                    },
                ],
                10,
            )

            print(f"Testing {add_comma(i)} nodes")
            repeat(
                benchmark_python_handle_trie,
                [
                    {
                        "f": test_handle_trie,
                        "name": "benchmark_handle_trie_nanobind",
                        "n_insertions": i,
                        "module": handletrie_nanobind,
                    },
                    {
                        "f": none,
                        "name": "benchmark_handle_trie_nanobind",
                        "n_insertions": i,
                    },
                ],
                10,
            )

            print(f"Testing {add_comma(i)} nodes")
            repeat(
                benchmark_python_handle_trie,
                [
                    {
                        "f": test_handle_trie,
                        "name": "benchmark_handle_trie_pybind",
                        "n_insertions": i,
                        "module": handletrie_pybind,
                    },
                    {
                        "f": none,
                        "name": "benchmark_handle_trie_pybind",
                        "n_insertions": i,
                    },
                ],
                10,
            )

    except KeyboardInterrupt:
        run_event.clear()
        thread.join()


main()
