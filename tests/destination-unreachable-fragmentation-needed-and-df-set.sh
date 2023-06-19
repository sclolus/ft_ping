#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 destination-unreachable fragmentation-needed-and-df-set ${@:2}]
./scripts/generate-output-custom-errors.sh $1 destination-unreachable fragmentation-needed-and-df-set ${@:2}

