/*-----------------------------------------------------------------------

File  : ccl_subterm_index.h

Author: Stephan Schulz (schulz@eprover.org)

Contents
 
  A simple (hashed) index from terms to clauses in which this term
  appears as priviledged (rewriting rstricted) or unpriviledged term. 


  Copyright 2010 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Wed Aug  5 17:25:30 EDT 2009
    New

-----------------------------------------------------------------------*/

#ifndef CCL_SUBTERM_INDEX

#define CCL_SUBTERM_INDEX

#include <cte_fp_index.h>
#include <ccl_clauses.h>


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

/* Cell for recording all occurances of a subterm.*/

typedef struct subterm_occ_cell
{
   Term_p  term;
   PTree_p rw_rest; /* Of clauses in which the subterm appears in a
                       privileged position with restricted rewriting
                       */ 
   PTree_p rw_full; /* Of clauses in which it appeats unrestricted */ 
}SubtermOccCell, *SubtermOcc_p;



/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define SubtermOccCellAlloc() (SubtermOccCell*)SizeMalloc(sizeof(SubtermOccCell))
#define SubtermOccCellFree(junk) SizeFree(junk, sizeof(SubtermOccCell))

SubtermOcc_p SubtermOccAlloc(Term_p term);
void         SubtermOccFree(SubtermOcc_p soc);

int CmpSubtermCells(const void *soc1, const void *soc2);

void         SubtermTreeFree(PTree_p root);

SubtermOcc_p SubtermTreeInsertTerm(PTree_p *root, Term_p term);

bool         SubtermTreeInsertTermOcc(PTree_p *root, Term_p term, 
                                      Clause_p clause, bool restricted);  
void         SubtermTreeDeleteTerm(PTree_p *root, Term_p term);
void         SubtermTreeDeleteTermOcc(PTree_p *root, Term_p term, 
                                      Clause_p clause, bool restricted);


long         ClauseCollectIdxSubterms(Clause_p clause, 
                                      PTree_p *rest, 
                                      PTree_p *full);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/




