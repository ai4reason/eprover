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
/*
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
*/
#endif

static void extweight_init(EnigmaWeightTfParam_p data)
{
   Clause_p clause;
   Clause_p anchor;

   if (data->inited)
   {
      return;
   }

   //fprintf(GlobalOut, "# ENIGMA: TensorFlow C library version %s\n", TF_Version());
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

   char* etf_ip = "127.0.0.1";
   uint16_t etf_port = 8888;

   data->etf_socket = socket(AF_INET , SOCK_STREAM , 0);
	if (data->etf_socket == -1)
	{
      perror(NULL);
		Error("ENIGMA: Can not create socket to connect to TF server!", OTHER_ERROR);
	}

	data->etf_server.sin_family = AF_INET;
   data->etf_server.sin_addr.s_addr = inet_addr(etf_ip);
	data->etf_server.sin_port = htons(etf_port);

   if (connect(data->etf_socket, (struct sockaddr*)&(data->etf_server), sizeof(data->etf_server)) < 0)
   {
      perror(NULL);
      Error("ENIGMA: Error connecting to the TF server '%s:%d'.", OTHER_ERROR, etf_ip, etf_port);
   }

   fprintf(GlobalOut, "ENIGMA: Connected to the TF server '%s:%d'.\n", etf_ip, etf_port);

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

   // TODO SERVER: call server evaluation here
   
   //int idx = local->context_cnt;
   for (Clause_p handle=set->anchor->succ; handle!=set->anchor; handle=handle->succ)
   {
      handle->tf_weight = 1.23; // logits[idx++];
   }

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

