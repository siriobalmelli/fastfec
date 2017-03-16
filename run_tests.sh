#!/bin/bash

#TODO: expand test scope
./ffec_test.exe -f 1.05 -o 64000000
poop=$?; if (( $poop )); then exit $poop; fi
