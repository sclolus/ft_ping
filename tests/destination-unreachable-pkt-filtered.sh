#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable pkt-filtered ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable pkt-filtered ${@:2}

