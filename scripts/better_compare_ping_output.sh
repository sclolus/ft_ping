#!/bin/bash

diff -u $1 $2 | diffr --line-numbers aligned
