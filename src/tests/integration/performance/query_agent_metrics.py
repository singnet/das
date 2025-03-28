import subprocess
import sys
import time

QUERY_CLIENT_CONTAINER_NAME = "das-query-client"

# Number of times to run each query. At the end, the average time will be printed.
TESTS_ROUNDS = 10


def start_query_client_container() -> subprocess.Popen:
    command = f"""
        docker run --rm \
            --name="{QUERY_CLIENT_CONTAINER_NAME}" \
            --network host \
            --volume .:/opt/das \
            --workdir /opt/das \
            -it das-builder \
            tail
    """
    process = subprocess.Popen(
        command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return process


def stop_query_client_container(process: subprocess.Popen):
    process.terminate()
    process.wait()


def start_query_agent() -> subprocess.Popen:
    """
    Starts the Query Agent (with the command `make run-query-agent`) and returns a Popen object.

    Returns:
        subprocess.Popen: A Popen object that can be used to stop the Query Agent later with `process.terminate()`.
    """
    return subprocess.Popen(
        "make run-query-agent",
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def stop_query_agent(process: subprocess.Popen):
    """
    Stops the Query Agent (with the command `make stop-query-agent`).

    Args:
        process (subprocess.Popen): A Popen object that was returned by `start_query_agent()`.
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

    # Start the Query Client container
    query_client_process = start_query_client_container()
    # Wait for the Query Client to be ready
    time.sleep(3)  # Adjust this time as needed
    print("Query Client container started.")

    # cmd_prefix = "bash src/scripts/run.sh query 'localhost:31701' 'localhost:31700' "  # Prefix for all commands
    cmd_prefix = f"docker exec -it {QUERY_CLIENT_CONTAINER_NAME} src/bin/query 'localhost:31701' 'localhost:31700' "  # Prefix for all commands
    cmd_suffix = ""  # Suffix for all commands

    for name, query in queries.items():
        print(f"\nRunning query '{name}'...")

        execution_time: float = 0.0

        print(f"Rounds [for round in range({TESTS_ROUNDS})]:", flush=True)

        for round in range(TESTS_ROUNDS):
            print(f"  {round}: ", flush=True, end="")

            # Start the Query Agent
            query_agent_process = start_query_agent()

            # Wait for the Query Agent to be ready
            time.sleep(3)  # Adjust this time as needed

            # Run the query
            round_time = run_command(cmd_prefix + '"' + query.replace("\n", " ") + '"' + cmd_suffix)

            # Stop the Query Agent
            stop_query_agent(query_agent_process)

            execution_time += round_time

            print(f"{round_time:.2f} seconds")

        execution_time_avg = execution_time / TESTS_ROUNDS

        print(f"Average time for '{name}': {execution_time_avg:.2f} seconds")

    # Stop the Query Client container
    stop_query_client_container(query_client_process)


if __name__ == "__main__":
    main()
