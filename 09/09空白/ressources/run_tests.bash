#!/bin/bash

source ./color_codes.sh

PORT=31026
SRV_NAME="myGroup-server"
CLI_NAME="myGroup-client"
INPUT_C="input-testC"
INPUT_D="input-testD"
TIMEOUT=5 #seconds

SRV_CMD="./$SRV_NAME"
CLI_CMD="./$CLI_NAME"
CLI_OUT="./cli.out"
SRV_OUT="./srv.out"

FIFO_NAME="fifo_in"

#sm: server output match
#cm: client ouput match
#Test A
sm0="new connection accepted"

#Test B
sm1="Handshake: Hello, I'm the client"
cm1="Hello, I'm the server"

#Test C
sm2="client [0-9]* write: test1"
sm3="client [0-9]* write: Testzwei"
sm4="client [0-9]* write: TESTIII"

#Test D
sm5="client [0-9]* read: test1"
sm6="client [0-9]* read: Testzwei"
sm7="client [0-9]* read: TESTIII"

#points
##achieved
points_A=0
points_B=0
points_C=0
points_D=0
##possible
max_points_A=3
max_points_B=3
max_points_C=4
max_points_D=3

function test_binaries {
	if [[ ! -x "$SRV_CMD" ]]; then
		echo -e ${COLOR_MESSAGE}${SRV_CMD}does not exist or is not executable${COLOR_RESET}
		exit 1
	fi
	if [[ ! -x "$CLI_CMD" ]]; then
		echo -e ${COLOR_MESSAGE}${CLI_CMD} does not exist or is not executable${COLOR_RESET}
		exit 1
	fi
}

function client_still_running {
	if ! pgrep $CLI_NAME > /dev/null; then
		echo -e ${COLOR_BOLD_RED}${CLI_CMD} not running anymore. will stop tests!${COLOR_RESET}
		print_summary
		# Kill server, in case it's still running
		killall -eq $SRV_NAME
		exit 1
	fi
}

function start_server {
	stdbuf -oL $SRV_CMD > $SRV_OUT &
}

function start_client {
	if [ ! -p fifo_in ]; then
		mkfifo $FIFO_NAME
	fi
	tail -f $FIFO_NAME | stdbuf -oL $CLI_CMD > $CLI_OUT &
	sleep 0.5
}

function match_server {
	sleep 1
	sync
	arg="$@"
	if grep -q "$arg" $SRV_OUT; then
		echo -e "	${COLOR_BOLD_GREEN}matching server log entry: \"$arg\"${COLOR_RESET}"
		return 0
	else
		echo -e "	${COLOR_BOLD_RED}expected server log entry: \"$arg\"${COLOR_RESET}"
		return 1
	fi
}

function match_client {
	sleep 1
	sync
	arg="$@"
	if grep -q "$arg" $CLI_OUT; then
		echo -e "	${COLOR_BOLD_GREEN}matching client log entry: \"$arg\"${COLOR_RESET}"
		return 0
	else
		echo -e "	${COLOR_BOLD_RED}expected client log entry: \"$arg\"${COLOR_RESET}"
		return 1
	fi
}


function check_A {
	if match_server $sm0; then
		points_A=$max_points_A
	fi
}

function check_B {
	match_server $sm1 
	r1=$?
	match_client $cm1
	r2=$?
	if [ $(($r1+$r2)) -eq 0 ]; then
		points_B=$max_points_B
	fi
}

function check_C {
	match_server $sm2 
	r1=$?
	match_server $sm3 
	r2=$?
	match_server $sm4
	r3=$?
	if [ $(($r1+$r2+$r3)) -eq 0 ]; then
		points_C=$max_points_C
	fi
}

function check_D {
	match_server $sm5
	r1=$?
	match_server $sm6 
	r2=$?
	match_server $sm7
	r3=$?
	if [ $(($r1+$r2+$r3)) -eq 0 ]; then
		points_D=$max_points_D
	fi
}

function print_summary {
	echo -e ${COLOR_MESSAGE}Summary
	echo -e "	9.1: ${points_A}/${max_points_A}P"
	echo -e "	9.2: ${points_B}/${max_points_B}P"
	echo -e "	9.3: -"
	echo -e "	9.4: ${points_C}/${max_points_C}P"
	echo -e "	9.5: ${points_D}/${max_points_D}P"
	echo -e "	9.6: -"
	echo -e "	9.7: -"
	echo -e "${COLOR_RESET}"
}

SCRIPTNAME=$0
PARAMETER=$1

if [ $# != 0 ]; then
	echo -e "usage: $0\n\tno arguments required"
	exit 1
fi

rm -f $FIFO_NAME

#kill server
killall -eq $SRV_NAME

#check if killed (port in use?)
pid=`lsof -Pi :$PORT -sTCP:LISTEN | tail -1 | cut -d" " -f2`
if [ ! -z $pid ]; then
	echo "Could not kill server. Port $PORT is still in use by process $pid!"
	exit 1
fi

#PROCESSING

test_binaries

start_server

sleep 3

start_client

#Test A
echo -e ${COLOR_MESSAGE}TEST A \(9.1\)${COLOR_RESET}
check_A

#Test B
echo -e ${COLOR_MESSAGE}TEST B \(9.2\)${COLOR_RESET}
check_B

#Test C
echo -e ${COLOR_MESSAGE}TEST C \(9.4\)${COLOR_RESET}
client_still_running
cat $INPUT_C > ./$FIFO_NAME
check_C

#Test D
echo -e ${COLOR_MESSAGE}TEST D \(9.5\)${COLOR_RESET}
client_still_running
cat $INPUT_D > ./$FIFO_NAME
check_D

kill %1 #kill server
kill %2 #kill client

print_summary

rm $FIFO_NAME
rm $CLI_OUT
rm $SRV_OUT
