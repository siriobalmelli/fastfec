#!/bin/bash

./bits_test.exe s
poop=$?; if (( $poop )); then exit $poop; fi
