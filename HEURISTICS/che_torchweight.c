/*-----------------------------------------------------------------------

File  : che_torchweight.c

Author: could be anyone

Contents
 
  Auto generated. Your comment goes here ;-).

  Copyright 2016 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Tue Mar  8 22:40:31 CET 2016
    New

-----------------------------------------------------------------------*/

#include "che_torchweight.h"

#include <torcheval.h>

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/
   
/* HowTo: Iterate over literals:

for(Eqn_p lit = clause->literals; lit; lit = lit->next)
{
   // EqnIsPositive(lit) ...
   if (lit->rterm->f_code == SIG_TRUE_CODE) 
   {
      // predicate symbols other than == and !=
      // use just lit->lterm
   }
   else
   {
      // lit->lterm, lit->rterm, TermIsVar, TermIsConst, 
      // SigIsPredicate(lit->bank->sig, lit->lterm->f_code)
   }
}

*/

static void extweight_init(TorchWeightParam_p data)
{
   if (data->inited) 
   {
      return;
   }

   Clause_p clause;
   Clause_p anchor;

   te_init();

   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if (ClauseQueryTPTPType(clause)==CPTypeNegConjecture) 
      {
         te_conjecture_clause(clause);
      }
   }

   te_conjecture_done();
   data->inited = true;
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

TorchWeightParam_p TorchWeightParamAlloc(void)
{
   TorchWeightParam_p res = TorchWeightParamCellAlloc();
   return res;
}

void TorchWeightParamFree(TorchWeightParam_p junk)
{
   TorchWeightParamCellFree(junk);
}
 
WFCB_p TorchWeightParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, CloseBracket);

   return TorchWeightInit(
      prio_fun, 
      ocb,
      state);
}

WFCB_p TorchWeightInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate)
{
   TorchWeightParam_p data = TorchWeightParamAlloc();

   data->init_fun   = extweight_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   data->inited     = false;
   
   return WFCBAlloc(
      TorchWeightCompute, 
      prio_fun,
      TorchWeightExit, 
      data);
}

double TorchWeightCompute(void* data, Clause_p clause)
{
   TorchWeightParam_p local = data;
   double res = 0.0;
   
   local->init_fun(data);
   
   long long start = GetUSecClock();

   double clen = ClauseWeight(clause,1,1,1,1,1,false);
   double eval = te_eval_clause(clause);

   res = clen * eval;
  
   if (OutputLevel > 1) 
   {
      fprintf(GlobalOut, "=%.2f (torch,t=%.3fms,eval=%.3f,clen=%.1f): ", res, (double)(GetUSecClock() - start)/ 1000.0, eval, clen);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
   }
   
   return res;
}

void TorchWeightExit(void* data)
{
   TorchWeightParam_p junk = data;
   
   TorchWeightParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

