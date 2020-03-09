/*-----------------------------------------------------------------------

File  : che_enigmaweighttf.c

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

#include "che_enigmaweighttf.h"
#include "cco_proofproc.h"

#include <tensorflow/c/c_api_experimental.h>

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

// FIXME: This will not work with different Tensorflow model!
static EnigmaWeightTfParam_p saved_local = NULL;


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/


#ifdef DEBUG_ETF
static void debug_symbols(EnigmaWeightTfParam_p data)
{  
   PStack_p stack;
   NumTree_p node;
   
   fprintf(GlobalOut, "#TF# Symbols map:\n");
   fprintf(GlobalOut, "#TF# (conjecture):\n");
   stack = NumTreeTraverseInit(data->conj_syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   s%ld: %s\n", node->val1.i_val, node->key ? 
         SigFindName(data->proofstate->signature, node->key) : "=");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses):\n");
   stack = NumTreeTraverseInit(data->syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   s%ld: %s\n", node->val1.i_val, node->key ? 
         SigFindName(data->proofstate->signature, node->key) : "=");
   }
   NumTreeTraverseExit(stack);
}

static void debug_terms(EnigmaWeightTfParam_p data)
{  
   PStack_p stack;
   NumTree_p node;
   
   fprintf(GlobalOut, "#TF# Terms map:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   stack = NumTreeTraverseInit(data->conj_terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: %s", node->val1.i_val, (node->key % 2 == 1) ? "~" : "");
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses)\n");
   stack = NumTreeTraverseInit(data->terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: %s", node->val1.i_val, (node->key % 2 == 1) ? "~" : "");
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
}

static void debug_edges(EnigmaWeightTfParam_p data)
{
   long i;
   
   fprintf(GlobalOut, "#TF# Clause edges:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   for (i=0; i<data->conj_cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->conj_cedges, i);
      fprintf(GlobalOut, "#TF#   (c%ld, t%ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1));
   }
   fprintf(GlobalOut, "#TF# (clauses)\n");
   for (i=0; i<data->cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->cedges, i);
      fprintf(GlobalOut, "#TF#   (c%ld, t%ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1));
   }

   fprintf(GlobalOut, "#TF# Term edges:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   for (i=0; i<data->conj_tedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->conj_tedges, i);
      fprintf(GlobalOut, "#TF#   (t%ld, t%ld, t%ld, s%ld, %ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1),
         PDArrayElementInt(edge, 2), PDArrayElementInt(edge, 3),
         PDArrayElementInt(edge, 4));
   }
   fprintf(GlobalOut, "#TF# (clauses)\n");
   for (i=0; i<data->tedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->tedges, i);
      fprintf(GlobalOut, "#TF#   (t%ld, t%ld, t%ld, s%ld, %ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1),
         PDArrayElementInt(edge, 2), PDArrayElementInt(edge, 3),
         PDArrayElementInt(edge, 4));
   }

}

static void debug_vector_float(char* name, float* vals, int len, char* tfid, int idx) 
{
   fprintf(GlobalOut, "#TF# %s: index=%d op=%s\n", name, idx, tfid);
   fprintf(GlobalOut, "\t%s[%d] = [ ", name, len);
   for (int i=0; i<len; i++)
   {
      fprintf(GlobalOut, "%.02f%s", vals[i], (i<len-1) ? ", " : " ]\n");
   }
}

static void debug_vector_int32(char* name, int32_t* vals, int len, char* tfid, int idx)
{
   fprintf(GlobalOut, "#TF# %s: index=%d op=%s\n", name, idx, tfid);
   fprintf(GlobalOut, "\t%s[%d] = [ ", name, len);
   for (int i=0; i<len; i++)
   {
      fprintf(GlobalOut, "%d%s", vals[i], (i<len-1) ? ", " : " ]\n");
   }
}

static void debug_matrix(char* name, int32_t* vals, int dimx, int dimy, char* tfid, int idx0) 
{
   fprintf(GlobalOut, "#TF# %s: index=%d op=%s\n", name, idx0, tfid);
   fprintf(GlobalOut, "\t%s[%d,%d] = [ ", name, dimx, dimy);
   int idx = 0;
   for (int x=0; x<dimx; x++)
   {
      fprintf(GlobalOut, "[");
      for (int y=0; y<dimy; y++)
      {
         fprintf(GlobalOut, "%d%s", vals[idx++], (y<dimy-1) ? "," : "]");
      }
      fprintf(GlobalOut, "%s", (x<dimx-1) ? ", " : " ]\n");
   }
}
#endif

void idle_deallocator(void* data, size_t len, void* arg)
{
}

void set_input_vector_int32(int idx, int size, char* id, int32_t* values, 
   char* name, EnigmaWeightTfParam_p data)
{
   data->inputs[idx].oper = TF_GraphOperationByName(data->graph, name);
   data->inputs[idx].index = 0;

   int64_t dims[1];
   dims[0] = size;
   data->input_values[idx] = TF_NewTensor(
      TF_INT32, dims, 1, values, size*sizeof(int32_t), idle_deallocator, NULL);
   
#ifdef DEBUG_ETF 
   debug_vector_int32(id, values, size, name, idx);
#endif
}

void set_input_vector_float(int idx, int size, char* id, float* values, 
   char* name, EnigmaWeightTfParam_p data)
{
   data->inputs[idx].oper = TF_GraphOperationByName(data->graph, name);
   data->inputs[idx].index = 0;

   int64_t dims[1];
   dims[0] = size;
   data->input_values[idx] = TF_NewTensor(
      TF_FLOAT, dims, 1, values, size*sizeof(float), idle_deallocator, NULL);
   
#ifdef DEBUG_ETF
   debug_vector_float(id, values, size, name, idx);
#endif
}

void set_input_matrix(int idx, int dimx, int dimy, char* id, int32_t* values, 
   char* name, EnigmaWeightTfParam_p data)
{
   data->inputs[idx].oper = TF_GraphOperationByName(data->graph, name);
   data->inputs[idx].index = 0;

   int64_t dims[2];
   dims[0] = dimx;
   dims[1] = dimy;
   int64_t size = dimx * dimy;
   data->input_values[idx] = TF_NewTensor(
      TF_INT32, dims, 2, values, size*sizeof(int32_t), idle_deallocator, NULL);
  
#ifdef DEBUG_ETF 
   debug_matrix(id, values, dimx, dimy, name, idx);
#endif
}

static void tensor_fill_input(EnigmaWeightTfParam_p data)
{
   EnigmaTensorsFill(data->tensors);

   int n_ce = data->tensors->cedges->current + data->tensors->conj_cedges->current;
   //int n_te = data->tensors->tedges->current + data->tensors->conj_tedges->current;
   int n_s = data->tensors->fresh_s;
   int n_c = data->tensors->fresh_c;
   int n_t = data->tensors->fresh_t;
   int n_q = n_c - (data->tensors->conj_fresh_c - data->tensors->context_cnt); // query clauses (evaluated and context clauses)
   int n_is = data->tensors->n_is;
   int n_i1 = data->tensors->n_i1;
   int n_i2 = data->tensors->n_i2;
   int n_i3 = data->tensors->n_i3;

   // set vectors to data
   set_input_vector_int32(0, n_t, "ini_nodes", data->tensors->ini_nodes, "GraphPlaceholder/ini_nodes", data);
   set_input_vector_int32(1, n_s, "ini_symbols", data->tensors->ini_symbols, "GraphPlaceholder/ini_symbols", data);
   set_input_vector_int32(2, n_c, "ini_clauses", data->tensors->ini_clauses, "GraphPlaceholder/ini_clauses", data);
   set_input_vector_int32(3, n_t, "node_inputs_1_lens", data->tensors->node_inputs_1_lens, "GraphPlaceholder/GraphHyperEdgesA/segment_lens", data);
   set_input_vector_int32(4, n_i1, "node_inputs_1_symbols", data->tensors->node_inputs_1_symbols, "GraphPlaceholder/GraphHyperEdgesA/symbols", data);
   set_input_vector_float(5, n_i1, "node_inputs_1_sgn", data->tensors->node_inputs_1_sgn, "GraphPlaceholder/GraphHyperEdgesA/sgn", data);
   set_input_vector_int32(6, n_t, "node_inputs_2_lens", data->tensors->node_inputs_2_lens, "GraphPlaceholder/GraphHyperEdgesA_1/segment_lens", data);
   set_input_vector_int32(7, n_i2, "node_inputs_2_symbols", data->tensors->node_inputs_2_symbols, "GraphPlaceholder/GraphHyperEdgesA_1/symbols", data);
   set_input_vector_float(8, n_i2, "node_inputs_2_sgn", data->tensors->node_inputs_2_sgn, "GraphPlaceholder/GraphHyperEdgesA_1/sgn", data);
   set_input_vector_int32(9, n_t, "node_inputs_3_lens", data->tensors->node_inputs_3_lens, "GraphPlaceholder/GraphHyperEdgesA_2/segment_lens", data);
   set_input_vector_int32(10, n_i3, "node_inputs_3_symbols", data->tensors->node_inputs_3_symbols, "GraphPlaceholder/GraphHyperEdgesA_2/symbols", data);
   set_input_vector_float(11, n_i3, "node_inputs_3_sgn", data->tensors->node_inputs_3_sgn, "GraphPlaceholder/GraphHyperEdgesA_2/sgn", data);
   set_input_vector_int32(12, n_s, "symbol_inputs_lens", data->tensors->symbol_inputs_lens, "GraphPlaceholder/GraphHyperEdgesB/segment_lens", data);
   set_input_vector_float(13, n_is, "symbol_inputs_sgn", data->tensors->symbol_inputs_sgn, "GraphPlaceholder/GraphHyperEdgesB/sgn", data);
   set_input_vector_int32(14, n_t, "node_c_inputs_lens", data->tensors->node_c_inputs_lens, "GraphPlaceholder/GraphEdges/segment_lens", data);
   set_input_vector_int32(15, n_ce, "node_c_inputs_data", data->tensors->node_c_inputs_data, "GraphPlaceholder/GraphEdges/data", data);
   set_input_vector_int32(16, n_c, "clause_inputs_lens", data->tensors->clause_inputs_lens, "GraphPlaceholder/GraphEdges_1/segment_lens", data);
   set_input_vector_int32(17, n_ce, "clause_inputs_data", data->tensors->clause_inputs_data, "GraphPlaceholder/GraphEdges_1/data", data);
   set_input_vector_int32(18, 1, "prob_segments_lens", data->tensors->prob_segments_lens, "segment_lens", data);
   set_input_vector_int32(19, 1+n_q, "prob_segments_data", data->tensors->prob_segments_data, "segment_data", data);
   set_input_vector_int32(20, n_q, "labels", data->tensors->labels, "Placeholder", data);
   set_input_matrix(21, n_i1, 2, "node_inputs_1_nodes", data->tensors->node_inputs_1_nodes, "GraphPlaceholder/GraphHyperEdgesA/nodes", data);
   set_input_matrix(22, n_i2, 2, "node_inputs_2_nodes", data->tensors->node_inputs_2_nodes, "GraphPlaceholder/GraphHyperEdgesA_1/nodes", data);
   set_input_matrix(23, n_i3, 2, "node_inputs_3_nodes", data->tensors->node_inputs_3_nodes, "GraphPlaceholder/GraphHyperEdgesA_2/nodes", data);
   set_input_matrix(24, n_is, 3, "symbol_inputs_nodes", data->tensors->symbol_inputs_nodes, "GraphPlaceholder/GraphHyperEdgesB/nodes", data);
}

static void tensor_fill_output(EnigmaWeightTfParam_p data)
{
   static float logits[ETF_TENSOR_SIZE];

   data->outputs[0].oper = TF_GraphOperationByName(data->graph, "Squeeze");
   data->outputs[0].index = 0;
   
   int n_q = data->fresh_c - (data->conj_fresh_c - data->context_cnt);
   int64_t out_dims[1];
   out_dims[0] = n_q;
   data->output_values[0] = TF_NewTensor(TF_FLOAT, out_dims, 1, logits, 
      n_q*sizeof(float), idle_deallocator, NULL);
}

static void tensor_eval(EnigmaWeightTfParam_p data)
{
   TF_Status* status = TF_NewStatus();

   TF_SessionRun(
       data->session,
       // RunOptions
       NULL, //data->run,
       // Input tensors
       data->inputs, 
       data->input_values, 
       25, 
       // Output tensors
       data->outputs, 
       data->output_values, 
       1, 
       // Target operations
       NULL, 
       0, 
       // RunMetadata
       NULL,
       // Output status
       status
   );
   
   if (TF_GetCode(status) != TF_OK)
   {
      Error("Enigma: Tensorflow: %s\n", OTHER_ERROR, TF_Message(status));
   }

   TF_DeleteStatus(status);
}

static void tensor_free(EnigmaWeightTfParam_p data)
{
   for (int i=0; i<25; i++)
   {
      TF_DeleteTensor(data->input_values[i]);
      data->input_values[i] = NULL;
      data->inputs[i].oper = NULL;
   }

   TF_DeleteTensor(data->output_values[0]);
   data->outputs[0].oper = NULL;
}
  
/*
static double tensor_transform(EnigmaWeightTfParam_p data)
{
   int n_q = data->fresh_c - data->conj_fresh_c;
   float* logits = TF_TensorData(data->output_values[0]);
#ifdef DEBUG_ETF
   debug_vector_float("logits", logits, n_q, "Squeeze", 0);
#endif

   double val = logits[n_q-1];
   //return -val;
   return (val > 0.0) ? 1 : 10;
}
*/

