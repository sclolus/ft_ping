#!/bin/bash

GENERAL_TEST=./tests/all-errors.sh
rm -f $GENERAL_TEST
echo "#!/bin/bash" >> $GENERAL_TEST

echo "echo-reply default
	destination-unreachable destination-net-unreachable
	destination-unreachable destination-host-unreachable
	destination-unreachable destination-protocol-unreachable
	destination-unreachable destination-port-unreachable
	destination-unreachable fragmentation-needed-and-df-set
	destination-unreachable source-route-failed
	destination-unreachable network-unknown
	destination-unreachable host-unknown
	destination-unreachable host-isolated
	destination-unreachable net-ano
	destination-unreachable host-ano
	destination-unreachable destination-network-unreachable-at-this-tos
	destination-unreachable destination-host-unreachable-at-this-tos
	destination-unreachable pkt-filtered
	destination-unreachable precedence-violation
	destination-unreachable precedence-cutoff
	destination-unreachable destination-host-unreachable-at-this-tos
	source-quench default
	redirect redirect-network
	redirect redirect-host
	redirect redirect-type-of-service-and-network
	redirect redirect-type-of-service-and-host
	echo-request default
	time-exceeded time-to-live-exceeded
	time-exceeded frag-reassembly-time-exceeded
	parameter-prob requested-option-absent
	timestamp default
	timestamp-reply default
	info-request default
	info-reply default
	address-request default
	address-mask-reply default" |

    while read line
    do
	FILENAME="`echo $line | sed 's/ /\-/g'`.sh"
	
	echo "#!/bin/bash 
./scripts/generate-output-custom-errors.sh \$1 $line \${@:2}
" > tests/$FILENAME
	chmod +x tests/$FILENAME

	echo "./tests/$FILENAME \$1 \${@:2} localhost" >> $GENERAL_TEST
    done

chmod +x $GENERAL_TEST
