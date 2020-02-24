/*-----------------------------------------------------------------------

File  : che_enigmaweighttf.c

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
// (and neither will tensor_fill_input)!
static EnigmaWeightTfParam_p saved_local = NULL;


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/


static long number_symbol(FunCode sym, EnigmaWeightTfParam_p data)
{
   NumTree_p node;

   node = NumTreeFind(&data->conj_syms, sym);
   if (!node)
   {
      if (!data->conj_mode) 
      {
         node = NumTreeFind(&data->syms, sym);
      }
      if (!node)
      {
         node = NumTreeCellAlloc();
         node->key = sym;
         node->val2.i_val = 0L;
         if (data->conj_mode)
         {
            node->val1.i_val = data->conj_fresh_s;
            data->conj_fresh_s++;
            NumTreeInsert(&data->conj_syms, node);
         }
         else
         {
            node->val1.i_val = data->fresh_s;
            data->fresh_s++;
            NumTreeInsert(&data->syms, node);
         }
      }
   }

   return node->val1.i_val;   
}

static long number_term(Term_p term, long b, EnigmaWeightTfParam_p data)
{
   NumTree_p node;

   if (!term)
   {
      return -1;
   }
   // encode:
   // 1. variables (id<0)
   // 2. positive terms (even)
   // 3. negated terms (odd)
   long id = 2*term->entry_no; 
   if (b == -1)
   {
      id += 1;
   }

   node = NumTreeFind(&data->conj_terms, id);
   if (!node)
   {
      if (!data->conj_mode)
      {
         node = NumTreeFind(&data->terms, id);
      }
      if (!node)
      {
         node = NumTreeCellAlloc();
         node->key = id;
         node->val2.p_val = term;
         if (data->conj_mode)
         {
            node->val1.i_val = data->conj_fresh_t;
            data->conj_fresh_t++;
            NumTreeInsert(&data->conj_terms, node);
         }
         else
         {
            node->val1.i_val = data->fresh_t;
            data->fresh_t++;
            NumTreeInsert(&data->terms, node);
         }
      }
   }

   return node->val1.i_val;
}

static Term_p fresh_term(Term_p term, EnigmaWeightTfParam_p data, DerefType deref)
{
   // NOTE: never call number_term from here as it updates maxvar
   term = TermDeref(term, &deref);

   Term_p fresh;

   if (TermIsVar(term))
   {
      fresh = VarBankVarAssertAlloc(data->tmp_bank->vars, 
         term->f_code - data->maxvar, term->type);      
   }
   else
   {
      fresh = TermTopCopyWithoutArgs(term);
      for(int i=0; i<term->arity; i++)
      {
         fresh->args[i] = fresh_term(term->args[i], data, deref);
      }
   }
   
   return fresh;
}

static void fresh_clause(Clause_p clause, EnigmaWeightTfParam_p data)
{
   for (Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      lit->lterm = fresh_term(lit->lterm, data, DEREF_ALWAYS);
      lit->rterm = fresh_term(lit->rterm, data, DEREF_ALWAYS);
   }
}

static Clause_p clause_fresh_copy(Clause_p clause, EnigmaWeightTfParam_p data)
{
   Clause_p clause0 = ClauseFlatCopy(clause);
   fresh_clause(clause0, data);
   Clause_p clause1 = ClauseCopy(clause0, data->tmp_bank);
   ClauseFree(clause0);
   return clause1;
}

static void edge_clause(long cid, long tid, EnigmaWeightTfParam_p data)
{
   PDArray_p edge = PDArrayAlloc(2, 2);
   PDArrayAssignInt(edge, 0, cid);
   PDArrayAssignInt(edge, 1, tid);
   if (data->conj_mode)
   {
      PStackPushP(data->conj_cedges, edge);
   }
   else
   {
      PStackPushP(data->cedges, edge);
   }
}

static bool edge_term_check(long i, long j, long k, long l, long b,
   PStack_p edges)
{
   for (int idx=0; i<edges->current; i++)
   { 
      PDArray_p edge = PStackElementP(edges, idx);
      if (
         (PDArrayElementInt(edge, 0) == i) &&
         (PDArrayElementInt(edge, 1) == j) &&
         (PDArrayElementInt(edge, 2) == k) &&
         (PDArrayElementInt(edge, 3) == l) &&
         (PDArrayElementInt(edge, 4) == b))
      {
         return true;
      }
   }
   return false;
}

static void edge_term_get(PDArray_p edge, long* i, long* j, long* k, long *l, long *b)
{
   *i = PDArrayElementInt(edge, 0);
   *j = PDArrayElementInt(edge, 1);
   *k = PDArrayElementInt(edge, 2);
   *l = PDArrayElementInt(edge, 3);
   *b = PDArrayElementInt(edge, 4);
}

static void edge_term(long i, long j, long k, long l, long b,
      EnigmaWeightTfParam_p data)
{
   if (edge_term_check(i, j, k, l, b, data->conj_tedges) ||
       edge_term_check(i, j, k, l, b, data->tedges))
   {
      return;
   }

   PDArray_p edge = PDArrayAlloc(5, 5);
   PDArrayAssignInt(edge, 0, i);
   PDArrayAssignInt(edge, 1, j);
   PDArrayAssignInt(edge, 2, k);
   PDArrayAssignInt(edge, 3, l);
   PDArrayAssignInt(edge, 4, b);
   if (data->conj_mode)
   {
      PStackPushP(data->conj_tedges, edge);
   }
   else
   {
      PStackPushP(data->tedges, edge);
   }
}

static long names_update_term(Term_p term, EnigmaWeightTfParam_p data, long b)
{
   long tid = number_term(term, b, data);
   if (TermIsVar(term))
   {
      data->maxvar = MAX(data->maxvar, -term->f_code);
      return tid;
   }

   long sid = number_symbol(term->f_code, data);
   long tid0 = 0;
   long tid1 = 0;
   for (int i=0; i<term->arity; i++)
   {
      tid0 = tid1;
      tid1 = names_update_term(term->args[i], data, 1);
      if ((tid0 != 0) && (tid1 != 0))
      {
         edge_term(tid, tid0, tid1, sid, b, data);
      }
   }
   if (term->arity == 0)
   {
      edge_term(tid, -1, -1, sid, b, data);
   }
   if (term->arity == 1)
   {
      edge_term(tid, tid1, -1, sid, b, data);
   }

   return tid;
}

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

static void free_edges(PStack_p stack)
{
   while (!PStackEmpty(stack))
   {  
      PDArray_p edge = PStackPopP(stack);
      PDArrayFree(edge);
   }
}

static void tensor_fill_ini_nodes(int32_t* vals, NumTree_p syms, 
   EnigmaWeightTfParam_p data)
{
   NumTree_p node;
   PStack_p stack;

   stack = NumTreeTraverseInit(syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      if (node->key < 0) 
      {
         vals[node->val1.i_val] = 2; // variable
      }
      else 
      {
         Term_p term = node->val2.p_val;
         if (SigIsPredicate(data->proofstate->signature, term->f_code))
         {
            vals[node->val1.i_val] = 1; // literal
         }
         else
         {
            vals[node->val1.i_val] = 0; // otherwise
         }
      }
   }
   NumTreeTraverseExit(stack);
}

static void tensor_fill_ini_symbols(int32_t* vals, NumTree_p terms, 
   EnigmaWeightTfParam_p data)
{
   NumTree_p node;
   PStack_p stack;

   stack = NumTreeTraverseInit(terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      if (SigIsPredicate(data->proofstate->signature,  node->key)) 
      {
         vals[node->val1.i_val] = 1; // predicate
      }
      else 
      {
         vals[node->val1.i_val] = 0; // function
      }
   }
   NumTreeTraverseExit(stack);
}

static void tensor_fill_ini_clauses(int32_t* vals, EnigmaWeightTfParam_p data)
{
   for (int i=0; i<data->fresh_c; i++)
   {
      vals[i] = (i < (data->conj_fresh_c - data->context_cnt)) ? 0 : 1;
   }
}

static int tensor_fill_encode_list(PStack_p lists, int32_t* vals, int32_t* lens)
{
   int idx = 0;
   for (int i=0; i<lists->current; i++)
   {
      PStack_p list = PStackElementP(lists, i);
      lens[i] = list->current;
      while (!PStackEmpty(list))
      {
         vals[idx++] = PStackPopInt(list);
      }
      PStackFree(list);
   }
   PStackFree(lists);
   return idx;
}

static int tensor_fill_encode_dict_symbol(PStack_p lists, 
   int32_t* nodes, float* sgn, int32_t* lens)
{
   long i, j, k, l, b;
   int idx_nodes = 0;
   int idx_sgn = 0;
   for (int idx=0; idx<lists->current; idx++)
   {
      PStack_p list = PStackElementP(lists, idx);
      lens[idx] = list->current;
      while (!PStackEmpty(list))
      {
         PDArray_p edge = PStackPopP(list);
         edge_term_get(edge, &i, &j, &k, &l, &b);
         nodes[idx_nodes++] = i;
         nodes[idx_nodes++] = j;
         nodes[idx_nodes++] = k;
         sgn[idx_sgn++] = b;
      }
      PStackFree(list);
   }
   PStackFree(lists);
   return idx_sgn;
}

static int tensor_fill_encode_dict_node(PStack_p lists, 
   int32_t* symbols, int32_t* nodes, float* sgn, int32_t* lens, int node_mode)
{
   long i, j, k, l, b;
   int idx_nodes = 0;
   int idx_sgn = 0;
   int idx_symbols = 0;
   for (int idx=0; idx<lists->current; idx++)
   {
      PStack_p list = PStackElementP(lists, idx);
      lens[idx] = list->current;
      while (!PStackEmpty(list))
      {
         PDArray_p edge = PStackPopP(list);
         edge_term_get(edge, &i, &j, &k, &l, &b);
         switch (node_mode)
         {
         case 1:
            nodes[idx_nodes++] = j;
            nodes[idx_nodes++] = k;
            break;
         case 2:
            nodes[idx_nodes++] = i;
            nodes[idx_nodes++] = k;
            break;
         case 3:
            nodes[idx_nodes++] = i;
            nodes[idx_nodes++] = j;
            break;
         default:
            Error("TensorFlow: Unknown encoding node_mode!", USAGE_ERROR);
         }
         symbols[idx_symbols++] = l;
         sgn[idx_sgn++] = b;
      }
      PStackFree(list);
   }
   PStackFree(lists);
   return idx_symbols;
}

static void tensor_fill_clause_inputs(
   int32_t* clause_inputs_data, 
   int32_t* clause_inputs_lens, 
   int32_t* node_c_inputs_data, 
   int32_t* node_c_inputs_lens, 
   EnigmaWeightTfParam_p data)
{
   int i;

   PStack_p clists = PStackAlloc();
   PStack_p tlists = PStackAlloc();
   for (i=0; i<data->fresh_c; i++)
   {
      PStackPushP(clists, PStackAlloc());
   }
   for (i=0; i<data->fresh_t; i++)
   {
      PStackPushP(tlists, PStackAlloc());
   }

   for (i=0; i<data->conj_cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->conj_cedges, i);
      long ci = PDArrayElementInt(edge, 0);
      long tj = PDArrayElementInt(edge, 1);
      PStackPushInt(PStackElementP(clists, ci), tj);
      PStackPushInt(PStackElementP(tlists, tj), ci);
   }
   for (i=0; i<data->cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->cedges, i);
      long ci = PDArrayElementInt(edge, 0);
      long tj = PDArrayElementInt(edge, 1);
      PStackPushInt(PStackElementP(clists, ci), tj);
      PStackPushInt(PStackElementP(tlists, tj), ci);
   }

   // this also frees all the stacks
   tensor_fill_encode_list(clists, clause_inputs_data, clause_inputs_lens);
   tensor_fill_encode_list(tlists, node_c_inputs_data, node_c_inputs_lens);
}
   
static void tensor_fill_term_inputs(
   int32_t* symbol_inputs_nodes, 
   float* symbol_inputs_sgn, 
   int32_t* symbol_inputs_lens, 
   int32_t* node_inputs_1_symbols,
   int32_t* node_inputs_1_nodes,
   float* node_inputs_1_sgn,
   int32_t* node_inputs_1_lens,
   int32_t* node_inputs_2_symbols,
   int32_t* node_inputs_2_nodes,
   float* node_inputs_2_sgn,
   int32_t* node_inputs_2_lens,
   int32_t* node_inputs_3_symbols,
   int32_t* node_inputs_3_nodes,
   float* node_inputs_3_sgn,
   int32_t* node_inputs_3_lens,
   EnigmaWeightTfParam_p data)
{
   int idx;

   PStack_p slists = PStackAlloc();
   for (idx=0; idx<data->fresh_s; idx++)
   {
      PStackPushP(slists, PStackAlloc());
   }
   PStack_p n1lists = PStackAlloc();
   PStack_p n2lists = PStackAlloc();
   PStack_p n3lists = PStackAlloc();
   for (idx=0; idx<data->fresh_t; idx++)
   {
      PStackPushP(n1lists, PStackAlloc());
      PStackPushP(n2lists, PStackAlloc());
      PStackPushP(n3lists, PStackAlloc());
   }

   long i, j, k, l, b;
   for (idx=0; idx<data->conj_tedges->current; idx++)
   { 
      PDArray_p edge = PStackElementP(data->conj_tedges, idx);
      edge_term_get(edge, &i, &j, &k, &l, &b);
      PStackPushP(PStackElementP(n1lists, i), edge);
      if (j != -1)
      {
         PStackPushP(PStackElementP(n2lists, j), edge);
      }
      if (k != -1)
      {
         PStackPushP(PStackElementP(n3lists, k), edge);
      }
      PStackPushP(PStackElementP(slists, l), edge);
   }
   for (idx=0; idx<data->tedges->current; idx++)
   { 
      PDArray_p edge = PStackElementP(data->tedges, idx);
      edge_term_get(edge, &i, &j, &k, &l, &b);
      PStackPushP(PStackElementP(n1lists, i), edge);
      if (j != -1)
      {
         PStackPushP(PStackElementP(n2lists, j), edge);
      }
      if (k != -1)
      {
         PStackPushP(PStackElementP(n3lists, k), edge);
      }
      PStackPushP(PStackElementP(slists, l), edge);
   }

   data->n_is = tensor_fill_encode_dict_symbol(slists, symbol_inputs_nodes, 
      symbol_inputs_sgn, symbol_inputs_lens);
   data->n_i1 = tensor_fill_encode_dict_node(n1lists, node_inputs_1_symbols, 
      node_inputs_1_nodes, node_inputs_1_sgn, node_inputs_1_lens, 1);
   data->n_i2 = tensor_fill_encode_dict_node(n2lists, node_inputs_2_symbols, 
      node_inputs_2_nodes, node_inputs_2_sgn, node_inputs_2_lens, 2);
   data->n_i3 = tensor_fill_encode_dict_node(n3lists, node_inputs_3_symbols, 
      node_inputs_3_nodes, node_inputs_3_sgn, node_inputs_3_lens, 3);
}

static void tensor_fill_query(
   int32_t* labels, 
   int32_t* prob_segments_lens, 
   int32_t* prob_segments_data, 
   EnigmaWeightTfParam_p data)
{
   int n_q = data->fresh_c - (data->conj_fresh_c - data->context_cnt); 
   prob_segments_lens[0] = 1 + n_q;
   prob_segments_data[0] = data->conj_fresh_c - data->context_cnt;
   for (int i=0; i<n_q; i++)
   {
      prob_segments_data[1+i] = 1;
   }
}

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
   static int32_t ini_nodes[ETF_TENSOR_SIZE];
   static int32_t ini_symbols[ETF_TENSOR_SIZE];
   static int32_t ini_clauses[ETF_TENSOR_SIZE];
   static int32_t clause_inputs_data[ETF_TENSOR_SIZE];
   static int32_t clause_inputs_lens[ETF_TENSOR_SIZE];
   static int32_t node_c_inputs_data[ETF_TENSOR_SIZE];
   static int32_t node_c_inputs_lens[ETF_TENSOR_SIZE];
   static int32_t symbol_inputs_nodes[3*ETF_TENSOR_SIZE]; 
   static int32_t symbol_inputs_lens[ETF_TENSOR_SIZE]; 
   static int32_t node_inputs_1_symbols[ETF_TENSOR_SIZE];
   static int32_t node_inputs_1_nodes[2*ETF_TENSOR_SIZE];
   static int32_t node_inputs_1_lens[ETF_TENSOR_SIZE];
   static int32_t node_inputs_2_symbols[ETF_TENSOR_SIZE];
   static int32_t node_inputs_2_nodes[2*ETF_TENSOR_SIZE];
   static int32_t node_inputs_2_lens[ETF_TENSOR_SIZE];
   static int32_t node_inputs_3_symbols[ETF_TENSOR_SIZE];
   static int32_t node_inputs_3_nodes[2*ETF_TENSOR_SIZE];
   static int32_t node_inputs_3_lens[ETF_TENSOR_SIZE];
   static float symbol_inputs_sgn[ETF_TENSOR_SIZE]; 
   static float node_inputs_1_sgn[ETF_TENSOR_SIZE];
   static float node_inputs_2_sgn[ETF_TENSOR_SIZE];
   static float node_inputs_3_sgn[ETF_TENSOR_SIZE];
   static int32_t prob_segments_lens[ETF_TENSOR_SIZE];
   static int32_t prob_segments_data[ETF_TENSOR_SIZE];
   static int32_t labels[ETF_TENSOR_SIZE];
   
   int n_ce = data->cedges->current + data->conj_cedges->current;
   int n_te = data->tedges->current + data->conj_tedges->current;
   int n_s = data->fresh_s;
   int n_c = data->fresh_c;
   int n_t = data->fresh_t;
   int n_q = n_c - (data->conj_fresh_c - data->context_cnt); // query clauses (evaluated and context clauses)
   if (n_te > ETF_TENSOR_SIZE)
   {
      Error("Enigma-TF: Too many term edges (required: %d; max: %d).\nRecompile with increased ETF_TENSOR_SIZE.", OTHER_ERROR, n_te, ETF_TENSOR_SIZE);
   }

   tensor_fill_ini_nodes(ini_nodes, data->conj_terms, data);
   tensor_fill_ini_nodes(ini_nodes, data->terms, data);
   tensor_fill_ini_symbols(ini_symbols, data->conj_syms, data);
   tensor_fill_ini_symbols(ini_symbols, data->syms, data);
   tensor_fill_ini_clauses(ini_clauses, data);

   tensor_fill_clause_inputs(
      clause_inputs_data, 
      clause_inputs_lens, 
      node_c_inputs_data, 
      node_c_inputs_lens, 
      data
   );
   
   data->n_is = 0;
   data->n_i1 = 0;
   data->n_i2 = 0;
   data->n_i3 = 0;
   tensor_fill_term_inputs(
      symbol_inputs_nodes, 
      symbol_inputs_sgn, 
      symbol_inputs_lens, 
      node_inputs_1_symbols,
      node_inputs_1_nodes,
      node_inputs_1_sgn,
      node_inputs_1_lens,
      node_inputs_2_symbols,
      node_inputs_2_nodes,
      node_inputs_2_sgn,
      node_inputs_2_lens,
      node_inputs_3_symbols,
      node_inputs_3_nodes,
      node_inputs_3_sgn,
      node_inputs_3_lens,
      data
   );
   int n_is = data->n_is;
   int n_i1 = data->n_i1;
   int n_i2 = data->n_i2;
   int n_i3 = data->n_i3;
   
   tensor_fill_query(labels, prob_segments_lens, prob_segments_data, data);
   
   // set vectors to data
   set_input_vector_int32(0, n_t, "ini_nodes", ini_nodes, "GraphPlaceholder/ini_nodes", data);
   set_input_vector_int32(1, n_s, "ini_symbols", ini_symbols, "GraphPlaceholder/ini_symbols", data);
   set_input_vector_int32(2, n_c, "ini_clauses", ini_clauses, "GraphPlaceholder/ini_clauses", data);
   set_input_vector_int32(3, n_t, "node_inputs_1_lens", node_inputs_1_lens, "GraphPlaceholder/GraphHyperEdgesA/segment_lens", data);
   set_input_vector_int32(4, n_i1, "node_inputs_1_symbols", node_inputs_1_symbols, "GraphPlaceholder/GraphHyperEdgesA/symbols", data);
   set_input_vector_float(5, n_i1, "node_inputs_1_sgn", node_inputs_1_sgn, "GraphPlaceholder/GraphHyperEdgesA/sgn", data);
   set_input_vector_int32(6, n_t, "node_inputs_2_lens", node_inputs_2_lens, "GraphPlaceholder/GraphHyperEdgesA_1/segment_lens", data);
   set_input_vector_int32(7, n_i2, "node_inputs_2_symbols", node_inputs_2_symbols, "GraphPlaceholder/GraphHyperEdgesA_1/symbols", data);
   set_input_vector_float(8, n_i2, "node_inputs_2_sgn", node_inputs_2_sgn, "GraphPlaceholder/GraphHyperEdgesA_1/sgn", data);
   set_input_vector_int32(9, n_t, "node_inputs_3_lens", node_inputs_3_lens, "GraphPlaceholder/GraphHyperEdgesA_2/segment_lens", data);
   set_input_vector_int32(10, n_i3, "node_inputs_3_symbols", node_inputs_3_symbols, "GraphPlaceholder/GraphHyperEdgesA_2/symbols", data);
   set_input_vector_float(11, n_i3, "node_inputs_3_sgn", node_inputs_3_sgn, "GraphPlaceholder/GraphHyperEdgesA_2/sgn", data);
   set_input_vector_int32(12, n_s, "symbol_inputs_lens", symbol_inputs_lens, "GraphPlaceholder/GraphHyperEdgesB/segment_lens", data);
   set_input_vector_float(13, n_is, "symbol_inputs_sgn", symbol_inputs_sgn, "GraphPlaceholder/GraphHyperEdgesB/sgn", data);
   set_input_vector_int32(14, n_t, "node_c_inputs_lens", node_c_inputs_lens, "GraphPlaceholder/GraphEdges/segment_lens", data);
   set_input_vector_int32(15, n_ce, "node_c_inputs_data", node_c_inputs_data, "GraphPlaceholder/GraphEdges/data", data);
   set_input_vector_int32(16, n_c, "clause_inputs_lens", clause_inputs_lens, "GraphPlaceholder/GraphEdges_1/segment_lens", data);
   set_input_vector_int32(17, n_ce, "clause_inputs_data", clause_inputs_data, "GraphPlaceholder/GraphEdges_1/data", data);
   set_input_vector_int32(18, 1, "prob_segments_lens", prob_segments_lens, "segment_lens", data);
   set_input_vector_int32(19, 1+n_q, "prob_segments_data", prob_segments_data, "segment_data", data);
   set_input_vector_int32(20, n_q, "labels", labels, "Placeholder", data);
   set_input_matrix(21, n_i1, 2, "node_inputs_1_nodes", node_inputs_1_nodes, "GraphPlaceholder/GraphHyperEdgesA/nodes", data);
   set_input_matrix(22, n_i2, 2, "node_inputs_2_nodes", node_inputs_2_nodes, "GraphPlaceholder/GraphHyperEdgesA_1/nodes", data);
   set_input_matrix(23, n_i3, 2, "node_inputs_3_nodes", node_inputs_3_nodes, "GraphPlaceholder/GraphHyperEdgesA_2/nodes", data);
   set_input_matrix(24, n_is, 3, "symbol_inputs_nodes", symbol_inputs_nodes, "GraphPlaceholder/GraphHyperEdgesB/nodes", data);
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
   data->tmp_bank = TBAlloc(data->proofstate->signature);
   data->conj_mode = true;
   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture) 
      {
         EnigmaTensorsUpdateClause(clause, data);
      }
   }
   data->conj_mode = false;
   data->conj_maxvar = data->maxvar; // save maxvar to restore
   EnigmaTensorsReset(data);

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

void EnigmaTensorsReset(EnigmaWeightTfParam_p data)
{
   if (data->terms)
   {
      NumTreeFree(data->terms);
      data->terms = NULL;
   }
   if (data->syms)
   {
      NumTreeFree(data->syms);
      data->syms = NULL;
   }

   free_edges(data->cedges);
   free_edges(data->tedges);

   data->fresh_t = data->conj_fresh_t;
   data->fresh_s = data->conj_fresh_s;
   data->fresh_c = data->conj_fresh_c;
   data->maxvar = data->conj_maxvar;
}


void EnigmaTensorsUpdateClause(Clause_p clause, EnigmaWeightTfParam_p data)
{
   Clause_p clause0 = clause_fresh_copy(clause, data); 

   long tid = -1;
   long cid = (data->conj_mode) ? data->conj_fresh_c : data->fresh_c;
   for (Eqn_p lit = clause0->literals; lit; lit = lit->next)
   {
      bool pos = EqnIsPositive(lit);
      if (lit->rterm->f_code == SIG_TRUE_CODE)
      {
         tid = names_update_term(lit->lterm, data, pos ? 1 : -1);
      }
      else
      {
         Term_p term = TermTopAlloc(data->proofstate->signature->eqn_code, 2);
         term->args[0] = lit->lterm;
         term->args[1] = lit->rterm;
         Term_p term1 = TBInsert(data->tmp_bank, term, DEREF_ALWAYS);
         tid = names_update_term(term1, data, pos ? 1 : -1);
         TermTopFree(term); 
      }
      edge_clause(cid, tid, data);
   }
   if (tid == -1)
   {
      return;
   }
   if (data->conj_mode)
   {
      data->conj_fresh_c++;
   }
   else
   {
      data->fresh_c++;
   }

#ifdef DEBUG_ETF
   fprintf(GlobalOut, "#TF# Clause c%ld: ", cid);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
#endif

   ClauseFree(clause0);
}


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
   res->tmp_bank = NULL;

   return res;
}

void EnigmaWeightTfParamFree(EnigmaWeightTfParam_p junk)
{
   free(junk->model_dirname);
   
   if (!junk->inited)
   {
      return;
   }

   free_edges(junk->tedges);
   free_edges(junk->cedges);
   free_edges(junk->conj_tedges);
   free_edges(junk->conj_cedges);
   PStackFree(junk->tedges);
   PStackFree(junk->cedges);
   PStackFree(junk->conj_tedges);
   PStackFree(junk->conj_cedges);

   if (junk->tmp_bank)
   {
      TBFree(junk->tmp_bank);
      junk->tmp_bank = NULL;
   }
   
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
   EnigmaTensorsReset(local);

   for (Clause_p handle=set->anchor->succ; handle!=set->anchor; handle=handle->succ)
   {
      EnigmaTensorsUpdateClause(handle, local);
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
   EnigmaTensorsReset(local);
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

   EnigmaTensorsReset(local);
   local->conj_mode = true;
   EnigmaTensorsUpdateClause(clause, local);
   local->conj_mode = false;
   local->conj_maxvar = local->maxvar; // save maxvar to restore
   EnigmaTensorsReset(local);
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

