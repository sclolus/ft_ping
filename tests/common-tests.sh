#!/bin/bash

./scripts/generate-output.sh $1 google.com
./scripts/generate-output.sh $1 -v google.com
./scripts/generate-output.sh $1 192.168.122.1
./scripts/generate-output.sh $1 -v 192.168.122.1
./scripts/generate-output.sh $1 142.250.74.46
./scripts/generate-output.sh $1 -v 142.250.74.46
./scripts/generate-output.sh $1 wikipedia.fr   
./scripts/generate-output.sh $1 -v wikipedia.fr   
./scripts/generate-output.sh $1 localhost
./scripts/generate-output.sh $1 -v localhost
./scripts/generate-output.sh $1 192.168.122.1
