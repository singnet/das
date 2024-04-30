declare -A colors=(
  [reset]="\033[0m"
  [red]="\033[0;31m"
  [green]="\033[0;32m"
  [blue]="\033[0;34m"
  [yellow]="\033[0;33m"
)

function requirements() {
  local required_commands=("$@")

  for cmd in "${required_commands[@]}"; do
    if ! command -v "$cmd" &>/dev/null; then
      print ":red:$cmd is required but not installed.:/red:" >&2
      exit 1
    fi
  done
}

function boolean_prompt() {
  local prompt="$1"

  while true; do
    read -p "$prompt" answer
    case "${answer,,}" in
    "y" | "yes")
      return 0
      ;;
    "n" | "no")
      return 1
      ;;
    *) continue ;;
    esac
  done
}

function text_prompt() {
  local prompt=$(print "$1")

  read -p "$prompt" answer
  echo $answer
}

function password_prompt() {
  local prompt=$(print "$1")

  read -s -p "$prompt" answer
  echo $answer
}

function retrieve_json_object_with_property_value() {
  jq -c --arg prop "$2" --arg val "$3" '.[] | select(.[$prop] == $val)' <<<"$1"
}

function contains_property_value() {
  if [ "$(retrieve_json_object_with_property_value "$@")" ]; then echo 1; else echo 0; fi
}

function get_repository_latest_version() {
  local latest_version=$(git for-each-ref --sort=-taggerdate --format '%(refname:short)' refs/tags |
    grep -E '^(v?[0-9]+\.[0-9]+\.[0-9]+)$' | head -n 1)

  echo "${latest_version:-unknown}"
}

function fmt_version() {
  [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] && echo 1 || echo 0
}

function verify_file_exists() {
  local file_path="$1"

  if [[ ! -f "$file_path" ]]; then
    print ":red:File not found at $file_path:/red:"
    return 1
  fi
}

function configure_git_identity() {
  local git_user="${GIT_USER_NAME:-$(git config user.name)}"
  local git_email="${GIT_USER_EMAIL:-$(git config user.email)}"

  if [[ -z "$git_user" ]]; then
    git_user=$(text_prompt "Your Git username is not configured. Please provide your Git name: ")
    export GIT_USER_NAME="$git_user"
  fi

  if [[ -z "$git_email" ]]; then
    git_email=$(text_prompt "Your Git email is not configured. Please provide your Git email: ")
    export GIT_USER_EMAIL="$git_email"
  fi

  git config user.name "$git_user"
  git config user.email "$git_email"

  print "Git configured with username: :green:$git_user:/green: and email: :green:$git_email:/green:"
}

function check_for_uncommitted_changes() {
  if git diff --quiet && git diff --staged --quiet; then
    print ":yellow:Skipping commit changes because no files were changed to be committed:/yellow:"
    return 1
  fi
  return 0
}

function show_git_diff_and_confirm() {
  local files_to_add="${3:-.}"

  if ! check_for_uncommitted_changes; then
    return
  fi

  git diff --color $files_to_add

  if ! boolean_prompt "Do you want to continue with the commit? [y/n] "; then
    print ":red:Commit canceled by the user.:/red:"
    exit 1
  fi
}

function commit_and_push_changes() {
  local commit_msg="$1"
  local target_branch="${2:-master}"
  local files_to_add="${3:-.}"

  if ! check_for_uncommitted_changes; then
    return
  fi

  configure_git_identity

  git add $files_to_add
  git commit -m "$commit_msg"
  git push origin "$target_branch"

  print ":green:Changes committed and pushed to the branch '$target_branch'.:/green:"
}

function clone_repo_to_temp_dir() {
  local package_repository="$1"
  local target_branch="${2:-master}"
  local tmp_folder=$(mktemp -d)

  git clone "$package_repository" "$tmp_folder" &>/dev/null

  git -C "$tmp_folder" checkout "$target_branch" &>/dev/null

  echo "$tmp_folder"
}

