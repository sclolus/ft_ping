#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 timestamp-reply special ${@:2}]
./scripts/generate-output-custom-errors.sh $1 timestamp-reply special ${@:2}

