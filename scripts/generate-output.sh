#!/bin/bash
# ./scripts/generate-output.sh <duration> <ping cmd>
REAL_PING=./real_ping

echo The executed commands will be like:
echo "[ping] ${@:2}"

echo Collecting output from ft_ping
echo Executing cmd ["./ft_ping ${@:2}"]
./ft_ping ${@:2} > ft.log &
PID=$!
sleep "$1"
kill -SIGINT $PID
wait $PID
echo ft_ping was killed.

echo collecting output from the real ping
echo Executing cmd ["${REAL_PING} ${@:2}"]
${REAL_PING} ${@:2} > real.log &
PID=$!
sleep "$1"
kill -SIGINT $PID
wait $PID
echo The real ping was killed.

echo ======================= DIFF ========================
./scripts/better_compare_ping_output.sh ft.log real.log
echo ======================= ==== ========================
# chown $USER:$USER ft.log
# chown $USER:$USER real.log
