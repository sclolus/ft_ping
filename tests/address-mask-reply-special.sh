#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 address-mask-reply special ${@:2}]
./scripts/generate-output-custom-errors.sh $1 address-mask-reply special ${@:2}

