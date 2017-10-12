#!/bin/bash
# The arguments should be passed in this format:
# buildscript.sh <svr's python directory> <python executable> <command for setup.py>
pushd $1 > /dev/null
$2 setup.py $3
popd > /dev/null
