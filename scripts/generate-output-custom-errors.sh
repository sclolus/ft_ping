#!/bin/bash
# ./scripts/generate-output-custom-errors.sh <duration> <type> <code> <ping cmd>

sudo ./scripts/turn_off_icmp_echo.sh

echo The pings will be tested against custom icmp error scheme: $2 $3

sudo ./pong $2 $3  > /dev/null & 
PONG_PID=$!
echo $PONG_PID is the now pong pid
sleep 1

./scripts/generate-output.sh $1 ${@:4}

echo Killing and waiting for $PONG_PID
setsid sudo kill $PONG_PID # Fuck my goddamn life...

wait $PONG_PID
echo $PONG_PID was killed and waited for...

sudo ./scripts/turn_on_icmp_echo.sh
