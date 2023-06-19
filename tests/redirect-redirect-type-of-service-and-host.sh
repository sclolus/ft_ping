#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh $1 redirect redirect-type-of-service-and-host ${@:2}]
./scripts/generate-output-custom-errors.sh $1 redirect redirect-type-of-service-and-host ${@:2}

