/*-----------------------------------------------------------------------

File  : che_torchweight.h

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

#ifndef CHE_TORCHWEIGHT

#define CHE_TORCHWEIGHT

#include <ccl_relevance.h>
#include <che_refinedweight.h>


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct torchweightparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;
   
   bool inited;
   char* filename;

   void   (*init_fun)(struct torchweightparamcell*);
}TorchWeightParamCell, *TorchWeightParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define TorchWeightParamCellAlloc() (TorchWeightParamCell*) \
        SizeMalloc(sizeof(TorchWeightParamCell))
#define TorchWeightParamCellFree(junk) \
        SizeFree(junk, sizeof(TorchWeightParamCell))

TorchWeightParam_p TorchWeightParamAlloc(void);
void              TorchWeightParamFree(TorchWeightParam_p junk);


WFCB_p TorchWeightParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p TorchWeightInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* filename);

double TorchWeightCompute(void* data, Clause_p clause);

void TorchWeightExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

