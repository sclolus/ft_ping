#!/bin/bash
# ./scripts/generate-output-custom-errors.sh <duration> <type> <code> <ping cmd>

./scripts/turn_off_icmp_echo.sh

echo The pings will be tested against a custom icmp error scheme

./pong $2 $3 >/dev/null 2>/dev/null &
PONG_PID=$!

./scripts/generate-output.sh $1 ${@:4}

kill -SIGKILL $PONG_PID

./scripts/turn_on_icmp_echo.sh
