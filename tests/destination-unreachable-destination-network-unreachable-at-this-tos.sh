#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable destination-network-unreachable-at-this-tos ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable destination-network-unreachable-at-this-tos ${@:2}

