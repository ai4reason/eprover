/*-----------------------------------------------------------------------

  File  : ccl_proofwatch.c

  Author: Stephan Schulz and AI4REASON

Contents

  ProofWatch interface.  Implements the paper:

  Zarathustra Goertzel, Jan Jakubuv, Stephan Schulz, Josef Urban:
  ProofWatch: Watchlist Guidance for Large Theories in E. 
  ITP 2018: 270-288, https://doi.org/10.1007/978-3-319-94821-8\_16.

  Partially supported by the AI4REASON ERC Consolidator grant number 649043,
  and by the Czech project AI&Reasoning CZ.02.1.01/0.0/0.0/15003/0000466 and
  the European Regional Development Fund.

  Copyright 1998-2019 by the authors.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

-----------------------------------------------------------------------*/

#include "ccl_proofwatch.h"
#include "ccl_derivation.h"



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

bool ProofWatchRecordsProgress = false;
bool ProofWatchInheritsRelevance = false;
double ProofWatchDecay = 0.1;
double ProofWatchAlpha = 0.03;
double ProofWatchBeta = 0.009;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: watch_parents_relevance()
//
//   Compute ProofWatch parents relevance for ProofWatch inherited
//   relevance.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static double watch_parents_relevance(Clause_p clause)
{
   PStackPointer i, sp;
   DerivationCode op;
   Clause_p parent;
   double relevance = 0.0;
   int parents = 0;
  
   if (ClauseQueryProp(clause, CPInitial))
   {
      return 0.0;
   }
   if (!clause->derivation)
   {
      Error("Clause has no derivation.  Are you running with -p option?", USAGE_ERROR);
   }
#ifdef DEBUG_PROOF_WATCH
   fprintf(GlobalOut, "# PARENTS OF ");
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
#endif

   sp = PStackGetSP(clause->derivation);
   i = 0;
   while (i<sp)
   {
      op = PStackElementInt(clause->derivation, i);
      i++;
      
      if(DCOpHasCnfArg1(op))
      {
         parent = PStackElementP(clause->derivation, i);
         relevance += parent->watch_relevance;
         parents++;
    
#ifdef DEBUG_PROOF_WATCH
         fprintf(GlobalOut, "# -> ");
         ClausePrint(GlobalOut, parent, true);
         fprintf(GlobalOut, "\n");
#endif
      }
      if(DCOpHasArg1(op))
      {
         i++;
      }

      if(DCOpHasCnfArg2(op))
      {
         parent = PStackElementP(clause->derivation, i);
         relevance += parent->watch_relevance;
         parents++;

#ifdef DEBUG_PROOF_WATCH
        fprintf(GlobalOut, "# -> ");
        ClausePrint(GlobalOut, parent, true);
        fprintf(GlobalOut, "\n");
#endif
      }
      if(DCOpHasArg2(op))
      {
         i++;
      }
   }

   return (parents>0) ? (relevance/parents) : 0.0;
}

