/*-----------------------------------------------------------------------

File  : che_enigmaweighttf.h

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

#ifndef CHE_ENIGMAWEIGHTTF

#define CHE_ENIGMAWEIGHTTF

#include <ccl_relevance.h>
#include <che_refinedweight.h>
//#include <che_enigma.h>

#include <tensorflow/c/c_api.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

#define ETF_TENSOR_SIZE (10*1024*1024)
#define ETF_QUERY_CLAUSES 512
#define ETF_CONTEXT_CLAUSES 256
//#define DEBUG_ETF


typedef struct enigmaweighttfparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;
   TB_p         tmp_bank;
   long         tmp_bank_vars;

   char* model_dirname;
   double len_mult;
   bool inited;

   void   (*init_fun)(struct enigmaweighttfparamcell*);

   // clause edges
   NumTree_p terms;
   NumTree_p syms;
   long fresh_t;
   long fresh_s;
   long fresh_c;
   long maxvar;
   PStack_p tedges;
   PStack_p cedges;

   // context
   long context_cnt;

   // conjecture edges
   bool conj_mode;
   NumTree_p conj_terms;
   NumTree_p conj_syms;
   long conj_fresh_t;
   long conj_fresh_s;
   long conj_fresh_c;
   long conj_maxvar;
   PStack_p conj_tedges;
   PStack_p conj_cedges;

   // TensorFlow data
   TF_Graph* graph;
   TF_SessionOptions* options;
   TF_Session* session;
   TF_Buffer* run;
   //TF_Buffer* meta;

   TF_Output inputs[25];
   TF_Tensor* input_values[25];

   TF_Output outputs[1];
   TF_Tensor* output_values[1];

   int n_is;
   int n_i1;
   int n_i2;
   int n_i3;
   
   
}EnigmaWeightTfParamCell, *EnigmaWeightTfParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaWeightTfParamCellAlloc() (EnigmaWeightTfParamCell*) \
        SizeMalloc(sizeof(EnigmaWeightTfParamCell))
#define EnigmaWeightTfParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaWeightTfParamCell))

EnigmaWeightTfParam_p EnigmaWeightTfParamAlloc(void);
void              EnigmaWeightTfParamFree(EnigmaWeightTfParam_p junk);


WFCB_p EnigmaWeightTfParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p EnigmaWeightTfInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_dirname,
   double len_mult);

double EnigmaWeightTfCompute(void* data, Clause_p clause);

void EnigmaComputeEvals(ClauseSet_p set, EnigmaWeightTfParam_p local);

void EnigmaClauseProcessing(Clause_p clause, EnigmaWeightTfParam_p local);

void EnigmaWeightTfExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

