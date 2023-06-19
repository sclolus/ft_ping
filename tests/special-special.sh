#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 special special ${@:2}]
./scripts/generate-output-custom-errors.sh $1 special special ${@:2}

