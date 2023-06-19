#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 info-reply default ${@:2}]
./scripts/generate-output-custom-errors.sh $1 info-reply default ${@:2}