function print() {
  local text="$1"

  for color in "${!colors[@]}"; do
    if [[ "$color" != "reset" ]]; then
      text="${text//:$color:/${colors[$color]}}"
      text="${text//:\/$color:/${colors[reset]}}"
    fi
  done

  echo -e "$text"
}

function load_or_request_github_token() {
  local secret_path="$1"
  local secret=""

  if [[ -f "$secret_path" ]]; then
    read -r secret <"$secret_path"

    if [[ -n "$secret" ]]; then
      echo "$secret"
      return
    else
      print ":yellow:Found secret file but it was empty. Requesting new token...:/yellow:" >&2
    fi
  else
    print ":yellow:Secret file not found. Requesting new token...:/yellow:" >&2
  fi

  secret=$(text_prompt "Enter your GitHub access token: ")

  echo "$secret" >"$secret_path"
  chmod 600 "$secret_path"
  echo "$secret"
}

function append_content_in_file() {
  local file_path="$1"
  local new_block="$2"
  local line=$3

  if [ -z "$line" ]; then
    echo "$new_block" >>"$file_path"
  else
    printf '%s\n' "$new_block" | sed -i "$((line + 1))r /dev/stdin" "$file_path"
  fi
}

function find_line_number_by_text_in_file() {
  local file_path="$1"
  local text="$2"

  echo -e $(grep -n "$text" "$file_path" | cut -d: -f1)
}

function encrypt_password() {
  local password="$1"
  local key="$2"

  local encrypted_password=$(echo -n "$password" | openssl dgst -sha256 -hmac "$key" | awk '{print $2}')

  echo "$encrypted_password"
}

function print_header() {
  local header_text="$1"
  local header_length=${#header_text}

  local dash_line=""
  for ((i = 0; i < header_length + 20; i++)); do
    dash_line="${dash_line}-"
  done

  echo "$dash_line"
  printf "%*s\n" $(((${#dash_line} + ${#header_text}) / 2)) "$header_text"
  echo "$dash_line"
}

function execute_ssh_commands() {
  local server_ip=$1
  local server_username=$2
  local private_key_connection="$3"
  local pem_key_path="$4"
  shift 4
  local commands=("$@")

  local commands_str=""
  for cmd in "${commands[@]}"; do
    commands_str+="$cmd && "
  done
  commands_str=${commands_str%&& }

  if [ "$private_key_connection" ]; then
    ssh -T -i "$pem_key_path" $server_username@$server_ip "$commands_str"
  else
    sshpass -p "$password" ssh -T $server_username@$server_ip "$commands_str"
  fi

}

function ping_ssh_server() {
  local ip="$1"
  local username="$2"
  local private_key_connection="$3"
  local private_key_path="$3"
  local ping_command="echo hello"

  execute_ssh_commands "$ip" "$username" "$private_key_connection" "$private_key_path" "$ping_command"

  if [ $? -eq 0 ]; then
    print ":green:As credenciais SSH estão corretas e a conexão foi bem-sucedida.:/green:"
  else
    print ":red:As credenciais SSH estão incorretas ou a conexão falhou.:/red:"
    exit 1
  fi
}

function choose_menu() {
  local prompt="$1" outvar="$2"
  shift
  shift
  local options=("$@") cur=0 count=${#options[@]} index=0
  local esc=$(echo -en "\e")
  printf "$prompt\n"
  while true; do
    index=0
    for o in "${options[@]}"; do
      if [ "$index" == "$cur" ]; then
        echo -e " >\e[7m$o\e[0m"
      else
        echo "  $o"
      fi
      index=$(($index + 1))
    done
    read -s -n3 key
    if [[ $key == $esc[A ]]; then
      cur=$(($cur - 1))
      [ "$cur" -lt 0 ] && cur=0
    elif [[ $key == $esc[B ]]; then
      cur=$(($cur + 1))
      [ "$cur" -ge $count ] && cur=$(($count - 1))
    elif [[ $key == "" ]]; then
      break
    fi
    echo -en "\e[${count}A"
  done
  printf -v $outvar "${options[$cur]}"
  clear
}
