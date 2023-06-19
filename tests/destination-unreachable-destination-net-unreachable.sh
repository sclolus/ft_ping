#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable destination-net-unreachable ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable destination-net-unreachable ${@:2}

