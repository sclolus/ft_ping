#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable network-unknown ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable network-unknown ${@:2}