static void extweight_init(EnigmaWeightTfParam_p data)
{
   Clause_p clause;
   Clause_p anchor;

   if (data->inited)
   {
      return;
   }

   fprintf(GlobalOut, "# ENIGMA: TensorFlow C library version %s\n", TF_Version());
   // process conjectures
   data->tensors->tmp_bank = TBAlloc(data->proofstate->signature);
   data->conj_mode = true;
   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture) 
      {
         EnigmaTensorsUpdateClause(clause, data->tensors);
      }
   }
   data->conj_mode = false;
   data->conj_maxvar = data->maxvar; // save maxvar to restore
   EnigmaTensorsReset(data->tensors);

   // init tensorflow
   data->graph = TF_NewGraph();
   data->options = TF_NewSessionOptions();
   data->run = TF_NewBuffer();
   TF_Status* status = TF_NewStatus();

   const char* tags[1] = {"serve"};

   TF_Buffer* config = TF_CreateConfig(0, 0, 1);
   TF_SetConfig(data->options, config->data, config->length, status);
   // TODO: free config

   data->session = TF_LoadSessionFromSavedModel(
      data->options, 
      NULL, // NULL, // const TF_Buffer* run_options,
      data->model_dirname, 
      tags, 
      1,
      data->graph, 
      NULL, // NULL, //TF_Buffer* meta_graph_def, 
      status
   );

   if (TF_GetCode(status) != TF_OK)
   {
      Error("Enigma: Tensorflow: %s\n", OTHER_ERROR, TF_Message(status));
   }
   TF_DeleteStatus(status);

   fprintf(GlobalOut, "# ENIGMA: TensorFlow: model '%s' loaded (query=%ld; context=%ld; weigths=%s; len_mult=%f).\n", 
      data->model_dirname, DelayedEvalSize, data->context_size,
      (data->binary_weights ? "binary" : "real"), data->len_mult);

   data->inited = true;
}


