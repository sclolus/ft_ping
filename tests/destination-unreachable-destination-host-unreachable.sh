#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable destination-host-unreachable ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable destination-host-unreachable ${@:2}

