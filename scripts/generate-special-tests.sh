#!/bin/bash

GENERAL_TEST=./tests/special-tests.sh
rm -f $GENERAL_TEST
echo "#!/bin/bash" >> $GENERAL_TEST

echo "	destination-unreachable
	source-quench
	redirect
	echo-request
	time-exceeded
	timestamp
	timestamp-reply
	info-request
	info-reply
	address-request
	address-mask-reply
	special" |

    while read line
    do
	FILENAME="`echo $line | sed 's/ /\-/g'`-special.sh"
	
	echo "#!/bin/bash
echo Executing [./scripts/generate-output-custom-errors.sh \$1 $line special \${@:2}]
./scripts/generate-output-custom-errors.sh \$1 $line special \${@:2}
" > tests/$FILENAME
	chmod +x tests/$FILENAME

	echo "./tests/$FILENAME \$1 \${@:2} localhost" >> $GENERAL_TEST
    done

chmod +x $GENERAL_TEST