/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

//void EnigmaTensorsUpdateClause(Clause_p clause, EnigmaWeightTfParam_p data)

EnigmaWeightTfParam_p EnigmaWeightTfParamAlloc(void)
{
   EnigmaWeightTfParam_p res = EnigmaWeightTfParamCellAlloc();

   res->inited = false;

   res->terms = NULL;
   res->syms = NULL;
   res->fresh_t = 0;
   res->fresh_s = 0;
   res->fresh_c = 0;
   res->tedges = PStackAlloc();
   res->cedges = PStackAlloc();

   res->context_cnt = 0;
   
   res->conj_mode = false;
   res->conj_terms = NULL;
   res->conj_syms = NULL;
   res->conj_fresh_t = 0;
   res->conj_fresh_s = 0;
   res->conj_fresh_c = 0;
   res->conj_tedges = PStackAlloc();
   res->conj_cedges = PStackAlloc();

   res->maxvar = 0;
   res->tensors = EnigmaTensorsAlloc();

   return res;
}

void EnigmaWeightTfParamFree(EnigmaWeightTfParam_p junk)
{
   free(junk->model_dirname);
   
   if (!junk->inited)
   {
      return;
   }

   EnigmaTensorsFree(junk->tensors);
   junk->tensors = NULL;
   
   // terminate
   TF_Status* status = TF_NewStatus();
   TF_CloseSession(junk->session, status);
   if (TF_GetCode(status) != TF_OK)
   {
      Error("ENIGMA: Tensorflow: %s\n", OTHER_ERROR, TF_Message(status));
   }
   TF_DeleteSession(junk->session, status);
   if (TF_GetCode(status) != TF_OK)
   {
      Error("ENIGMA: Tensorflow: %s\n", OTHER_ERROR, TF_Message(status));
   }
   TF_DeleteSessionOptions(junk->options);
   TF_DeleteGraph(junk->graph);
   TF_DeleteStatus(status);
   TF_DeleteBuffer(junk->run);

   EnigmaWeightTfParamCellFree(junk);
}
 
