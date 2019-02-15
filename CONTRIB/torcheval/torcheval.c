#include "torcheval.h"
#include "torch.h"

// for Eqn_p
#include <ccl_eqn.h>
// for Term_p
#include <cte_termtypes.h>

#include <stdio.h>

void te_conjecture_clause(Clause_p cl)
{
  // assert(TermIsShared(t));
}

void te_conjecture_done()
{
  torch_embed_conjectures();
}

float te_eval_clause(Clause_p t)
{
  return 0.0;
}
