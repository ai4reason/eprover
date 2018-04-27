/*-----------------------------------------------------------------------

  File  : ccl_proofstate.c

  Author: Stephan Schulz

  Contents

  Basic functions for proof state objects.

  Copyright 1998-2017 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

  Created: Wed Oct 14 22:46:13 MET DST 1998

-----------------------------------------------------------------------*/

#include "ccl_proofstate.h"

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

char* UseInlinedWatchList = WATCHLIST_INLINE_STRING;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: clause_set_analyse_gc()
//
//   Count number of clauses, given clauses, and used given clauses.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static void clause_set_analyse_gc(ClauseSet_p set, unsigned long *clause_count,
                           unsigned long *gc_count, unsigned long *gc_used_count)
{
   Clause_p handle;
   unsigned long clause_c = 0, gc_c = 0, gc_used_c = 0;

   for(handle = set->anchor->succ; handle != set->anchor; handle = handle->succ)
   {
      clause_c++;
      if(ClauseIsEvalGC(handle))
      {
         //printf("Clause found (%p): ",set);ClausePrint(stdout, handle, true);printf("\n");
         gc_c++;
         if(ClauseQueryProp(handle, CPIsProofClause))
         {
            gc_used_c++;
         }
      }
   }
   *clause_count  += clause_c;
   *gc_count      += gc_c;
   *gc_used_count += gc_used_c;

   /* printf("# Set %p: Clauses: %7lu GCs: %7lu GCus: %7lu\n",
      set, clause_c, gc_c, gc_used_c); */
}


