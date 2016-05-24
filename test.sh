#!/bin/bash
# Project: Carry Look Ahead Parallel Binary Adder using MPI
# Author: Ondøej Krpec, xkrpec01@stud.fit.vutbr.cz
# Date: 1.5.2016
# Description: Test script for implementation of carry look ahead parallel binary adder for subject PRL lectured at FIT VUT in Brno. 

if [ $# != 1 ]; then
    echo "Chyba: Spatny pocet parametru."
    exit 1
fi

mpic++ --prefix /sr/local/share/OpenMPI -o clapba clapba.cpp
mpirun --prefix /usr/local/share/OpenMPI -np $((2 * $1 - 1)) clapba 
rm clapba