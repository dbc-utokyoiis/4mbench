#!/bin/sh

echo "### [003] Checking the distribution of MATERIALLOG reading values ..."

../dgen/dgen -l 101 -d 10

cat MATERIALLOG*.dat | perl _003.pl

rm *.dat

echo "### Completed."

# EOF