/*-----------------------------------------------------------------------
//
// Function: clause_set_pick_training_examples()
//
//   Find given clauses and classify them as positive (used in the
//   proof) and negative (not used) examples. Return the two sets via
//   the result-stacks provided.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static void clause_set_pick_training_examples(ClauseSet_p set,
                                             PStack_p pos_examples,
                                             PStack_p neg_examples)
{
   Clause_p handle;

   for(handle = set->anchor->succ; handle != set->anchor; handle = handle->succ)
   {
      if(ClauseIsEvalGC(handle))
      {
         if(ClauseQueryProp(handle, CPIsProofClause))
         {
            PStackPushP(pos_examples, handle);
         }
         else
         {
            PStackPushP(neg_examples, handle);
         }
      }
   }
}





/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: ProofStateAlloc()
//
//   Return an empty, initialized proof state. The argument is:
//   free_symb_prop: Which sub-properties of FPDistinctProp should be
//                   ignored (i.e. which classes with distinct object
//                   syntax  should be treated as plain free
//                   symbols). Use FPIgnoreProps for default
//                   behaviour, FPDistinctProp for fully free
//                   (conventional) semantics.
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

ProofState_p ProofStateAlloc(FunctionProperties free_symb_prop)
{
   ProofState_p handle = ProofStateCellAlloc();

   handle->sort_table           = DefaultSortTableAlloc();
   handle->signature            = SigAlloc(handle->sort_table);
   SigInsertInternalCodes(handle->signature);
   handle->original_symbols     = 0;
   handle->terms                = TBAlloc(handle->signature);
   handle->tmp_terms            = TBAlloc(handle->signature);
   handle->freshvars            = VarBankAlloc(handle->sort_table);
   handle->f_axioms             = FormulaSetAlloc();
   handle->f_ax_archive         = FormulaSetAlloc();
   handle->ax_archive           = ClauseSetAlloc();
   handle->axioms               = ClauseSetAlloc();
   handle->processed_pos_rules  = ClauseSetAlloc();
   handle->processed_pos_eqns   = ClauseSetAlloc();
   handle->processed_neg_units  = ClauseSetAlloc();
   handle->processed_non_units  = ClauseSetAlloc();
   handle->unprocessed          = ClauseSetAlloc();
   handle->tmp_store            = ClauseSetAlloc();
   handle->eval_store           = ClauseSetAlloc();
   handle->archive              = ClauseSetAlloc();
   handle->watchlist            = ClauseSetAlloc();
   handle->f_archive            = FormulaSetAlloc();
   handle->extract_roots        = PStackAlloc();
   GlobalIndicesNull(&(handle->gindices));
   handle->fvi_initialized      = false;
   handle->fvi_cspec            = NULL;
   handle->processed_pos_rules->demod_index = PDTreeAlloc();
   handle->processed_pos_eqns->demod_index  = PDTreeAlloc();
   handle->processed_neg_units->demod_index = PDTreeAlloc();
   handle->demods[0]            = handle->processed_pos_rules;
   handle->demods[1]            = handle->processed_pos_eqns;
   handle->demods[2]            = NULL;
   GlobalIndicesNull(&(handle->wlindices));
   handle->state_is_complete       = true;
   handle->has_interpreted_symbols = false;
   handle->definition_store     = DefStoreAlloc(handle->terms);
   handle->def_store_cspec      = NULL;

   handle->gc_terms             = GCAdminAlloc(handle->terms);
   GCRegisterFormulaSet(handle->gc_terms, handle->f_axioms);
   GCRegisterFormulaSet(handle->gc_terms, handle->f_ax_archive);
   GCRegisterClauseSet(handle->gc_terms, handle->axioms);
   GCRegisterClauseSet(handle->gc_terms, handle->ax_archive);
   GCRegisterClauseSet(handle->gc_terms, handle->processed_pos_rules);
   GCRegisterClauseSet(handle->gc_terms, handle->processed_pos_eqns);
   GCRegisterClauseSet(handle->gc_terms, handle->processed_neg_units);
   GCRegisterClauseSet(handle->gc_terms, handle->processed_non_units);
   GCRegisterClauseSet(handle->gc_terms, handle->unprocessed);
   GCRegisterClauseSet(handle->gc_terms, handle->tmp_store);
   GCRegisterClauseSet(handle->gc_terms, handle->eval_store);
   GCRegisterClauseSet(handle->gc_terms, handle->archive);
   GCRegisterClauseSet(handle->gc_terms, handle->watchlist);
   GCRegisterClauseSet(handle->gc_terms, handle->definition_store->def_clauses);
   GCRegisterFormulaSet(handle->gc_terms, handle->definition_store->def_archive);
   GCRegisterFormulaSet(handle->gc_terms, handle->f_archive);

   handle->status_reported              = false;
   handle->answer_count                 = 0;

   handle->processed_count              = 0;
   handle->proc_trivial_count           = 0;
   handle->proc_forward_subsumed_count  = 0;
   handle->proc_non_trivial_count       = 0;
   handle->other_redundant_count        = 0;
   handle->non_redundant_deleted        = 0;
   handle->backward_subsumed_count      = 0;
   handle->backward_rewritten_count     = 0;
   handle->backward_rewritten_lit_count = 0;
   handle->generated_count              = 0;
   handle->generated_lit_count          = 0;
   handle->non_trivial_generated_count  = 0;
   handle->context_sr_count   = 0;
   handle->paramod_count      = 0;
   handle->factor_count       = 0;
   handle->resolv_count       = 0;
   handle->satcheck_count     = 0;
   handle->satcheck_success   = 0;
   handle->gc_count           = 0;
   handle->gc_used_count      = 0;

   handle->signature->distinct_props =
      handle->signature->distinct_props&(~free_symb_prop);

   handle->watch_progress = NULL;

   return handle;
}




/*-----------------------------------------------------------------------
//
// Function: ProofStateLoadWatchlist()
//
//   Load the watchlist (if requested and not inline), remove it if
//   not requested.
//
// Global Variables: -
//
// Side Effects    : IO, memory ops.
//
/----------------------------------------------------------------------*/

PStack_p ProofStateLoadWatchlist(ProofState_p state,
                                 char* watchlist_filename,
                                 char* watchlist_dirname,
                                 IOFormat parse_format)
{
   if (watchlist_filename && watchlist_dirname)
   {
      Error("Options --watchlist and --watchlist-dir can not be used together.",
         USAGE_ERROR);
   }

   if (watchlist_dirname)
   {
      return ProofStateLoadWatchlistDir(state, watchlist_dirname, parse_format);
   }

   // handles both cases: watchlist_filename and !watchlist_filename
   ProofStateLoadWatchlistFile(state, watchlist_filename, parse_format);
   return NULL;
}

