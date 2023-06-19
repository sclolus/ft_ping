#!/bin/bash
# ./scripts/generate-output-custom-errors.sh <duration> <type> <code> <ping cmd>

sudo ./scripts/turn_off_icmp_echo.sh

echo The pings will be tested against custom icmp error scheme: $2 $3

sudo ./pong $2 $3  > /dev/null & 
PONG_PID=$!
sleep 1

./scripts/generate-output.sh $1 ${@:4}

# setsid sudo kill $PONG_PID # Fuck my goddamn life...
sudo pkill -SIGKILL pong > /dev/null 2>/dev/null
wait $PONG_PID >/dev/null 2>/dev/null

sudo ./scripts/turn_on_icmp_echo.sh
