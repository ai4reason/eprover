#!/bin/sh

export LD_LIBRARY_PATH=$PWD/../EXTERNAL/tensorflow/lib
../PROVER/eprover -s -H'(1*EnigmaTf(ConstPrio,model,0))' $@