void ProofStateLoadWatchlistFile(ProofState_p state,
                                 char* watchlist_filename,
                                 IOFormat parse_format)
{
   Scanner_p in;

   assert(state->watchlist);

   if(watchlist_filename)
   {
      if (OutputLevel >= 1)
      {
         fprintf(GlobalOut, "# Loading file watchlist from '%s'\n", 
            watchlist_filename);
      }

      if(watchlist_filename!=UseInlinedWatchList)
      {
         in = CreateScanner(StreamTypeFile, watchlist_filename, true, NULL);
         ScannerSetFormat(in, parse_format);
         ClauseSetParseList(in, state->watchlist,
                            state->terms);
         CheckInpTok(in, NoToken);
         DestroyScanner(in);
      }
      ClauseSetSetTPTPType(state->watchlist, CPTypeWatchClause);
      ClauseSetSetProp(state->watchlist, CPWatchOnly);
      ClauseSetDefaultWeighClauses(state->watchlist);
      ClauseSetSortLiterals(state->watchlist, EqnSubsumeInverseCompareRef);
      ClauseSetDocInital(GlobalOut, OutputLevel, state->watchlist);
   }
   else if(!watchlist_filename)
   {
      GCDeregisterClauseSet(state->gc_terms, state->watchlist);
      ClauseSetFree(state->watchlist);
      state->watchlist = NULL;
      if (OutputLevel >= 1)
      {
         fprintf(GlobalOut, "# Watchlist not in use\n");
      }
   }
}

/*-----------------------------------------------------------------------
//
// Function: ProofStateLoadWatchlistDir()
//
//   Load the watchlists from watchlist directory.
//
// Global Variables: -
//
// Side Effects    : IO, memory ops, allocates watchlists stack.
//
/----------------------------------------------------------------------*/

PStack_p ProofStateLoadWatchlistDir(ProofState_p state,
                                    char* watchlist_dir,
                                    IOFormat parse_format)
{
   Scanner_p in;
   DIR *dp;
   struct dirent *ep;
   PStack_p watchlists;
   ClauseSet_p watchlist;
   FormulaSet_p fset;
   DStr_p filename;
   Clause_p handle;
   long proof_no = 0;

   watchlists = PStackAlloc();
   if (!watchlist_dir)
   {
      return watchlists;
   }

   if (OutputLevel >= 1)
   {
      fprintf(GlobalOut, "# Loading directory watchlist from '%s'\n", 
         watchlist_dir);
   }

   dp = opendir(watchlist_dir);
   if (!dp)
   {
      return watchlists;
   }

   while ((ep = readdir(dp)) != NULL)
   {
      if (ep->d_type == DT_DIR)
      {
         continue;
      }

      filename = DStrAlloc();
      DStrAppendStr(filename, watchlist_dir);
      DStrAppendChar(filename, '/');
      DStrAppendStr(filename, ep->d_name);
      in = CreateScanner(StreamTypeFile, DStrView(filename), true, NULL);
      ScannerSetFormat(in, parse_format);

      watchlist = ClauseSetAlloc();
      fset = FormulaSetAlloc();
      FormulaAndClauseSetParse(in, fset, watchlist, state->terms, NULL, NULL);
      CheckInpTok(in, NoToken);
      DestroyScanner(in);

      // move clauses from formulas to watchlist
      while (!FormulaSetEmpty(fset))
      {
         WFormula_p fml = FormulaSetExtractFirst(fset);
         if (fml->is_clause) 
         {
            ClauseSetInsert(watchlist, WFormClauseToClause(fml));
         }
         WFormulaFree(fml);
      }
      FormulaSetFree(fset);

      // stuff taken from ProofStateLoadWatchlist (needed ???)
      ClauseSetSetTPTPType(watchlist, CPTypeWatchClause);
      ClauseSetSetProp(watchlist, CPWatchOnly);
      ClauseSetDefaultWeighClauses(watchlist);
      ClauseSetSortLiterals(watchlist, EqnSubsumeInverseCompareRef);
      ClauseSetDocInital(GlobalOut, OutputLevel, watchlist);

      // set the origin proof number
      for(handle = watchlist->anchor->succ; 
          handle != watchlist->anchor; 
          handle = handle->succ)
      {
         handle->watch_proof = proof_no;
      }
      
      if (OutputLevel >= 1)
      {
         fprintf(GlobalOut, "#   watchlist %4ld: %8ld clauses from '%s'\n", 
            proof_no, watchlist->members, DStrView(filename));
      }

      DStrFree(filename);
      PStackPushP(watchlists, watchlist);
      proof_no++;
   }

   closedir(dp);
   return watchlists;
}


/*-----------------------------------------------------------------------
//
// Function: ProofStateInitWatchlist()
//
//   Initialize the (preloaded) watchlist.
//
// Global Variables: -
//
// Side Effects    : Changes state->axioms, IO, memory ops.
//
/----------------------------------------------------------------------*/

