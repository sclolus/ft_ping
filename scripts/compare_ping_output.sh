#!/bin/bash
# Provide two filenames to be compared word by word

wdiff -n $1 $2 | colordiff
