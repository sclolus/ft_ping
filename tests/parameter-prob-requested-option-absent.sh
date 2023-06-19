#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 parameter-prob requested-option-absent ${@:2}]
./scripts/generate-output-custom-errors.sh $1 parameter-prob requested-option-absent ${@:2}

