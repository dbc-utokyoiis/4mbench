#!/bin/sh

echo "### [001] Checking the uniqueness of EQUIPMENTLOG records ..."

../dgen/dgen -l 101 -d 10

cat EQUIPMENTLOG*.dat | perl _001.pl

rm *.dat

echo "### Completed."

# EOF
