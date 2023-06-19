#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 redirect redirect-network ${@:2}]
./scripts/generate-output-custom-errors.sh $1 redirect redirect-network ${@:2}

