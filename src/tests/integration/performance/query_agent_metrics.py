import subprocess
import sys
import time
import os
from typing import MutableMapping

# Number of times to run each query. At the end, the average time will be printed.
TESTS_ROUNDS = 10


def get_env_vars() -> MutableMapping:
    """
    Sets the environment variables for the Query Agent.
    """
    os.environ.update(
        {
            "DAS_MONGODB_HOSTNAME": "localhost",
            "DAS_MONGODB_PORT": "38000",
            "DAS_MONGODB_USERNAME": "dbadmin",
            "DAS_MONGODB_PASSWORD": "dassecret",
            "DAS_REDIS_HOSTNAME": "localhost",
            "DAS_REDIS_PORT": "39000",
        }
    )
    return os.environ


def start_process(command: str) -> subprocess.Popen:
    return subprocess.Popen(
        command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=get_env_vars(),
    )


def stop_process(process: subprocess.Popen):
    process.terminate()
    process.wait()


def run_command(command: str, check: bool = True) -> float:
    """
    Executes a shell command and returns the time taken to execute it.

    Args:
        command (str): The shell command to be executed.

    Returns:
        float: The execution time of the command in seconds.

    Raises:
        SystemExit: If the command execution fails.
    """
    start_time = time.perf_counter()
    try:
        subprocess.run(
            command,
            shell=True,
            check=check,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=get_env_vars(),
        )
    except subprocess.CalledProcessError:
        print(f"Command failed: {command}")
        sys.exit(1)
    end_time = time.perf_counter()
    execution_time = end_time - start_time
    return execution_time


def main():
    # fmt: off
    queries: dict[str, str] = dict(
        linktemplate_3_node_var_link=("""
            LINK_TEMPLATE Expression 3
                NODE Symbol Contains
                VARIABLE sentence1
                LINK Expression 2
                    NODE Symbol Word
                    NODE Symbol '"aaa"'
        """),
        and_2_linktemplate_linktemplate=("""
            AND 2
                LINK_TEMPLATE Expression 3
                    NODE Symbol Contains
                    VARIABLE sentence1
                    LINK Expression 2
                        NODE Symbol Word
                        NODE Symbol '"bbb"'
                LINK_TEMPLATE Expression 3
                    NODE Symbol Contains
                    VARIABLE sentence2
                    LINK Expression 2
                        NODE Symbol Word
                        NODE Symbol '"aaa"'
        """),
        and_2_linktemplate_or_2_linktemplate_linktemplate=("""
            AND 2
                LINK_TEMPLATE Expression 3
                    NODE Symbol Contains
                    VARIABLE sentence1
                    LINK Expression 2
                        NODE Symbol Word
                        NODE Symbol '"bbb"'
                OR 2
                    LINK_TEMPLATE Expression 3
                        NODE Symbol Contains
                        VARIABLE sentence2
                        LINK Expression 2
                            NODE Symbol Word
                            NODE Symbol '"aaa"'
                    LINK_TEMPLATE Expression 3
                        NODE Symbol Contains
                        VARIABLE sentence3
                        LINK Expression 2
                            NODE Symbol Word
                            NODE Symbol '"ccc"'
        """),
    )
    # fmt: on

    cmd_prefix = "bash src/scripts/run.sh query 'localhost:31701' 'localhost:31700' false "  # Prefix for all commands
    cmd_suffix = ""  # Suffix for all commands

    # Start the Attention Broker
    print("Starting Attention Broker...", flush=True)
    attention_broker_process = start_process("make run-attention-broker")
    # Wait for the Attention Broker to be ready
    time.sleep(3)  # Adjust this time as needed

    # For some unknown reason, the Query Agent needs to be started in advance, even if it will
    # already be started down below. This is a workaround to avoid the Query Agent to crash later.
    # This issue only happens in this particular script.
    query_agent_process = start_process("make run-query-agent")
    time.sleep(3)
    stop_process(query_agent_process)
    time.sleep(3)

    for name, query in queries.items():
        print(f"\nRunning query '{name}'...")

        execution_time: float = 0.0

        print(f"Rounds [for round in range({TESTS_ROUNDS})]:", flush=True)

        for round in range(TESTS_ROUNDS):
            # Start the Query Agent
            query_agent_process = start_process("make run-query-agent")

            # Wait for the Query Agent to be ready
            time.sleep(3)  # Adjust this time as needed

            print(f"  {round}: ", flush=True, end="")

            # Run the query
            round_time = run_command(cmd_prefix + query.replace("\n", " ") + cmd_suffix)

            # Stop the Query Agent
            stop_process(query_agent_process)

            execution_time += round_time

            print(f"{round_time:.2f} seconds")

        execution_time_avg = execution_time / TESTS_ROUNDS

        print(f"Average time for '{name}': {execution_time_avg:.2f} seconds")

    # Stop the Attention Broker
    print("\nStopping Attention Broker...", flush=True)
    stop_process(attention_broker_process)


if __name__ == "__main__":
    main()
