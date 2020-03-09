#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>
#include <string.h>
#include <cio_commandline.h>
#include <cco_sine.h>
#include <cio_output.h>
#include <cte_termbanks.h>
#include <ccl_formulafunc.h>
#include <ccl_proofstate.h>
#include <che_clausesetfeatures.h>
#include <che_enigmatensors.h>

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

char *outname = NULL;
char *enigmapname= NULL;
FunctionProperties free_symb_prop = FPIgnoreProps;
ProblemType problemType  = PROBLEM_FO;
bool app_encode = false;

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void read_conjecture(char* fname, EnigmaTensors_p tensors, TB_p bank)
{
   Scanner_p in = CreateScanner(StreamTypeFile, fname, true, NULL, NULL);
   ScannerSetFormat(in, TSTPFormat);
   tensors->conj_mode = true;
   while (TestInpId(in, "cnf"))
   {
      Clause_p clause = ClauseParse(in, bank);
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture)
      {
         EnigmaTensorsUpdateClause(clause, tensors);
      }
      ClauseFree(clause);
   }
   CheckInpTok(in, NoToken);
   tensors->conj_mode = false;
   tensors->conj_maxvar = tensors->maxvar; // save maxvar to restore
   EnigmaTensorsReset(tensors);
   DestroyScanner(in);
}

static void read_clauses(char* fname, EnigmaTensors_p tensors, TB_p bank)
{
   Scanner_p in = CreateScanner(StreamTypeFile, fname, true, NULL, NULL);
   ScannerSetFormat(in, TSTPFormat);
   while (TestInpId(in, "cnf"))
   {
      Clause_p clause = ClauseParse(in, bank);
      EnigmaTensorsUpdateClause(clause, tensors);
      ClauseFree(clause);
   }
   CheckInpTok(in, NoToken);
   DestroyScanner(in);
}

static void python_vector_int32(PyObject* dict, int size, char* id, int32_t* values)
{
   PyObject* list = PyList_New((Py_ssize_t)size);
   for (int i=0; i<size; i++)
   {
      PyList_SetItem(list, (Py_ssize_t)i, PyLong_FromLong(values[i]));
   }
   PyObject* key = PyUnicode_FromString(id);
   PyDict_SetItem(dict, key, list);
   Py_DECREF(key);
   Py_DECREF(list);
}

static void python_vector_float(PyObject* dict, int size, char* id, float* values)
{
   PyObject* list = PyList_New((Py_ssize_t)size);
   for (int i=0; i<size; i++)
   {
      PyList_SetItem(list, (Py_ssize_t)i, PyLong_FromLong(((long)values[i])));
   }
   PyObject* key = PyUnicode_FromString(id);
   PyDict_SetItem(dict, key, list);
   Py_DECREF(key);
   Py_DECREF(list);
}

static void python_matrix(PyObject* dict, int dimx, int dimy, char* id, int32_t* values)
{
   int size = dimx*dimy;
   PyObject* list = PyList_New((Py_ssize_t)size);
   for (int i=0; i<size; i++)
   {
      PyList_SetItem(list, (Py_ssize_t)i, PyLong_FromLong(values[i]));
   }
   PyObject* key = PyUnicode_FromString(id);
   PyDict_SetItem(dict, key, list);
   Py_DECREF(key);
   Py_DECREF(list);
}

void python_labels(PyObject* dict, char* id, int n_pos, int n_neg)
{
   int size = n_pos + n_neg;
   PyObject* list = PyList_New((Py_ssize_t)size);
   for (int i=0; i<size; i++)
   {
      PyList_SetItem(list, (Py_ssize_t)i, PyLong_FromLong((i<n_pos) ? 1 : 0));
   }
   PyObject* key = PyUnicode_FromString(id);
   PyDict_SetItem(dict, key, list);
   Py_DECREF(key);
   Py_DECREF(list);
}

void python_symbols(PyObject* dict, char* id, bool func, EnigmaTensors_p tensors)
{
   PStack_p stack;
   NumTree_p node;
   Sig_p sig = tensors->tmp_bank->sig;
   bool is_func;
   char* name;
   PyObject* key;
   PyObject* val;
   PyObject* map = PyDict_New();

   stack = NumTreeTraverseInit(tensors->conj_syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      is_func = node->key && SigIsFunction(sig, node->key);
      if (is_func != func) 
      {
         continue;
      }
      name = (node->key!=sig->eqn_code) ?  SigFindName(sig, node->key) : "=";
      key = PyUnicode_FromString(name);
      val = PyLong_FromLong(node->val1.i_val);
      PyDict_SetItem(map, key, val);
      Py_DECREF(key);
      Py_DECREF(val);
   }
   NumTreeTraverseExit(stack);
   
   stack = NumTreeTraverseInit(tensors->syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      is_func = node->key && SigIsFunction(sig, node->key);
      if (is_func != func) 
      {
         continue;
      }
      name = (node->key!=sig->eqn_code) ?  SigFindName(sig, node->key) : "=";
      key = PyUnicode_FromString(name);
      val = PyLong_FromLong(node->val1.i_val);
      PyDict_SetItem(map, key, val);
      Py_DECREF(key);
      Py_DECREF(val);
   }
   NumTreeTraverseExit(stack);
   
   key = PyUnicode_FromString(id);
   PyDict_SetItem(dict, key, map);
   Py_DECREF(key);
   Py_DECREF(map);
}

