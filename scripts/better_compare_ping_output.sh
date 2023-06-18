#!/bin/bash
# ./scripts/better_compare_ping_output.sh <ping_output_1> <ping_output_2>

diff -u $1 $2 | diffr --line-numbers aligned
