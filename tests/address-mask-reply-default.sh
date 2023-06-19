#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 address-mask-reply default ${@:2}]
./scripts/generate-output-custom-errors.sh $1 address-mask-reply default ${@:2}