void ProofStateInitWatchlist(ProofState_p state, 
                             OCB_p ocb, 
                             PStack_p watchlists)
{
   if (!watchlists) 
   {
      ProofStateInitWatchlistFile(state, ocb);
   }
   else
   {
      ProofStateInitWatchlistDir(state, ocb, watchlists);
   }
}

void ProofStateInitWatchlistFile(ProofState_p state, OCB_p ocb)
{
   ClauseSet_p tmpset;
   Clause_p handle;

   if(state->watchlist)
   {
      tmpset = ClauseSetAlloc();

      ClauseSetMarkMaximalTerms(ocb, state->watchlist);
      while(!ClauseSetEmpty(state->watchlist))
      {
         handle = ClauseSetExtractFirst(state->watchlist);
         ClauseSetInsert(tmpset, handle);
      }
      ClauseSetIndexedInsertClauseSet(state->watchlist, tmpset);
      ClauseSetFree(tmpset);
      GlobalIndicesInsertClauseSet(&(state->wlindices),state->watchlist);

      if (OutputLevel >= 1)
      {
         fprintf(GlobalOut, "# Total file watchlist clauses: %ld\n", 
            state->watchlist->members);
      }
      // ClauseSetPrint(stdout, state->watchlist, true);
   }
}

/*-----------------------------------------------------------------------
//
// Function: ProofStateInitWatchlistDir()
//
//   Initialize (preloaded) watchlists and watchlists progress.
//
// Global Variables: -
//
// Side Effects    : Changes state->watchlist, IO, memory ops,
//                   frees watchlist.
//
/----------------------------------------------------------------------*/

void ProofStateInitWatchlistDir(ProofState_p state, 
                                OCB_p ocb, 
                                PStack_p watchlists)
{
   ClauseSet_p tmpwatch;
   IntOrP val1, val2;
   long proof_no;

   if (!watchlists || PStackEmpty(watchlists))
   {
      if (OutputLevel >= 1)
      {
         fprintf(GlobalOut, "# Watchlist not in use\n");
      }
      GCDeregisterClauseSet(state->gc_terms, state->watchlist);
      ClauseSetFree(state->watchlist);
      state->watchlist = NULL;
      PStackFree(watchlists);
      return;
   }
      
   // go through all the input watchlists (backwards)
   proof_no = watchlists->current;
   while (!PStackEmpty(watchlists))
   {
      proof_no--;
      // take an input watchlist
      tmpwatch = PStackPopP(watchlists);
      ClauseSetMarkMaximalTerms(ocb, tmpwatch);

      // init proof progress status
      assert(tmpwatch->members > 0);
      val1.i_val = 0; // 0 matched so far ...
      val2.i_val = tmpwatch->members; // ... out of "val2" total
      NumTreeStore(&state->watch_progress, proof_no, val1, val2);

      // insert clauses into a global watchlist
      ClauseSetIndexedInsertClauseSet(state->watchlist, tmpwatch);
      ClauseSetFree(tmpwatch);
   }
  
   if (OutputLevel >= 1)
   {
      fprintf(GlobalOut, "# Total directory watchlist clauses: %ld\n", 
         state->watchlist->members);
   }

   GlobalIndicesInsertClauseSet(&(state->wlindices),state->watchlist);
   PStackFree(watchlists);
      
   //ClauseSetPrint(stdout, state->watchlist, true);
}


/*-----------------------------------------------------------------------
//
// Function: ProofStateResetClauseSets()
//
//   Empty _all_ clause and formula sets in proof state. Keep the
//   signature and term bank. If term_gc is true, perform a garbage
//   collection of term cells.
//
// Global Variables: -
//
// Side Effects    : Memory operations.
//
/----------------------------------------------------------------------*/

void ProofStateResetClauseSets(ProofState_p state, bool term_gc)
{
   ClauseSetFreeClauses(state->axioms);
   FormulaSetFreeFormulas(state->f_axioms);
   FormulaSetFreeFormulas(state->f_ax_archive);
   ClauseSetFreeClauses(state->processed_pos_rules);
   ClauseSetFreeClauses(state->processed_pos_eqns);
   ClauseSetFreeClauses(state->processed_neg_units);
   ClauseSetFreeClauses(state->processed_non_units);
   ClauseSetFreeClauses(state->unprocessed);
   ClauseSetFreeClauses(state->tmp_store);
   ClauseSetFreeClauses(state->eval_store);
   ClauseSetFreeClauses(state->archive);
   ClauseSetFreeClauses(state->ax_archive);
   FormulaSetFreeFormulas(state->f_ax_archive);
   GlobalIndicesReset(&(state->gindices));
   if(state->watchlist)
   {
      ClauseSetFreeClauses(state->watchlist);
      GlobalIndicesReset(&(state->wlindices));
   }
   if(term_gc)
   {
      GCCollect(state->gc_terms);
      //GCCollect(state->gc_original_terms);
   }
}


