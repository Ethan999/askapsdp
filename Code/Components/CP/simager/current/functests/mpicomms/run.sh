#!/bin/bash

OUTPUT=output.txt

export AIPSPATH=${ASKAP_ROOT}/Code/Base/accessors/current

if [ ! -x ${ASKAP_ROOT}/Code/Components/CP/simager/current/apps/tMpiComms.sh ]; then
    echo tMpiComms.sh does not exist
    exit 1
fi

mpirun -np 3 ${ASKAP_ROOT}/Code/Components/CP/simager/current/apps/tMpiComms.sh | tee $OUTPUT
if [ $? -ne 0 ]; then
    echo Error: mpirun of tMpiComms.sh returned an error
    exit 1
fi
