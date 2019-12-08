#!/bin/sh

export LD_LIBRARY_PATH=$PWD/../EXTERNAL/tensorflow/lib
PROVER="../PROVER/eprover"

$PROVER --print-statistics --resources-info -s --definitional-cnf=24 --split-aggressive   --simul-paramod --forward-context-sr --destructive-er-aggressive --destructive-er --prefer-initial-clauses -tKBO6 -winvfreqrank -c1 -Ginvfreq -F1 --delete-bad-limit=150000000 -WSelectMaxLComplexAvoidPosPred -H'(12*EnigmaTf(ConstPrio,model,0),1*ConjectureTermPrefixWeight(DeferSOS,1,3,0.1,5,0,0.1,1,4),1*ConjectureTermPrefixWeight(DeferSOS,1,3,0.5,100,0,0.2,0.2,4),1*Refinedweight(PreferWatchlist,4,300,4,4,0.7),1*RelevanceLevelWeight2(PreferProcessed,0,1,2,1,1,1,200,200,2.5,9999.9,9999.9),1*StaggeredWeight(DeferSOS,1),1*SymbolTypeweight(DeferSOS,18,7,-2,5,9999.9,2,1.5),2*Clauseweight(PreferWatchlist,20,9999,4),2*ConjectureSymbolWeight(DeferSOS,9999,20,50,-1,50,3,3,0.5),2*StaggeredWeight(DeferSOS,2))' $@

