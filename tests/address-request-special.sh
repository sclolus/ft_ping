#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 address-request special ${@:2}]
./scripts/generate-output-custom-errors.sh $1 address-request special ${@:2}