/*-----------------------------------------------------------------------
//
// Function: watch_progress_update()
//
//   Increase the number of matched clauses by one and return the new
//   completion ratio.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static double watch_progress_update(Clause_p watch_clause, 
                                    NumTree_p* watch_progress)
{
   NumTree_p proof;

   assert(watch_clause->watch_proof != -1);

   // find the proof progress statistics ...
   proof = NumTreeFind(watch_progress, watch_clause->watch_proof);
   if (!proof) 
   {
      Error("Unknown proof number (%ld) of a watchlist clause! Should not happen!", 
         OTHER_ERROR, watch_clause->watch_proof);
   }
   
   // ... and update it (val1 matched out of val2 total)
   proof->val1.i_val++;
   
   return (double)proof->val1.i_val/proof->val2.i_val;
}

/*-----------------------------------------------------------------------
//
// Function: watch_progress_get()
//
//   Get the current completion ratio of a given watchlist, that is,
//   how many clauses have been matched so far from watchlist number 
//   proof_no (divided by the watchlist size).
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static double watch_progress_get(NumTree_p* watch_progress, long proof_no)
{
   NumTree_p proof;

   // find the proof progress statistics ...
   proof = NumTreeFind(watch_progress, proof_no);
   if (!proof) 
   {
      Error("Unknown proof number (%ld) of a watchlist clause! Should not happen!", 
         OTHER_ERROR, proof_no);
   }
   
   return (double)proof->val1.i_val/proof->val2.i_val;
}

/*-----------------------------------------------------------------------
//
// Function: watch_progress_print()
//
//   Print the current ProofWatch progress as the ratios of clauses
//   matched so far from each watchlist.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static void watch_progress_print(FILE* out, NumTree_p watch_progress)
{
   NumTree_p proof;
   PStack_p stack;

   fprintf(out, "# ProofWatch proof progress:\n");
   stack = NumTreeTraverseInit(watch_progress);
   while((proof = NumTreeTraverseNext(stack)))
   {
      fprintf(out, "#   watchlist %4ld: %0.3f (%8ld / %8ld)\n",
         proof->key, (double)proof->val1.i_val/proof->val2.i_val,
         proof->val1.i_val, proof->val2.i_val);
   }
   NumTreeTraverseExit(stack);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: ProofWatchSetProofNumber()
//
//   Set the proof number of a watchlist matcher to the most complete
//   watchlist proof.  When more watchlist clauses are matched,
//   this function should be called for every matched clause.  The ratio
//   of the most complete proof is stored in best_progress between the 
//   calls.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ProofWatchSetProofNumber(NumTree_p* watch_progress, Clause_p matcher, 
      Clause_p watchclause, double* best_progress)
{
   double progress;
   if (watch_progress && *watch_progress) 
   {
      progress = watch_progress_update(watchclause, watch_progress);
      if (progress > *best_progress)
      {
         matcher->watch_proof = watchclause->watch_proof;
         *best_progress = progress;
      }
   }
}

/*-----------------------------------------------------------------------
//
// Function: ProofWatchSetRelevance()
//
//   Compute the ProofWatch relevance of a clause.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ProofWatchSetRelevance(NumTree_p* watch_progress, Clause_p matcher)
{
   if (watch_progress && *watch_progress) 
   {
      double proof_progress = 0.0;

      if (matcher->watch_proof > 0)
      {
         proof_progress = watch_progress_get(watch_progress, matcher->watch_proof);
      }

      if (ProofWatchInheritsRelevance)
      { 
         double parents_relevance = watch_parents_relevance(matcher);
         double combined_relevance = proof_progress + (ProofWatchDecay * parents_relevance);

#ifdef DEBUG_PROOF_WATCH
         if (matcher->watch_proof > 0)
         {
            fprintf(GlobalOut, "# PROOFWATCH RELEVANCE: relevance=%1.3f(=%1.3f+%1.3f*%1.3f); proof=%ld; clause=", 
                  combined_relevance,
                  proof_progress,
                  ProofWatchDecay,
                  parents_relevance,
                  matcher->watch_proof);
            ClausePrint(GlobalOut, matcher, true);
            fprintf(GlobalOut, "\n");
         }
#endif
         matcher->watch_relevance = combined_relevance;
      }
      else
      {
#ifdef DEBUG_PROOF_WATCH
         if (matcher->watch_proof > 0)
         {
            fprintf(GlobalOut, "# PROOFWATCH RELEVANCE: relevance=%1.3f; proof=%ld; clause=", 
                  proof_progress,
                  matcher->watch_proof);
            ClausePrint(GlobalOut, matcher, true);
            fprintf(GlobalOut, "\n");
         }
#endif
         matcher->watch_relevance = proof_progress;
      }
   }
}

/*-----------------------------------------------------------------------
//
// Function: ProofWatchPrintProgress()
//
//   Print the current ProofWatch progress as the ratios of clauses
//   matched so far from each watchlist.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ProofWatchPrintProgress(FILE* out, NumTree_p watch_progress)
{
   if (OutputLevel >= 1)
   {
      watch_progress_print(out, watch_progress);
   }
}

/*-----------------------------------------------------------------------
//
// Function: ProofWatchDumpProgress()
//
//   Helper function to print ProofWatch proof progress one line when
//   outputing training examples.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ProofWatchPrintClauseState(FILE* out, Clause_p clause)
{
   NumTree_p proof;
   PStack_p stack;

   stack = NumTreeTraverseInit(clause->watch_proof_state);
   while((proof = NumTreeTraverseNext(stack)))
   {
      fprintf(out, "%ld:%0.3f(%ld/%ld),",
         proof->key, (double)proof->val1.i_val/proof->val2.i_val,
         proof->val1.i_val, proof->val2.i_val);
   }
   NumTreeTraverseExit(stack);
}



/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/
