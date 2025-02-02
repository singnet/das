#!/bin/bash

declare -a ssh_hosts

IMAGE_NAME="das-spawner-client-builder:latest"

usage() {
    cat << EOF

Error: Incorrect parameters

Usage: $0 <path_to_hosts_file> <das_node_server_id> <query_tokens+>

The hosts file should have the pattern:
  ip:port;username;path_to_private_key or localhost:port

Example:
  localhost:9000
  11.22.33.44:6000;john;/home/john/.ssh
EOF
    exit 1
}

validate() {
    if [ "$#" -lt 3 ]; then
        usage
    fi

    local hosts_file="$1"
    local das_node_server_id="$2"
    local query="$3"

    if [ ! -f "$hosts_file" ] || [ ! -r "$hosts_file" ]; then
        echo ""
        echo "Error: File '$hosts_file' does not exist or cannot be read."
        exit 1
    fi

    if [[ ! "$das_node_server_id" =~ ^(([0-9]{1,3}\.){3}[0-9]{1,3}|localhost):[0-9]+$ ]]; then
        echo ""
        echo "Error: Invalid das_node_server_id format. Use IP:port (e.g., 11.22.33.44:9090 or localhost:9090)"
        exit 1
    fi

    if [ -z "$query" ]; then
        echo ""
        echo "Error: Query cannot be empty."
        exit 1
    fi
}

parse_file() {
    local file="$1"

    while IFS=';' read -r ip_port username private_key || [ -n "$ip_port" ]; do
        if [[ -n "$ip_port" ]]; then
            if [[ -z "$username" && -z "$private_key" ]]; then
                ssh_hosts+=("$ip_port")
            else
                ssh_hosts+=("$ip_port,$username,$private_key")
            fi
        fi
    done < "$file"
}

check_running() {
    container_name=$1
    docker ps --filter "name=$container_name" --format '{{.Names}}' | grep -q "$container_name"
    echo $?
}

main() {
    validate "$@"
    
    hosts_file="$1"
    das_node_server_id="$2"
    query="$3"

    parse_file "$hosts_file"

    server_host="${ssh_hosts[0]}"
    IFS=':' read -r server_ip server_port <<< "$server_host"
    server_id="$server_ip:55000"

    docker_cmd_sentinel="
        docker run -d --rm --net=host \
        --name=sentinel-server \
        --volume /tmp:/tmp \
        $IMAGE_NAME \
        sh -c './bin/sentinel_server 55000 > /tmp/sentinel.log 2>&1 && tail -f /tmp/sentinel.log' \
        >/dev/null 2>&1 && echo \"The server has started.\"
    "

    for (( i=0; i<${#ssh_hosts[@]}; i++ )); do
        IFS=',' read -r ip_port username private_key <<< "${ssh_hosts[$i]}"
        IFS=':' read -r ip port <<< "$ip_port"

        docker_cmd_worker="
            docker run --rm --net=host \
            --volume /tmp:/tmp \
            $IMAGE_NAME \
            ./bin/worker_client \
            $ip_port $server_id $das_node_server_id $query
        "

        if [[ "$i" -eq 0 ]]; then
            RUNNING=$(check_running "sentinel-server")
            if [ "$RUNNING" -eq 0 ]; then
                docker_cmd="echo 'Server already running.'"
            else
                docker_cmd=$docker_cmd_sentinel
            fi           
        else
            docker_cmd=$docker_cmd_worker
        fi

        if [[ -z "$username" && -z "$private_key" ]]; then            
            if [[ "$ip" == "localhost" || "$ip" == "127.0.0.1" ]]; then
                eval "$docker_cmd"
                sleep 1
            else
                echo "Error: To access the host '$ip_port' you need to provide valid credentials (username and private key)."
            fi
        else
            ssh -i "$private_key" "$username@$ip" "$docker_cmd"
        fi    
    done
}

main "$@"
