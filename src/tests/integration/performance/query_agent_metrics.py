import subprocess
import sys
import time

# Number of times to run each query. At the end, the average time will be printed.
TESTS_ROUNDS = 10


def start_query_agent() -> subprocess.Popen:
    """
    Starts the Query Agent.

    Returns:
        subprocess.Popen: The process object for the Query Agent.
    """
    process = subprocess.Popen(
        "make run-query-agent",
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    time.sleep(3)  # Wait for the Query Agent to start and be ready
    return process


def stop_query_agent(process: subprocess.Popen):
    """
    Stops the Query Agent.

    Args:
        process (subprocess.Popen): The process object for the Query Agent.
    """
    process.terminate()
    process.wait()


def run_command(command: str) -> float:
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
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
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

    cmd_prefix = "bash src/scripts/run.sh query 'localhost:31701' 'localhost:31700' "  # Prefix for all commands
    cmd_suffix = ""  # Suffix for all commands

    for name, query in queries.items():
        print(f"\nRunning query '{name}'...")

        execution_time: float = 0.0

        print(f"Rounds [for round in range({TESTS_ROUNDS})]:", flush=True)

        for round in range(TESTS_ROUNDS):
            print(f"  {round}: ", flush=True, end="")

            # Start the Query Agent
            query_agent_process = start_query_agent()

            # Run the query
            round_time = run_command(cmd_prefix + query.replace("\n", " ") + cmd_suffix)

            # Stop the Query Agent
            stop_query_agent(query_agent_process)

            execution_time += round_time

            print(f"{round_time:.2f} seconds")

        execution_time_avg = execution_time / TESTS_ROUNDS

        print(f"Average time for '{name}': {execution_time_avg:.2f} seconds")


if __name__ == "__main__":
    main()
