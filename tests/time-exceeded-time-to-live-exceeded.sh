#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 time-exceeded time-to-live-exceeded ${@:2}]
./scripts/generate-output-custom-errors.sh $1 time-exceeded time-to-live-exceeded ${@:2}