WFCB_p EnigmaWeightTfParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   double len_mult;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);
   char* d_model = ParseFilename(in);
   AcceptInpTok(in, Comma);
   long binary_weights = ParseInt(in);
   AcceptInpTok(in, Comma);
   long context_size = ParseInt(in);
   AcceptInpTok(in, Comma);
   len_mult = ParseFloat(in);
   AcceptInpTok(in, CloseBracket);

   return EnigmaWeightTfInit(
      prio_fun, 
      ocb,
      state,
      d_model,
      binary_weights,
      context_size,
      len_mult);
}

WFCB_p EnigmaWeightTfInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_dirname,
   long binary_weights,
   long context_size,
   double len_mult)
{
   EnigmaWeightTfParam_p data = EnigmaWeightTfParamAlloc();

   data->init_fun   = extweight_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   
   data->model_dirname = model_dirname;
   data->binary_weights = binary_weights;
   data->context_size = context_size;
   data->len_mult = len_mult;

   saved_local = data;
   
   return WFCBAlloc(
      EnigmaWeightTfCompute, 
      prio_fun,
      EnigmaWeightTfExit, 
      data);
}

double EnigmaWeightTfCompute(void* data, Clause_p clause)
{  
   EnigmaWeightTfParam_p local = data;
   local->init_fun(data);

   double weight;
   double length = ClauseWeight(clause,1,1,1,1,1,1,true);
   if (clause->tf_weight == 0.0)
   {
      weight = length;
   }
   else
   {
      if (local->binary_weights)
      {
         weight = (clause->tf_weight > 0.0) ? 1 : 10;
      }
      else
      {
         weight = 2.0 - (clause->tf_weight / (1 + fabs(clause->tf_weight)));
      }
   }

   weight += (local->len_mult * length);

#ifdef DEBUG_ETF
   fprintf(GlobalOut, "#TF#EVAL# %+.5f(%.1f)= ", weight, clause->tf_weight);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
#endif

   return weight;
}

