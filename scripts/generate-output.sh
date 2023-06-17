#!/bin/bash

REAL_PING=ping

echo The executed commands will be like:
echo "[ping] ${@:2}"

echo Collecting output from ft_ping
./ft_ping ${@:2} > ft.log &
PID=$!
sleep $1
echo Killing ft_ping...
sync
kill -SIGINT $PID
echo ft_ping was killed.

echo collecting output from the real ping
${REAL_PING} ${@:2} > real.log &
PID=$!
sleep $1
echo Killing the real ping...
sync
kill -SIGINT $PID
echo The real ping was killed.

./scripts/better_compare_ping_output.sh ft.log real.log
