#!/bin/sh

echo "### [002] Checking the distribution of EQUIPMENTLOG reading values ..."

../dgen/dgen -l 101 -d 10

cat EQUIPMENTLOG*.dat | perl _002.pl

rm *.dat

echo "### Completed."

# EOF
