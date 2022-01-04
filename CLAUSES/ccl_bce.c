/*-----------------------------------------------------------------------

File  : clb_objmaps.h

Author: Petar Vukmirovic

Contents

  Implements blocked clause elimination as described in 
  Blocked Clauses in First-Order Logic (https://doi.org/10.29007/c3wq).

Copyright 1998-2022 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

<1> di  4 jan 2022 13:00:10 CET
-----------------------------------------------------------------------*/

#include "ccl_bce.h"
#include <clb_min_heap.h>

#define OCC_CNT(s) ((s) ? PStackGetSP(s) : 0)
#define IS_BLOCKED(s) (s && PStackGetSP(s) == 0)

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct 
{
   Clause_p parent;
   int lit_idx;
   PStack_p candidates; // NB: Can be null;
   PStackPointer processed_cands;
} BCE_task;

typedef BCE_task* BCE_task_p;

typedef bool (*BlockednessChecker)(BCE_task_p, Clause_p);

#define BCETaskFree(t) (SizeFree((t), sizeof(BCE_task)))

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: make_task()
// 
//   Makes the task for BCE.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

BCE_task_p make_task(Clause_p cl, int lit_idx, PStack_p cands)
{
   BCE_task_p res = SizeMalloc(sizeof(BCE_task));
   res->parent = cl;
   res->lit_idx = lit_idx;
   res->candidates = cands;
   res->processed_cands = 0;
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: compare_taks()
// 
//   Function used to order tasks inside the task queue.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

int compare_taks(IntOrP* ip_a, IntOrP* ip_b)
{
   BCE_task_p a = (BCE_task_p)ip_a;
   BCE_task_p b = (BCE_task_p)ip_b;
   // order by the number of remaining candidates
   return CMP((OCC_CNT(a->candidates)-a->processed_cands),
              (OCC_CNT(b->candidates)-b->processed_cands));
}

/*-----------------------------------------------------------------------
//
// Function: make_sym_map()
// 
//   Performs the elimination of blocked clauses by moving them
//   from passive to archive. Tracking a predicate symbol will be stopped
//   after it reaches max_occs occurrences.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

IntMap_p make_sym_map(ClauseSet_p set, int occ_limit, bool* eq_found)
{
   IntMap_p res = IntMapAlloc();
   for(Clause_p cl = set->anchor->succ; cl!=set->anchor; cl = cl->succ)
   {
      for(Eqn_p lit=cl->literals; lit; lit = lit->next)
      {
         if(!EqnIsEquLit(lit))
         {
            FunCode fc = lit->lterm->f_code * (EqnIsPositive(lit) ? 1 : -1);
            PStack_p* fc_cls = (PStack_p*)IntMapGetRef(res, fc);
            if(!IS_BLOCKED(*fc_cls))
            {
               PStack_p* other_fc_cls = (PStack_p*)IntMapGetRef(res, -fc);
               if((OCC_CNT(*fc_cls) + OCC_CNT(*other_fc_cls)) >= occ_limit)
               {
                  // removing all elements -- essentially blocking clauses
                  if(*fc_cls)
                  {
                     PStackReset(*fc_cls);
                  }
                  else
                  {
                     *fc_cls = PStackAlloc();
                  }

                  if(*other_fc_cls)
                  {
                     PStackReset(*other_fc_cls);
                  }
                  else
                  {
                     *other_fc_cls = PStackAlloc();
                  }
               }
               else
               {
                  if(!*fc_cls)
                  {
                     *fc_cls = PStackAlloc();
                     PStackPushP(*fc_cls, cl);
                  }
                  else if(PStackTopP(*fc_cls) != cl)
                  {
                     // putting only one copy of a clause
                     PStackPushP(*fc_cls, cl);
                  }
               }
            }
         }
         else
         {
            *eq_found = true;
         }
      }
   }
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: make_bce_queue()
// 
//   For each literal in a clause build an object encapsulating
//   all the candidates (and how far we are in checking them), then
//   store it in a queue ordered by number of candidates to check.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

MinHeap_p make_bce_queue(ClauseSet_p set, IntMap_p sym_map)
{
   MinHeap_p res = MinHeapAlloc(compare_taks);
   for(Clause_p cl = set->anchor->succ; cl!=set->anchor; cl = cl->succ)
   {
      int lit_idx = 0;
      for(Eqn_p lit=cl->literals; lit; lit = lit->next)
      {
         if(!EqnIsEquLit(lit))
         {
            FunCode fc = lit->lterm->f_code * (EqnIsPositive(lit) ? 1 : -1);
            PStack_p cands = IntMapGetVal(sym_map, -fc);
            if(!IS_BLOCKED(cands))
            {
               BCE_task_p t = make_task(cl, lit_idx, cands);
               MinHeapAddP(res, t);
            }
         }
         lit_idx++;
      }  
   }
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: check_blockedness_neq()
// 
//   Check if clause all L-resolvents between literal described by task
//   and b are tautologies.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

bool check_blockedness_neq(BCE_task_p task, Clause_p b)
{
   return false;
}

/*-----------------------------------------------------------------------
//
// Function: check_blockedness_eq()
// 
//   Check if clause all equational L-resolvents between literal
//   described by task and b are tautologies.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

bool check_blockedness_eq(BCE_task_p task, Clause_p b)
{
   return false;
}

/*-----------------------------------------------------------------------
//
// Function: check_candidates()
// 
//   Forwards the task either to the first clause that makes it non-blocked.
//   Otherwise, forwards it to the end of the candidates list.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void check_candidates(BCE_task_p t, ClauseSet_p archive, BlockednessChecker f)
{
   assert(!IS_BLOCKED(t->candidates));

   for(;t->processed_cands < OCC_CNT(t->candidates); t->processed_cands++)
   {
      Clause_p cand = PStackElementP(t->candidates, t->processed_cands);
      if(cand != t->parent && cand->set != archive && !f(t, cand))
      {
         break;
      }
   }
}

/*-----------------------------------------------------------------------
//
// Function: resume_task()
// 
//   Forwards to the next candidate and reinserts the task into the queue.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void resume_task(MinHeap_p task_queue, BCE_task_p task)
{
   task->processed_cands++;
   MinHeapAddP(task_queue, task);
}

/*-----------------------------------------------------------------------
//
// Function: do_eliminate_clauses()
// 
//   Performs actual clause elimination
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void free_blocker(void *key, void* val)
{
   PStack_p blocked_tasks = (PStack_p)val;
   while(!PStackEmpty(blocked_tasks))
   {
      BCE_task_p t = PStackPopP(blocked_tasks);
      SizeFree(t, sizeof(BCE_task));
   }
   PStackFree(blocked_tasks);
}

void do_eliminate_clauses(MinHeap_p task_queue, ClauseSet_p archive, 
                          bool has_eq)
{
   PObjMap_p blocker_map = NULL;
   BlockednessChecker checker = 
      has_eq ? check_blockedness_eq : check_blockedness_neq;
   while(MinHeapSize(task_queue))
   {
      BCE_task_p min_task = MinHeapPopMaxP(task_queue);
      if(min_task->parent->set != archive)
      {
         // clause is not archived, we can go on
         check_candidates(min_task, archive, checker);
         if(min_task->processed_cands == OCC_CNT(min_task->candidates))
         {
            // all candidates are processed, clause is blocked
            ClauseSetMoveClause(archive, min_task->parent);

            PStack_p blocked = PObjMapExtract(&blocker_map, min_task->parent, PCmpFun);
            if(blocked)
            {
               while(!PStackEmpty(blocked))
               {
                  resume_task(task_queue, PStackPopP(blocked));
               }
               PStackFree(blocked);
            }
            BCETaskFree(min_task);
         }
         else
         {
            // because of the last checked candidate, clause is not blocked.
            // remember that checking of candidates needs to be continued
            // once the clause which prevented blocking is removed
            PStack_p* blocked = (PStack_p*)PObjMapGetRef(&blocker_map, min_task->parent, PCmpFun, NULL);
            if(!*blocked)
            {
               *blocked = PStackAlloc();
            }
            PStackPushP(*blocked, min_task);
         }
      }
   }
   PObjMapFreeWDeleter(blocker_map, free_blocker);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: EliminateBlockedClauses()
// 
//   Performs the elimination of blocked clauses by moving them
//   from passive to archive. Tracking a predicate symbol will be stopped
//   after it reaches max_occs occurrences.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

void EliminateBlockedClauses(ClauseSet_p passive, ClauseSet_p archive,
                             int max_occs)
{
   bool eq_found = false;
   IntMap_p sym_occs = make_sym_map(passive, max_occs, &eq_found);
   MinHeap_p task_queue = make_bce_queue(passive, sym_occs);
   do_eliminate_clauses(task_queue, archive, eq_found);
   IntMapIter_p iter = IntMapIterAlloc(sym_occs, LONG_MIN, LONG_MAX);
   
   long key;
   PStack_p cls = NULL;
   while( (cls = IntMapIterNext(iter, &key)) )
   {
      PStackFree(cls);
   }
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/