/*-----------------------------------------------------------------------
//
// Function: ProofStateFree()
//
//   Free a ProofStateCell.
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

void ProofStateFree(ProofState_p junk)
{
   assert(junk);
   ClauseSetFree(junk->axioms);
   FormulaSetFree(junk->f_axioms);
   FormulaSetFree(junk->f_ax_archive);
   ClauseSetFree(junk->processed_pos_rules);
   ClauseSetFree(junk->processed_pos_eqns);
   ClauseSetFree(junk->processed_neg_units);
   ClauseSetFree(junk->processed_non_units);
   ClauseSetFree(junk->unprocessed);
   ClauseSetFree(junk->tmp_store);
   ClauseSetFree(junk->eval_store);
   ClauseSetFree(junk->archive);
   ClauseSetFree(junk->ax_archive);
   FormulaSetFree(junk->f_archive);
   PStackFree(junk->extract_roots);
   GlobalIndicesFreeIndices(&(junk->gindices));
   GCAdminFree(junk->gc_terms);
   //GCAdminFree(junk->gc_original_terms);
   if(junk->watchlist)
   {
      ClauseSetFree(junk->watchlist);
   }
   GlobalIndicesFreeIndices(&(junk->wlindices));

   DefStoreFree(junk->definition_store);
   if(junk->fvi_cspec)
   {
      FVCollectFree(junk->fvi_cspec);
   }
   if(junk->def_store_cspec)
   {
      FVCollectFree(junk->def_store_cspec);
   }

   // junk->original_terms->sig = NULL;
   junk->terms->sig = NULL;
   junk->tmp_terms->sig = NULL;
   SigFree(junk->signature);
   // TBFree(junk->original_terms);
   TBFree(junk->terms);
   TBFree(junk->tmp_terms);
   VarBankFree(junk->freshvars);
   SortTableFree(junk->sort_table);

   ProofStateCellFree(junk);
}


/*-----------------------------------------------------------------------
//
// Function: ProofStateIsUntyped()
//
//   Return true if all clauses in the proof state are untyped. Does
//   not check formulas!
//
// Global Variables: -
//
// Side Effects    :  0
//
/----------------------------------------------------------------------*/

bool ProofStateIsUntyped(ProofState_p state)
{
   return ClauseSetIsUntyped(state->processed_pos_rules)
      && ClauseSetIsUntyped(state->processed_pos_eqns)
      && ClauseSetIsUntyped(state->processed_neg_units)
      && ClauseSetIsUntyped(state->processed_non_units)
      && ClauseSetIsUntyped(state->unprocessed);
}



/*-----------------------------------------------------------------------
//
// Function: ProofStateAnalyseGC()
//
//   Run an analysis of the use of given clauses in the proof search:
//   How many were used (i.e. useful) and how many were unused
//   (i.e. useless).
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ProofStateAnalyseGC(ProofState_p state)
{
   unsigned long clause_c = 0;

   clause_set_analyse_gc(state->ax_archive, &clause_c,
                         &(state->gc_count), &(state->gc_used_count));
   clause_set_analyse_gc(state->processed_pos_rules, &clause_c,
                         &(state->gc_count), &(state->gc_used_count));
   clause_set_analyse_gc(state->processed_pos_eqns, &clause_c,
                         &(state->gc_count), &(state->gc_used_count));
   clause_set_analyse_gc(state->processed_neg_units, &clause_c,
                         &(state->gc_count), &(state->gc_used_count));
   clause_set_analyse_gc(state->processed_non_units, &clause_c,
                         &(state->gc_count), &(state->gc_used_count));
   clause_set_analyse_gc(state->archive, &clause_c,
                         &(state->gc_count), &(state->gc_used_count));
}



/*-----------------------------------------------------------------------
//
// Function: ProofStatePickTrainingExamples()
//
//   Find positive and negative training examples in the proof state.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void ProofStatePickTrainingExamples(ProofState_p state,
                                    PStack_p pos_examples,
                                    PStack_p neg_examples)
{
   clause_set_pick_training_examples(state->ax_archive, pos_examples,
                                     neg_examples);
   clause_set_pick_training_examples(state->processed_pos_rules,
                                     pos_examples, neg_examples);
   clause_set_pick_training_examples(state->processed_pos_eqns,
                                     pos_examples, neg_examples);
   clause_set_pick_training_examples(state->processed_neg_units,
                                     pos_examples, neg_examples);
   clause_set_pick_training_examples(state->processed_non_units,
                                     pos_examples, neg_examples);
   clause_set_pick_training_examples(state->archive,
                                     pos_examples, neg_examples);
}


/*-----------------------------------------------------------------------
//
// Function: ProofStateTrain()
//
//   Perform some (yet to be specified ;-) training on the proof
//   state.
//
// Global Variables: -
//
// Side Effects    : Outpur
//
/----------------------------------------------------------------------*/

