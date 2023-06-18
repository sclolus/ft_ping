#!/bin/bash
./tests/echo-reply.sh $1 ${@:2}
./tests/destination-unreachable-destination-net-unreachable.sh $1 ${@:2}
./tests/destination-unreachable-destination-host-unreachable.sh $1 ${@:2}
./tests/destination-unreachable-destination-protocol-unreachable.sh $1 ${@:2}
./tests/destination-unreachable-destination-port-unreachable.sh $1 ${@:2}
./tests/destination-unreachable-fragmentation-needed-and-df-set.sh $1 ${@:2}
./tests/destination-unreachable-source-route-failed.sh $1 ${@:2}
./tests/destination-unreachable-network-unknown.sh $1 ${@:2}
./tests/destination-unreachable-host-unknown.sh $1 ${@:2}
./tests/destination-unreachable-host-isolated.sh $1 ${@:2}
./tests/destination-unreachable-net-ano.sh $1 ${@:2}
./tests/destination-unreachable-host-ano.sh $1 ${@:2}
./tests/destination-unreachable-destination-network-unreachable-at-this-tos.sh $1 ${@:2}
./tests/destination-unreachable-destination-host-unreachable-at-this-tos.sh $1 ${@:2}
./tests/destination-unreachable-pkt-filtered.sh $1 ${@:2}
./tests/destination-unreachable-precedence-violation.sh $1 ${@:2}
./tests/destination-unreachable-precedence-cutoff.sh $1 ${@:2}
./tests/destination-unreachable-destination-host-unreachable-at-this-tos.sh $1 ${@:2}
./tests/source-quench.sh $1 ${@:2}
./tests/redirect-redirect-network.sh $1 ${@:2}
./tests/redirect-redirect-host.sh $1 ${@:2}
./tests/redirect-redirect-type-of-service-and-network.sh $1 ${@:2}
./tests/redirect-redirect-type-of-service-and-host.sh $1 ${@:2}
./tests/echo-request.sh $1 ${@:2}
./tests/time-exceeded-time-to-live-exceeded.sh $1 ${@:2}
./tests/time-exceeded-frag-reassembly-time-exceeded.sh $1 ${@:2}
./tests/parameter-prob-requested-option-absent.sh $1 ${@:2}
./tests/timestamp.sh $1 ${@:2}
./tests/timestamp-reply.sh $1 ${@:2}
./tests/info-request.sh $1 ${@:2}
./tests/info-reply.sh $1 ${@:2}
./tests/address-request.sh $1 ${@:2}
./tests/address-mask-reply.sh $1 ${@:2}