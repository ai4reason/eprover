#include "torcheval.h"
#include "torch.h"

// for Clause_p
#include <ccl_clauses.h>
// for Eqn_p
#include <ccl_eqn.h>
// for Term_p
#include <cte_termtypes.h>

#include <stdio.h>

static void embed_term(Term_p t, TB_p bank)
{
  if (torch_stack_term_or_negation(t,/* negated= */false)) {
    // found in cache
    return;
  }
  
  // we don't cache simple terms in the main cache
  if (TermIsVar(t)) {
    torch_stack_const("X1");
    return;
  }
  
  char* nm = bank->sig->f_info[t->f_code].name;
  
  if (TermIsConst(t)) {
    torch_stack_const(nm);
    return;
  }

  torch_push();
  for(int i=0; i<t->arity; i++) {
    embed_term(t->args[i],bank);
  }

  // TODO: look up the top symbol name and shit
  torch_embed_and_cache_term(nm,t);
}

static void embed_equality(Term_p l, Term_p r, TB_p bank)
{
  /*
  fprintf(stderr,"embed_equality for ");
  TBPrintTerm(stderr, bank, l, true);
  fprintf(stderr,", ");
  TBPrintTerm(stderr, bank, r, true);
  fprintf(stderr,"\n");
  */

  if (torch_stack_equality_or_negation(l,r,/* negated= */ false)) {
    // found in cache
    return;
  }
  // building instead
  torch_push();
  embed_term(l,bank);
  embed_term(r,bank);
  // implicit pull and store below
  torch_embed_and_cache_equality(l,r);
}

static void embed_literals(Clause_p cl)
{
  for(Eqn_p lit = cl->literals; lit; lit = lit->next)
  {
    bool negated = EqnIsNegative(lit);
    
    /*
    fprintf(stderr,"Literal: neg%d ",negated);
    EqnPrint(stderr,lit,negated,true);
    fprintf(stderr,"\n");
    */
    
    if (lit->rterm->f_code == SIG_TRUE_CODE) { // predicate symbol case:
      if (negated) {
        if (torch_stack_term_or_negation(lit->lterm,/* negated= */true)) {
          // found in cache
          continue;
        }
        // building instead
        torch_push();
        
        embed_term(lit->lterm,lit->bank); // cache aware for both retrieve and store
        
        // implicit pull and store below
        // TODO: this could be unified with torch_embed_and_cache_term, by using the negation symbol
        torch_embed_and_cache_term_negation(lit->lterm);
      } else {
        embed_term(lit->lterm,lit->bank);
      }
    } else { // equality case
      // TODO: think of dealing with commutativity of "="
      if (negated) {
        if (torch_stack_equality_or_negation(lit->lterm,lit->rterm,/* negated= */ true)) {
          // found in cache
          continue;
        }
        // building instead
        torch_push();
      
        embed_equality(lit->lterm,lit->rterm,lit->bank); // cache aware for both retrieve and store
        
        // implicit pull and store below
        torch_embed_and_cache_equality_negation(lit->lterm,lit->rterm);
      } else {
        embed_equality(lit->lterm,lit->rterm,lit->bank);
      }
    }
  }
}

void te_init()
{
  // get ready to collecting clause embeddings
  // torch_push();
}

void te_conjecture_clause(Clause_p cl)
{
  // torch_push();
  
  // embed_literals(cl);
  
  // implicit pull and store below
  // torch_embed_clause(false);
}

void te_conjecture_done()
{
  // implicit pull (corresponds to the push in te_init)
  // torch_embed_conjectures();
}

float te_eval_clause(Clause_p cl)
{
  /*
  fprintf(stderr,"te_eval_clause\n");
  ClausePrint(stderr, cl, true);
  fprintf(stderr,"\n");
  */

  torch_push();

  embed_literals(cl);
  
  // implicit pull
  torch_embed_clause(/* aside= */ true); // setting result aside, where the below function finds it

  return torch_eval_clause();
}
