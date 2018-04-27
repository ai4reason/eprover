/*-----------------------------------------------------------------------

  File  : cte_termvars.h

  Author: Stephan Schulz

  Contents

  Functions for the management of shared variables.

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

  Changes

  Created: Tue Feb 24 15:52:12 MET 1998 (Rehacked with parts from the
  now obsolete cte_vartrans.h)

-----------------------------------------------------------------------*/

#ifndef CTE_TERMVARS

#define CTE_TERMVARS

#include <clb_pdarrays.h>
#include <clb_pstacks.h>
#include <cte_termtypes.h>
#include <cte_simplesorts.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

/* Associate FunCodes and cells */
typedef PStack_p VarBankStack_p;

/* Variable banks store information about variables. They contain two
   indices, one associating an external variable name with an internal
   term cell (and f_code, just because a StrTree can store two data
   items...), and one associating an f_code with a term cell. The first
   index is used for parsing and may be incomplete (i.e. not all
   variable cells will be indexed by a string), the second index
   should be complete (i.e. all variable cells have an entry in the
   array). */

typedef struct varbankcell
{
   long        var_count;
   FunCode     fresh_count; /* FunCode counter for new variables */
   SortTable_p sort_table;  /* Sorts that are used for variables */
   FunCode     max_var;       /* Largest variable ever created */
   PDArray_p   varstacks;  /* Maps each sort to a bank of variables
                            * of these sort available to represent
                            * external variables. */
   PDArray_p   v_counts;   /* Number of fresh variables of a given
                            * sort already used. */
   PDArray_p   variables;  /* Array of all variables, indexed by
                              -f_code */
   StrTree_p   ext_index;  /* Associate names and cells */
   PStack_p    env;        /* Scoping environment for quantified
                            * external variables */
}VarBankCell, *VarBank_p;


/* Remembers the association between a variable and a name */
typedef struct varbanknamedcell
{
   Term_p   var;
   char*    name;
}VarBankNamedCell, *VarBankNamed_p;


/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define INITIAL_SORT_STACK_SIZE  10
#define DEFAULT_VARBANK_SIZE   30

/* Variables greater than this are reserved for fresh variables. At
   the moment this is only used for term pattern generation and term
   top computing in the learning modules */
#define FRESH_VAR_LIMIT      1024

#define VarBankCellAlloc() (VarBankCell*)SizeMalloc(sizeof(VarBankCell))
#define VarBankCellFree(junk)    SizeFree(junk, sizeof(VarBankCell))
#define VarBankNamedCellAlloc() (VarBankNamedCell*)SizeMalloc(sizeof(VarBankNamedCell))
#define VarBankNamedCellFree(junk)  SizeFree(junk, sizeof(VarBankNamedCell))


/* Access the stack corresponding to this sort */
static __inline__ VarBankStack_p  VarBankGetStack(VarBank_p bank, SortType sort);

VarBank_p  VarBankAlloc(SortTable_p sort_table);
void       VarBankFree(VarBank_p junk);
void       VarBankResetVCounts(VarBank_p bank);
void       VarBankSetVCountsToUsed(VarBank_p bank);
void       VarBankClearExtNames(VarBank_p bank);
void       VarBankClearExtNamesNoReset(VarBank_p bank);
void       VarBankVarsSetProp(VarBank_p bank, TermProperties prop);
void       VarBankVarsDelProp(VarBank_p bank, TermProperties prop);

VarBankStack_p  VarBankCreateStack(VarBank_p bank, SortType sort);
Term_p VarBankFCodeFind(VarBank_p bank, FunCode f_code);
Term_p VarBankExtNameFind(VarBank_p bank, char* name);
static __inline__ Term_p VarBankVarAssertAlloc(VarBank_p bank, FunCode f_code, SortType sort);
Term_p VarBankVarAlloc(VarBank_p bank, FunCode f_code, SortType sort);
Term_p VarBankGetFreshVar(VarBank_p bank, SortType sort);
Term_p VarBankExtNameAssertAlloc(VarBank_p bank, char* name);
Term_p VarBankExtNameAssertAllocSort(VarBank_p bank, char* name, SortType sort);
void   VarBankPushEnv(VarBank_p bank);
void   VarBankPopEnv(VarBank_p bank);
long   VarBankCardinality(VarBank_p bank);    /* Number of existing variables */
long   VarBankCollectVars(VarBank_p bank, PStack_p stack);
#define VarIsFreshVar(var) ((var)->f_code <= -FRESH_VAR_LIMIT)
#define VarFCodeIsFresh(f_code) ((f_code) <= -FRESH_VAR_LIMIT)


/*---------------------------------------------------------------------*/
/*                         Inline Functions                            */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: VarBankGetStack()
//
//   Obtain a pointer to the stack that stores variables of a given
//   sort.
//
// Global Variables: -
//
// Side Effects    : May modify the varbank, Memory operations
//
/----------------------------------------------------------------------*/

static __inline__ VarBankStack_p  VarBankGetStack(VarBank_p bank, SortType sort)
{
   VarBankStack_p res;

   res = (VarBankStack_p) PDArrayElementP(bank->varstacks, sort);
   if (!res)
   {
      res = VarBankCreateStack(bank, sort);
   }
   return res;
}

/*-----------------------------------------------------------------------
//
// Function: VarBankVarAssertAlloc()
//
//   Return a pointer to the variable with the given f_code and sort in the
//   variable bank. Create the variable if it does not exist.
//
// Global Variables: -
//
// Side Effects    : May change variable bank
//
/----------------------------------------------------------------------*/

static __inline__ Term_p VarBankVarAssertAlloc(VarBank_p bank, FunCode f_code,
                                               SortType sort)
{
   Term_p var;

   assert(f_code < 0);
   var = PDArrayElementP(bank->variables, -f_code);
   if(UNLIKELY(!var))
   {
      var = VarBankVarAlloc(bank, f_code, sort);
   }

   assert(var->v_count==1);
   assert(var->sort != STNoSort);
   assert(var->sort == sort);

   return var;
}


#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/
