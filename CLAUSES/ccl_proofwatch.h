/*-----------------------------------------------------------------------

File  : ccl_proofwatch.h

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

#ifndef CCL_PROOFWATCH

#define CCL_PROOFWATCH

//#define DEBUG_PROOF_WATCH

#include "ccl_clauses.h"

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

extern bool ProofWatchRecordsProgress;
extern bool ProofWatchInheritsRelevance;
extern double ProofWatchDecay;
extern double ProofWatchAlpha;
extern double ProofWatchBeta;

void ProofWatchSetProofNumber(NumTree_p* watch_progress, Clause_p matcher, 
                         Clause_p watchclause, double* best_progress);

void ProofWatchSetRelevance(NumTree_p* watch_progress, Clause_p matcher);

void ProofWatchPrintProgress(FILE* out, NumTree_p watch_progress);

void ProofWatchPrintClauseState(FILE* out, Clause_p clause);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/
