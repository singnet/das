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
  local package_repository=$1
  local tmp_folder=$(mktemp -d)

  git clone $package_repository $tmp_folder &>/dev/null

  echo $tmp_folder
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
