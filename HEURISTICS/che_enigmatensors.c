/*-----------------------------------------------------------------------

File  : che_enigmatensors.c

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

#include <sys/socket.h>
#include "che_enigmatensors.h"

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static long number_symbol(FunCode sym, EnigmaTensors_p data)
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

static void free_edges(PStack_p stack)
{
   while (!PStackEmpty(stack))
   {  
      PDArray_p edge = PStackPopP(stack);
      PDArrayFree(edge);
   }
}

static Term_p fresh_term(Term_p term, EnigmaTensors_p data, DerefType deref)
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

static void fresh_clause(Clause_p clause, EnigmaTensors_p data)
{
   for (Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      lit->lterm = fresh_term(lit->lterm, data, DEREF_ALWAYS);
      lit->rterm = fresh_term(lit->rterm, data, DEREF_ALWAYS);
   }
}

static void free_term(Term_p term)
{
   if (!TermIsVar(term))
   {
      for(int i=0; i<term->arity; i++)
      {
         free_term(term->args[i]);
      }
      TermTopFree(term);
   }
}

static void free_clause(Clause_p clause)
{
   for (Eqn_p lit = clause->literals; lit; )
   {
      free_term(lit->lterm);
      free_term(lit->rterm);
      Eqn_p lit2 = lit->next;
      EqnCellFree(lit);
      lit = lit2;
   }
   ClauseCellFree(clause);
}

static Clause_p clause_fresh_copy(Clause_p clause, EnigmaTensors_p data)
{
   Clause_p clause0 = ClauseFlatCopy(clause);
   fresh_clause(clause0, data);
   Clause_p clause1 = ClauseCopy(clause0, data->tmp_bank);
   //ClauseFree(clause0);
   free_clause(clause0);
   return clause1;
}

static bool edge_term_check(long i, long j, long k, long l, long b,
   PStack_p edges)
{
   for (int idx=0; idx<edges->current; idx++)
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

static void edge_term(long i, long j, long k, long l, long b,
      EnigmaTensors_p data)
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

static long number_term(Term_p term, long b, EnigmaTensors_p data)
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

static void edge_clause(long cid, long tid, EnigmaTensors_p data)
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

static long names_update_term(Term_p term, EnigmaTensors_p data, long b)
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

static void tensor_fill_ini_nodes(int32_t* vals, NumTree_p syms, 
   EnigmaTensors_p data)
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
         if (SigIsPredicate(data->tmp_bank->sig, term->f_code))
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
   EnigmaTensors_p data)
{
   NumTree_p node;
   PStack_p stack;

   stack = NumTreeTraverseInit(terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      if (SigIsPredicate(data->tmp_bank->sig, node->key)) 
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

static void tensor_fill_ini_clauses(int32_t* vals, EnigmaTensors_p data)
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

static void edge_term_get(PDArray_p edge, long* i, long* j, long* k, long *l, long *b)
{
   *i = PDArrayElementInt(edge, 0);
   *j = PDArrayElementInt(edge, 1);
   *k = PDArrayElementInt(edge, 2);
   *l = PDArrayElementInt(edge, 3);
   *b = PDArrayElementInt(edge, 4);
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
   EnigmaTensors_p data)
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
   EnigmaTensors_p data)
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
   EnigmaTensors_p data)
{
   int n_q = data->fresh_c - (data->conj_fresh_c - data->context_cnt); 
   prob_segments_lens[0] = 1 + n_q;
   prob_segments_data[0] = data->conj_fresh_c - data->context_cnt;
   for (int i=0; i<n_q; i++)
   {
      prob_segments_data[1+i] = 1;
   }
}
/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaSocket_p EnigmaSocketAlloc(void)
{
   EnigmaSocket_p sock = EnigmaSocketCellAlloc();
   sock->cur = 0;
   sock->fd = 0;
   return sock;
}

void EnigmaSocketFree(EnigmaSocket_p junk)
{
   EnigmaSocketCellFree(junk);
}

EnigmaTensors_p EnigmaTensorsAlloc(void)
{
   EnigmaTensors_p res = EnigmaTensorsCellAlloc();

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

void EnigmaTensorsFree(EnigmaTensors_p junk)
{
   free_edges(junk->tedges);
   free_edges(junk->cedges);
   free_edges(junk->conj_tedges);
   free_edges(junk->conj_cedges);
   PStackFree(junk->tedges);
   PStackFree(junk->cedges);
   PStackFree(junk->conj_tedges);
   PStackFree(junk->conj_cedges);

   if (junk->terms)
   {
      NumTreeFree(junk->terms);
      junk->terms = NULL;
   }
   if (junk->syms)
   {
      NumTreeFree(junk->syms);
      junk->syms = NULL;
   }
   if (junk->conj_terms)
   {
      NumTreeFree(junk->conj_terms);
      junk->conj_terms = NULL;
   }
   if (junk->conj_syms)
   {
      NumTreeFree(junk->conj_syms);
      junk->conj_syms = NULL;
   }

   if (junk->tmp_bank)
   {
      TBFree(junk->tmp_bank);
      junk->tmp_bank = NULL;
   }
   
   EnigmaTensorsCellFree(junk);
}
 
void EnigmaTensorsReset(EnigmaTensors_p data)
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

void EnigmaTensorsUpdateClause(Clause_p clause, EnigmaTensors_p data)
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
         Term_p term = TermTopAlloc(data->tmp_bank->sig->eqn_code, 2);
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

void EnigmaTensorsFill(EnigmaTensors_p data)
{
   int n_te = data->tedges->current + data->conj_tedges->current;
   if (n_te > ETF_TENSOR_SIZE)
   {
      Error("Enigma-TF: Too many term edges (required: %d; max: %d).\nRecompile with increased ETF_TENSOR_SIZE.", OTHER_ERROR, n_te, ETF_TENSOR_SIZE);
   }

   tensor_fill_ini_nodes(data->ini_nodes, data->conj_terms, data);
   tensor_fill_ini_nodes(data->ini_nodes, data->terms, data);
   tensor_fill_ini_symbols(data->ini_symbols, data->conj_syms, data);
   tensor_fill_ini_symbols(data->ini_symbols, data->syms, data);
   tensor_fill_ini_clauses(data->ini_clauses, data);

   tensor_fill_clause_inputs(
      data->clause_inputs_data, 
      data->clause_inputs_lens, 
      data->node_c_inputs_data, 
      data->node_c_inputs_lens, 
      data
   );
   
   data->n_is = 0;
   data->n_i1 = 0;
   data->n_i2 = 0;
   data->n_i3 = 0;
   tensor_fill_term_inputs(
      data->symbol_inputs_nodes, 
      data->symbol_inputs_sgn, 
      data->symbol_inputs_lens, 
      data->node_inputs_1_symbols,
      data->node_inputs_1_nodes,
      data->node_inputs_1_sgn,
      data->node_inputs_1_lens,
      data->node_inputs_2_symbols,
      data->node_inputs_2_nodes,
      data->node_inputs_2_sgn,
      data->node_inputs_2_lens,
      data->node_inputs_3_symbols,
      data->node_inputs_3_nodes,
      data->node_inputs_3_sgn,
      data->node_inputs_3_lens,
      data
   );
   
   tensor_fill_query(data->labels, data->prob_segments_lens, 
      data->prob_segments_data, data);
}

void socket_flush(EnigmaSocket_p sock)
{
   int ret =  send(sock->fd, sock->buf, sock->cur, 0);
   if (ret < 0)
   {
      Error("Sending data to Tensorflow server via a socket failed (%d).", OTHER_ERROR, ret);
   }
   sock->cur = 0;
}

void socket_finish(EnigmaSocket_p sock)
{
   sock->buf[sock->cur] = '\0';
   sock->cur++;
   socket_flush(sock);
}

void socket_str(EnigmaSocket_p sock, char* str)
{
   // NOTE: make sure the string is not longer than the buffer size!
   int len = strlen(str);
   if (sock->cur + len >= SOCKET_BUF_SIZE)
   {
      socket_flush(sock);
   }
   strncpy(&sock->buf[sock->cur], str, len+1);
   sock->cur += len;
}

void socket_vector_int32(EnigmaSocket_p sock, int size, char* id, int32_t* values)
{
   char str[128];
   socket_str(sock, "\"");
   socket_str(sock, id);
   socket_str(sock, "\":[");
   for (int i=0; i<size; i++)
   {
      sprintf(str, "%d%s", values[i], (i<size-1) ? "," : "");
      socket_str(sock, str);
   }
   socket_str(sock, "]");
}

void socket_vector_float(EnigmaSocket_p sock, int size, char* id, float* values)
{
   char str[128];
   socket_str(sock, "\"");
   socket_str(sock, id);
   socket_str(sock, "\":[");
   for (int i=0; i<size; i++)
   {
      sprintf(str, "%.01f%s", values[i], (i<size-1) ? "," : "");
      socket_str(sock, str);
   }
   socket_str(sock, "]");
}

void socket_matrix(EnigmaSocket_p sock, int dimx, int dimy, char* id, int32_t* values)
{
   socket_vector_int32(sock, dimx*dimy, id, values);
}

void dump_vector_int32(FILE* out, int size, char* id, int32_t* values)
{
   fprintf(out, "   \"%s\": [", id);
   for (int i=0; i<size; i++)
   {
      fprintf(out, "%d%s", values[i], (i<size-1) ? ", " : "");
   }
   fprintf(out, "],\n");
}

void dump_vector_float(FILE* out, int size, char* id, float* values)
{
   fprintf(out, "   \"%s\": [", id);
   for (int i=0; i<size; i++)
   {
      fprintf(out, "%.01f%s", values[i], (i<size-1) ? ", " : "");
   }
   fprintf(out, "],\n");
}

void dump_matrix(FILE* out, int dimx, int dimy, char* id, int32_t* values)
{
   fprintf(out, "   \"%s\": [", id);
   int size = dimx*dimy;
   for (int i=0; i<size; i++)
   {
      fprintf(out, "%d%s", values[i], (i<size-1) ? ", " : "");
   }
   fprintf(out, "],\n");
}

void EnigmaTensorsDump(FILE* out, EnigmaTensors_p tensors)
{
   int n_ce = tensors->cedges->current + tensors->conj_cedges->current;
   //int n_te = tensors->tedges->current + tensors->conj_tedges->current;
   int n_s = tensors->fresh_s;
   int n_c = tensors->fresh_c;
   int n_t = tensors->fresh_t;
   int n_q = n_c - (tensors->conj_fresh_c - tensors->context_cnt); // query clauses (evaluated and context clauses)
   int n_is = tensors->n_is;
   int n_i1 = tensors->n_i1;
   int n_i2 = tensors->n_i2;
   int n_i3 = tensors->n_i3;

   dump_vector_int32(out, n_t, "ini_nodes", tensors->ini_nodes);
   dump_vector_int32(out, n_s, "ini_symbols", tensors->ini_symbols);
   dump_vector_int32(out, n_c, "ini_clauses", tensors->ini_clauses);
   dump_vector_int32(out, n_t, "node_inputs_1/lens", tensors->node_inputs_1_lens);
   dump_vector_int32(out, n_i1, "node_inputs_1/symbols", tensors->node_inputs_1_symbols);
   dump_vector_float(out, n_i1, "node_inputs_1/sgn", tensors->node_inputs_1_sgn);
   dump_vector_int32(out, n_t, "node_inputs_2/lens", tensors->node_inputs_2_lens);
   dump_vector_int32(out, n_i2, "node_inputs_2/symbols", tensors->node_inputs_2_symbols);
   dump_vector_float(out, n_i2, "node_inputs_2/sgn", tensors->node_inputs_2_sgn);
   dump_vector_int32(out, n_t, "node_inputs_3/lens", tensors->node_inputs_3_lens);
   dump_vector_int32(out, n_i3, "node_inputs_3/symbols", tensors->node_inputs_3_symbols);
   dump_vector_float(out, n_i3, "node_inputs_3/sgn", tensors->node_inputs_3_sgn);
   dump_vector_int32(out, n_s, "symbol_inputs/lens", tensors->symbol_inputs_lens);
   dump_vector_float(out, n_is, "symbol_inputs/sgn", tensors->symbol_inputs_sgn);
   dump_vector_int32(out, n_t, "node_c_inputs/lens", tensors->node_c_inputs_lens);
   dump_vector_int32(out, n_ce, "node_c_inputs/data", tensors->node_c_inputs_data);
   dump_vector_int32(out, n_c, "clause_inputs/lens", tensors->clause_inputs_lens);
   dump_vector_int32(out, n_ce, "clause_inputs/data", tensors->clause_inputs_data);
   dump_vector_int32(out, 1, "prob_segments/lens", tensors->prob_segments_lens);
   dump_vector_int32(out, 1+n_q, "prob_segments/data", tensors->prob_segments_data);
   //dump_vector_int32(out, n_q, "labels", tensors->labels);
   dump_matrix(out, n_i1, 2, "node_inputs_1/nodes", tensors->node_inputs_1_nodes);
   dump_matrix(out, n_i2, 2, "node_inputs_2/nodes", tensors->node_inputs_2_nodes);
   dump_matrix(out, n_i3, 2, "node_inputs_3/nodes", tensors->node_inputs_3_nodes);
   dump_matrix(out, n_is, 3, "symbol_inputs/nodes", tensors->symbol_inputs_nodes);
}

void EnigmaSocketSend(EnigmaSocket_p sock, EnigmaTensors_p tensors)
{
   int n_ce = tensors->cedges->current + tensors->conj_cedges->current;
   //int n_te = tensors->tedges->current + tensors->conj_tedges->current;
   int n_s = tensors->fresh_s;
   int n_c = tensors->fresh_c;
   int n_t = tensors->fresh_t;
   int n_q = n_c - (tensors->conj_fresh_c - tensors->context_cnt); // query clauses (evaluated and context clauses)
   int n_is = tensors->n_is;
   int n_i1 = tensors->n_i1;
   int n_i2 = tensors->n_i2;
   int n_i3 = tensors->n_i3;

   socket_str(sock, "{");
   socket_vector_int32(sock, n_t, "ini_nodes", tensors->ini_nodes);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_s, "ini_symbols", tensors->ini_symbols);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_c, "ini_clauses", tensors->ini_clauses);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_t, "node_inputs_1/lens", tensors->node_inputs_1_lens);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_i1, "node_inputs_1/symbols", tensors->node_inputs_1_symbols);
   socket_str(sock, ",");
   socket_vector_float(sock, n_i1, "node_inputs_1/sgn", tensors->node_inputs_1_sgn);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_t, "node_inputs_2/lens", tensors->node_inputs_2_lens);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_i2, "node_inputs_2/symbols", tensors->node_inputs_2_symbols);
   socket_str(sock, ",");
   socket_vector_float(sock, n_i2, "node_inputs_2/sgn", tensors->node_inputs_2_sgn);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_t, "node_inputs_3/lens", tensors->node_inputs_3_lens);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_i3, "node_inputs_3/symbols", tensors->node_inputs_3_symbols);
   socket_str(sock, ",");
   socket_vector_float(sock, n_i3, "node_inputs_3/sgn", tensors->node_inputs_3_sgn);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_s, "symbol_inputs/lens", tensors->symbol_inputs_lens);
   socket_str(sock, ",");
   socket_vector_float(sock, n_is, "symbol_inputs/sgn", tensors->symbol_inputs_sgn);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_t, "node_c_inputs/lens", tensors->node_c_inputs_lens);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_ce, "node_c_inputs/data", tensors->node_c_inputs_data);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_c, "clause_inputs/lens", tensors->clause_inputs_lens);
   socket_str(sock, ",");
   socket_vector_int32(sock, n_ce, "clause_inputs/data", tensors->clause_inputs_data);
   socket_str(sock, ",");
   socket_vector_int32(sock, 1, "prob_segments/lens", tensors->prob_segments_lens);
   socket_str(sock, ",");
   socket_vector_int32(sock, 1+n_q, "prob_segments/data", tensors->prob_segments_data);
   socket_str(sock, ",");
   //socket_vector_int32(sock, n_q, "labels", tensors->labels);
   socket_matrix(sock, n_i1, 2, "node_inputs_1/nodes", tensors->node_inputs_1_nodes);
   socket_str(sock, ",");
   socket_matrix(sock, n_i2, 2, "node_inputs_2/nodes", tensors->node_inputs_2_nodes);
   socket_str(sock, ",");
   socket_matrix(sock, n_i3, 2, "node_inputs_3/nodes", tensors->node_inputs_3_nodes);
   socket_str(sock, ",");
   socket_matrix(sock, n_is, 3, "symbol_inputs/nodes", tensors->symbol_inputs_nodes);
   socket_str(sock, "}");
   socket_finish(sock);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