void ProofStateTrain(ProofState_p state, bool print_pos, bool print_neg)
{
   PStack_p
      pos_examples = PStackAlloc(),
      neg_examples = PStackAlloc();

   ProofStatePickTrainingExamples(state, pos_examples, neg_examples);

   fprintf(GlobalOut, "# Training examples: %ld positive, %ld negative\n",
           PStackGetSP(pos_examples), PStackGetSP(neg_examples));
   if(print_pos)
   {
      fprintf(GlobalOut, "# Training: Positive examples begin\n");
      PStackClausePrint(GlobalOut, pos_examples, "# trainpos");
      fprintf(GlobalOut, "# Training: Positive examples end\n");
   }
   if(print_neg)
   {
      fprintf(GlobalOut, "# Training: Negative examples begin\n");
      PStackClausePrint(GlobalOut, neg_examples, "#trainneg");
      fprintf(GlobalOut, "# Training: Negative examples end\n");
   }

   PStackFree(pos_examples);
   PStackFree(neg_examples);
}




/*-----------------------------------------------------------------------
//
// Function: ProofStateStatisticsPrint()
//
//   Print the statistics of the proof state.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ProofStateStatisticsPrint(FILE* out, ProofState_p state)
{
   fprintf(out, "# Initial clauses in saturation        : %ld\n",
      state->axioms->members);
   fprintf(out, "# Processed clauses                    : %ld\n",
      state->processed_count);
   fprintf(out, "# ...of these trivial                  : %ld\n",
      state->proc_trivial_count);
   fprintf(out, "# ...subsumed                          : %ld\n",
      state->proc_forward_subsumed_count);
   fprintf(out, "# ...remaining for further processing  : %ld\n",
      state->proc_non_trivial_count);
   fprintf(out, "# Other redundant clauses eliminated   : %ld\n",
      state->other_redundant_count);
   fprintf(out, "# Clauses deleted for lack of memory   : %ld\n",
      state->non_redundant_deleted);
   fprintf(out, "# Backward-subsumed                    : %ld\n",
      state->backward_subsumed_count);
   fprintf(out, "# Backward-rewritten                   : %ld\n",
      state->backward_rewritten_count);
   fprintf(out, "# Generated clauses                    : %ld\n",
      state->generated_count - state->backward_rewritten_count);
   fprintf(out, "# ...of the previous two non-trivial   : %ld\n",
      state->non_trivial_generated_count);
   fprintf(out, "# Contextual simplify-reflections      : %ld\n",
      state->context_sr_count);
   fprintf(out, "# Paramodulations                      : %ld\n",
      state->paramod_count);
   fprintf(out, "# Factorizations                       : %ld\n",
      state->factor_count);
   fprintf(out, "# Equation resolutions                 : %ld\n",
      state->resolv_count);
   fprintf(out, "# Propositional unsat checks           : %ld\n",
      state->satcheck_count);
   fprintf(out, "# Propositional unsat check successes  : %ld\n",
      state->satcheck_success);
   fprintf(out,
      "# Current number of processed clauses  : %ld\n"
      "#    Positive orientable unit clauses  : %ld\n"
      "#    Positive unorientable unit clauses: %ld\n"
      "#    Negative unit clauses             : %ld\n"
      "#    Non-unit-clauses                  : %ld\n",
      state->processed_pos_rules->members+
      state->processed_pos_eqns->members+
      state->processed_neg_units->members+
      state->processed_non_units->members,
      state->processed_pos_rules->members,
      state->processed_pos_eqns->members,
      state->processed_neg_units->members,
      state->processed_non_units->members);
   fprintf(out,
      "# Current number of unprocessed clauses: %ld\n",
      state->unprocessed->members);
   fprintf(out,
      "# ...number of literals in the above   : %ld\n",
      state->unprocessed->literals);
   fprintf(out,
      "# Current number of archived formulas  : %ld\n",
      state->f_archive->members);
   fprintf(out,
      "# Current number of archived clauses   : %ld\n",
      state->archive->members);
   if(ProofObjectRecordsGCSelection)
   {
      fprintf(out,
              "# Proof object given clauses           : %ld\n",
              state->gc_used_count);
      fprintf(out,
              "# Proof search given clauses           : %ld\n",
              state->gc_count);
   }
   if(TBPrintDetails)
   {
      fprintf(out,
         "# Total literals in generated clauses  : %ld\n",
         state->generated_lit_count -
         state->backward_rewritten_lit_count);
      fprintf(out,
         "# Shared term nodes                    : %ld\n"
         "# ...corresponding unshared nodes      : %ld\n",
         TBTermNodes(state->terms),
         ClauseSetGetTermNodes(state->tmp_store)+
         ClauseSetGetTermNodes(state->eval_store)+
         ClauseSetGetTermNodes(state->processed_pos_rules)+
         ClauseSetGetTermNodes(state->processed_pos_eqns)+
         ClauseSetGetTermNodes(state->processed_neg_units)+
         ClauseSetGetTermNodes(state->processed_non_units)+
         ClauseSetGetTermNodes(state->unprocessed));
      fprintf(out,
         "# Shared rewrite steps                 : %lu\n",
         state->terms->rewrite_steps);
      fprintf(out,
         "# Match attempts with oriented units   : %lu\n"
         "# Match attempts with unoriented units : %lu\n",
         state->processed_pos_rules->demod_index->match_count,
         state->processed_pos_eqns->demod_index->match_count);
#ifdef MEASURE_EXPENSIVE
      fprintf(out,
         "# Oriented PDT nodes visited           : %lu\n"
         "# Unoriented PDT nodes visited         : %lu\n",
         state->processed_pos_rules->demod_index->visited_count,
         state->processed_pos_eqns->demod_index->visited_count);
#endif
   }
   /* TermCellStorePrintDistrib(out, &(state->terms->term_store)); */
   /* TBPrintTermsFlat=false;
      TBPrintInternalInfo=true;
      TBPrintBankInOrder(stdout,state->terms);*/
}