void EnigmaComputeEvals(ClauseSet_p set, EnigmaWeightTfParam_p local)
{
   if (!local)
   {
      if (!saved_local) { return; }
      local = saved_local;
   }
   local->init_fun(local);
   EnigmaTensorsReset(local->tensors);

   for (Clause_p handle=set->anchor->succ; handle!=set->anchor; handle=handle->succ)
   {
      EnigmaTensorsUpdateClause(handle, local->tensors);
   }

#ifdef DEBUG_ETF
   debug_symbols(local);
   debug_terms(local);
   debug_edges(local);
#endif
   tensor_fill_input(local);
   tensor_fill_output(local);
   tensor_eval(local);
   
   float* logits = TF_TensorData(local->output_values[0]);
#ifdef DEBUG_ETF
   int n_q = local->fresh_c - (local->conj_fresh_c - local->context_cnt);
   assert(n_q == set->members + local->context_cnt);
   debug_vector_float("logits", logits, n_q, "Squeeze", 0);
   fprintf(GlobalOut, "#TF#QUERY# query = %d; eval = %ld; context = %ld\n", n_q, set->members, local->context_cnt);
#endif

   int idx = local->context_cnt;
   for (Clause_p handle=set->anchor->succ; handle!=set->anchor; handle=handle->succ)
   {
      handle->tf_weight = logits[idx++];
   }

   tensor_free(local);
   EnigmaTensorsReset(local->tensors);
}

void EnigmaContextAdd(Clause_p clause, EnigmaWeightTfParam_p local)
{
   if (!local)
   {
      if (!saved_local) { return; }
      local = saved_local;
   }
   if (local->context_cnt >= local->context_size)
   {
      return;
   }
   local->init_fun(local);

   EnigmaTensorsReset(local->tensors);
   local->conj_mode = true;
   EnigmaTensorsUpdateClause(clause, local->tensors);
   local->conj_mode = false;
   local->conj_maxvar = local->maxvar; // save maxvar to restore
   EnigmaTensorsReset(local->tensors);
#ifdef DEBUG_ETF
   fprintf(GlobalOut, "#TF# Context clause %ld added: ", local->context_cnt);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
#endif
   local->context_cnt++;
}


void EnigmaWeightTfExit(void* data)
{
   EnigmaWeightTfParam_p junk = data;
   
   EnigmaWeightTfParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

