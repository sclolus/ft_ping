#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 info-reply special ${@:2}]
./scripts/generate-output-custom-errors.sh $1 info-reply special ${@:2}