/*-----------------------------------------------------------------------
//
// Function: ProofStatePrint()
//
//   Print the clause sets of the proof state.
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ProofStatePrint(FILE* out, ProofState_p state)
{
   fprintf(out, "\n# Processed positive unit clauses:\n");
   ClauseSetPrint(out, state->processed_pos_rules, true);
   ClauseSetPrint(out, state->processed_pos_eqns, true);
   fprintf(out, "\n# Processed negative unit clauses:\n");
   ClauseSetPrint(out, state->processed_neg_units, true);
   fprintf(out, "\n# Processed non-unit clauses:\n");
   ClauseSetPrint(out, state->processed_non_units, true);
   fprintf(out, "\n# Unprocessed clauses:\n");
   ClauseSetPrint(out, state->unprocessed, true);
}

/*-----------------------------------------------------------------------
//
// Function: ProofStatePropDocQuote()
//
//   Print all clauses in the main clause sets in state for which
//   props is true (if outputlevel is large enough, as defined in
//   ClauseSetPropDocQuote().
//
// Global Variables: -
//
// Side Effects    : Output
//
/----------------------------------------------------------------------*/

void ProofStatePropDocQuote(FILE* out, int level,
             FormulaProperties prop,
             ProofState_p state, char* comment)
{
   ClauseSetPropDocQuote(GlobalOut, level, prop,
          state->processed_pos_rules, comment);
   ClauseSetPropDocQuote(GlobalOut, level, prop,
          state->processed_pos_eqns, comment);
   ClauseSetPropDocQuote(GlobalOut, level, prop,
          state->processed_neg_units, comment);
   ClauseSetPropDocQuote(GlobalOut, level, prop,
          state->processed_non_units, comment);
   ClauseSetPropDocQuote(GlobalOut, level, prop,
          state->unprocessed, comment);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/
