#!/bin/bash
./tests/destination-unreachable-special.sh $1 ${@:2} localhost
./tests/source-quench-special.sh $1 ${@:2} localhost
./tests/redirect-special.sh $1 ${@:2} localhost
./tests/echo-request-special.sh $1 ${@:2} localhost
./tests/time-exceeded-special.sh $1 ${@:2} localhost
./tests/timestamp-special.sh $1 ${@:2} localhost
./tests/timestamp-reply-special.sh $1 ${@:2} localhost
./tests/info-request-special.sh $1 ${@:2} localhost
./tests/info-reply-special.sh $1 ${@:2} localhost
./tests/address-request-special.sh $1 ${@:2} localhost
./tests/address-mask-reply-special.sh $1 ${@:2} localhost
./tests/special-special.sh $1 ${@:2} localhost
