#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 time-exceeded frag-reassembly-time-exceeded ${@:2}]
./scripts/generate-output-custom-errors.sh $1 time-exceeded frag-reassembly-time-exceeded ${@:2}

