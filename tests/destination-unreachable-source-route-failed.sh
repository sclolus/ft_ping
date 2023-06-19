#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable source-route-failed ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable source-route-failed ${@:2}