static PyObject* python_fill(
   EnigmaTensors_p tensors, 
   int n_pos, 
   int n_neg)
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

   PyObject* dict = PyDict_New();
   python_vector_int32(dict, n_t, "ini_nodes", tensors->ini_nodes);
   python_vector_int32(dict, n_s, "ini_symbols", tensors->ini_symbols);
   python_vector_int32(dict, n_c, "ini_clauses", tensors->ini_clauses);
   python_vector_int32(dict, n_t, "node_inputs_1/lens", tensors->node_inputs_1_lens);
   python_vector_int32(dict, n_i1, "node_inputs_1/symbols", tensors->node_inputs_1_symbols);
   python_vector_float(dict, n_i1, "node_inputs_1/sgn", tensors->node_inputs_1_sgn);
   python_vector_int32(dict, n_t, "node_inputs_2/lens", tensors->node_inputs_2_lens);
   python_vector_int32(dict, n_i2, "node_inputs_2/symbols", tensors->node_inputs_2_symbols);
   python_vector_float(dict, n_i2, "node_inputs_2/sgn", tensors->node_inputs_2_sgn);
   python_vector_int32(dict, n_t, "node_inputs_3/lens", tensors->node_inputs_3_lens);
   python_vector_int32(dict, n_i3, "node_inputs_3/symbols", tensors->node_inputs_3_symbols);
   python_vector_float(dict, n_i3, "node_inputs_3/sgn", tensors->node_inputs_3_sgn);
   python_vector_int32(dict, n_s, "symbol_inputs/lens", tensors->symbol_inputs_lens);
   python_vector_float(dict, n_is, "symbol_inputs/sgn", tensors->symbol_inputs_sgn);
   python_vector_int32(dict, n_t, "node_c_inputs/lens", tensors->node_c_inputs_lens);
   python_vector_int32(dict, n_ce, "node_c_inputs/data", tensors->node_c_inputs_data);
   python_vector_int32(dict, n_c, "clause_inputs/lens", tensors->clause_inputs_lens);
   python_vector_int32(dict, n_ce, "clause_inputs/data", tensors->clause_inputs_data);
   python_vector_int32(dict, 1, "prob_segments/lens", tensors->prob_segments_lens);
   python_vector_int32(dict, 1+n_q, "prob_segments/data", tensors->prob_segments_data);
   //python_vector_int32(dict, n_q, "labels", tensors->labels);
   python_matrix(dict, n_i1, 2, "node_inputs_1/nodes", tensors->node_inputs_1_nodes);
   python_matrix(dict, n_i2, 2, "node_inputs_2/nodes", tensors->node_inputs_2_nodes);
   python_matrix(dict, n_i3, 2, "node_inputs_3/nodes", tensors->node_inputs_3_nodes);
   python_matrix(dict, n_is, 3, "symbol_inputs/nodes", tensors->symbol_inputs_nodes);
   
   python_symbols(dict, "sig/func", true, tensors);
   python_symbols(dict, "sig/pred", false, tensors);
   python_labels(dict, "labels", n_pos, n_neg);

   return dict;
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

static PyObject* parse(PyObject* self, PyObject* args)
{
   const char *f_pos;
   const char *f_neg;
   const char *f_cnf;
   
   if (!PyArg_ParseTuple(args, "sss", &f_pos, &f_neg, &f_cnf))
   {
      return NULL;
   }
   
   ProofState_p state = ProofStateAlloc(free_symb_prop);
   EnigmaTensors_p tensors = EnigmaTensorsAlloc();
   tensors->tmp_bank = TBAlloc(state->signature);

   read_conjecture((char*)f_cnf, tensors, state->terms);
   read_clauses((char*)f_pos, tensors, state->terms);
   int n_pos = tensors->fresh_c - tensors->conj_fresh_c;
   read_clauses((char*)f_neg, tensors, state->terms);
   int n_neg = tensors->fresh_c - tensors->conj_fresh_c - n_pos;

   EnigmaTensorsFill(tensors);

   PyObject* dict = python_fill(tensors, n_pos, n_neg);
   
   EnigmaTensorsFree(tensors);
   ProofStateFree(state);

   return dict;
}

/*---------------------------------------------------------------------*/
/*                           Python module                             */
/*---------------------------------------------------------------------*/

static PyMethodDef Methods[] = {
   {"parse",  parse, METH_VARARGS, "Parse clauses and return tensors."},
   {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef enigmatic_tensors = {
   PyModuleDef_HEAD_INIT,
   "enigmatic_tensors",   /* name of module */
   NULL, /* module documentation, may be NULL */
   -1,   /* size of per-interpreter state of the module,
            or -1 if the module keeps state in global variables. */
   Methods
};

PyMODINIT_FUNC
PyInit_enigmatic_tensors(void)
{
   return PyModule_Create(&enigmatic_tensors);
}

