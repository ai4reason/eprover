/*-----------------------------------------------------------------------

File  : che_enigmatensors.h

Author: AI4REASON

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

#ifndef CHE_ENIGMATENSORS

#define CHE_ENIGMATENSORS

#include <cte_termbanks.h>
#include <ccl_clauses.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

#define ETF_TENSOR_SIZE (10*1024*1024)
#define DEBUG_ETF


typedef struct enigmatensorsparamcell
{
   TB_p         tmp_bank;
   long         tmp_bank_vars;

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

   int n_is;
   int n_i1;
   int n_i2;
   int n_i3;
   
   // raw tensors data:
   int32_t ini_nodes[ETF_TENSOR_SIZE];
   int32_t ini_symbols[ETF_TENSOR_SIZE];
   int32_t ini_clauses[ETF_TENSOR_SIZE];
   int32_t clause_inputs_data[ETF_TENSOR_SIZE];
   int32_t clause_inputs_lens[ETF_TENSOR_SIZE];
   int32_t node_c_inputs_data[ETF_TENSOR_SIZE];
   int32_t node_c_inputs_lens[ETF_TENSOR_SIZE];
   int32_t symbol_inputs_nodes[3*ETF_TENSOR_SIZE]; 
   int32_t symbol_inputs_lens[ETF_TENSOR_SIZE]; 
   int32_t node_inputs_1_symbols[ETF_TENSOR_SIZE];
   int32_t node_inputs_1_nodes[2*ETF_TENSOR_SIZE];
   int32_t node_inputs_1_lens[ETF_TENSOR_SIZE];
   int32_t node_inputs_2_symbols[ETF_TENSOR_SIZE];
   int32_t node_inputs_2_nodes[2*ETF_TENSOR_SIZE];
   int32_t node_inputs_2_lens[ETF_TENSOR_SIZE];
   int32_t node_inputs_3_symbols[ETF_TENSOR_SIZE];
   int32_t node_inputs_3_nodes[2*ETF_TENSOR_SIZE];
   int32_t node_inputs_3_lens[ETF_TENSOR_SIZE];
   float symbol_inputs_sgn[ETF_TENSOR_SIZE]; 
   float node_inputs_1_sgn[ETF_TENSOR_SIZE];
   float node_inputs_2_sgn[ETF_TENSOR_SIZE];
   float node_inputs_3_sgn[ETF_TENSOR_SIZE];
   int32_t prob_segments_lens[ETF_TENSOR_SIZE];
   int32_t prob_segments_data[ETF_TENSOR_SIZE];
   int32_t labels[ETF_TENSOR_SIZE];

}EnigmaTensorsCell, *EnigmaTensors_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaTensorsCellAlloc() (EnigmaTensorsCell*) \
        SizeMalloc(sizeof(EnigmaTensorsCell))
#define EnigmaTensorsCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaTensorsCell))

EnigmaTensors_p EnigmaTensorsAlloc(void);
void              EnigmaTensorsFree(EnigmaTensors_p junk);

void EnigmaTensorsUpdateClause(Clause_p clause, EnigmaTensors_p data);

void EnigmaTensorsReset(EnigmaTensors_p data);

void EnigmaTensorsFill(EnigmaTensors_p data);

void EnigmaTensorsDump(FILE* out, EnigmaTensors_p tensors);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

