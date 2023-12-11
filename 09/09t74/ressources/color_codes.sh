# this file is intended to be included in other shell scripts to provide easy access to color codes

# ADVICE: for portability, please include this script via a path that is relative to the script where it is included.
#   Example: 
#     SCRIPT_DIR="$(dirname "$0")"
#     source "${SCRIPT_DIR}/color_codes.sh"

# EXAMPLES USAGE 
#   printf "I ${COLOR_RED}love${COLOR_RESET} color codes\n"
#   echo -e "${COLOR_BOLD_RED}WARNING:${COLOR_BOLD_WHITE} this is ${COLOR_BOLD_BLUE}BLUE${COLOR_RESET}"

COLOR_RESET='\033[0m' # No Color

# Plain colors
COLOR_BLACK='\033[0;30m'
COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_ORANGE='\033[0;33m'
COLOR_BLUE='\033[0;34m'
COLOR_PURPLE='\033[0;35m'
COLOR_CYAN='\033[0;36m'
COLOR_GRAY='\033[0;37m'

# BOLD Colors (brighter than plain colors)
COLOR_BOLD_GRAY='\033[1;30m'
COLOR_BOLD_RED='\033[1;31m'
COLOR_BOLD_GREEN='\033[1;32m'
COLOR_BOLD_YELLOW='\033[1;33m'
COLOR_BOLD_BLUE='\033[1;34m'
COLOR_BOLD_PURPLE='\033[1;35m'
COLOR_BOLD_CYAN='\033[1;36m'
COLOR_BOLD_WHITE='\033[1;37m'

COLOR_MESSAGE=$COLOR_CYAN
