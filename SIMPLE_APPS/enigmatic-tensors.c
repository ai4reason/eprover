/*-----------------------------------------------------------------------

File  : enigmatic-tensors.c

Author: AI4REASON

Contents

  Copyright 2019, 2020 by the author(s).
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri Nov 28 00:27:40 MET 1997

-----------------------------------------------------------------------*/

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
/*                  Data types                                         */
/*---------------------------------------------------------------------*/

typedef enum
{
   OPT_NOOPT=0,
   OPT_HELP,
   OPT_VERBOSE,
   OPT_FREE_NUMBERS,
   OPT_OUTPUT,
}OptionCodes;

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

OptCell opts[] =
{
    {OPT_HELP, 
        'h', "help", 
        NoArg, NULL,
        "Print a short description of program usage and options."},
    {OPT_VERBOSE, 
        'v', "verbose", 
        OptArg, "1",
        "Verbose comments on the progress of the program."},
    {OPT_OUTPUT,
        'o', "output-file",
        ReqArg, NULL,
        "Redirect output into the named file."},
   {OPT_FREE_NUMBERS,
    '\0', "free-numbers",
     NoArg, NULL,
     "Treat numbers (strings of decimal digits) as normal free function "
    "symbols in the input. By default, number now are supposed to denote"
    " domain constants and to be implicitly different from each other."},
    {OPT_NOOPT,
        '\0', NULL,
        NoArg, NULL,
        NULL}
};

char *outname = NULL;
char *enigmapname= NULL;
FunctionProperties free_symb_prop = FPIgnoreProps;
ProblemType problemType  = PROBLEM_NOT_INIT;
bool app_encode = false;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[]);
void print_help(FILE* out);

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
   }
   CheckInpTok(in, NoToken);
   DestroyScanner(in);
}

void dump_escape(FILE* out, char* str)
{
   fprintf(out, "\"");
   while (*str)
   {
      switch (*str) 
      {
         case '"':
            fprintf(out, "\\\"");
            break;
         case '\\':
            fprintf(out, "\\\\");
            break;
         default:
            fprintf(out, "%c", *str);
            break;
      }
      str++;
   }
   fprintf(out, "\"");
}

void dump_symbols(FILE* out, char* id, bool func, EnigmaTensors_p tensors)
{
   PStack_p stack;
   NumTree_p node;
   Sig_p sig = tensors->tmp_bank->sig;
   bool is_func;
   char* name;
   bool first = true;
   
   fprintf(out, "   \"%s\": {", id);
   stack = NumTreeTraverseInit(tensors->conj_syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      is_func = node->key && SigIsFunction(sig, node->key);
      if (is_func != func) 
      {
         continue;
      }
      name = (node->key!=sig->eqn_code) ?  SigFindName(sig, node->key) : "=";
      if (!first)
      {
         fprintf(out, " ,");
      }
      dump_escape(out, name);
      fprintf(out, ": %ld", node->val1.i_val);
      first = false;
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
      if (!first)
      {
         fprintf(out, " ,");
      }
      dump_escape(out, name);
      fprintf(out, ": %ld", node->val1.i_val);
      first = false;
   }
   NumTreeTraverseExit(stack);
   
   fprintf(out, "},\n");
}

void dump_labels(FILE* out, char* id, int n_pos, int n_neg)
{
   int size = n_pos + n_neg;
   fprintf(out, "   \"%s\": [", id);
   for (int i=0; i<size; i++)
   {
      fprintf(out, "%d%s", (i<n_pos) ? 1 : 0, (i<size-1) ? ", " : "");
   }
   fprintf(out, "]\n");
}

int main(int argc, char* argv[])
{
   InitIO(argv[0]);
   CLState_p args = process_options(argc, argv);
   //SetMemoryLimit(2L*1024*MEGA);
   OutputFormat = TSTPFormat;
   if (outname) { OpenGlobalOut(outname); }
   ProofState_p state = ProofStateAlloc(free_symb_prop);
   
   EnigmaTensors_p tensors = EnigmaTensorsAlloc();
   tensors->tmp_bank = TBAlloc(state->signature);

   int n_pos = 0;
   if (args->argc == 3)
   {
      read_conjecture(args->argv[2], tensors, state->terms);
   }
   if (args->argc == 1)
   {
      read_clauses(args->argv[0], tensors, state->terms);
      n_pos = tensors->fresh_c - tensors->conj_fresh_c;
   }
   else
   {
      read_clauses(args->argv[0], tensors, state->terms);
      n_pos = tensors->fresh_c - tensors->conj_fresh_c;
      read_clauses(args->argv[1], tensors, state->terms);
   }
   int n_neg = tensors->fresh_c - tensors->conj_fresh_c - n_pos;

   EnigmaTensorsFill(tensors);

   fprintf(GlobalOut, "{\n");
   EnigmaTensorsDump(GlobalOut, tensors);
   dump_symbols(GlobalOut, "sig/func", true, tensors);
   dump_symbols(GlobalOut, "sig/pred", false, tensors);
   dump_labels(GlobalOut, "labels", n_pos, n_neg);
   fprintf(GlobalOut, "}\n");

   EnigmaTensorsFree(tensors);
   
   ProofStateFree(state);
   CLStateFree(args);
   ExitIO();

   return 0;
}


/*-----------------------------------------------------------------------
//
// Function: process_options()
//
//   Read and process the command line option, return (the pointer to)
//   a CLState object containing the remaining arguments.
//
// Global Variables: opts, Verbose, TermPrologArgs,
//                   TBPrintInternalInfo 
//
// Side Effects    : Sets variables, may terminate with program
//                   description if option -h or --help was present
//
/----------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[])
{
   Opt_p handle;
   CLState_p state;
   char*  arg;
   
   state = CLStateAlloc(argc,argv);
   
   while((handle = CLStateGetOpt(state, &arg, opts)))
   {
      switch(handle->option_code)
      {
      case OPT_VERBOSE:
             Verbose = CLStateGetIntArg(handle, arg);
             break;
      case OPT_HELP: 
             print_help(stdout);
             exit(NO_ERROR);
      case OPT_OUTPUT:
             outname = arg;
             break;
      case OPT_FREE_NUMBERS:
            free_symb_prop = free_symb_prop|FPIsInteger|FPIsRational|FPIsFloat;
            break;
      default:
          assert(false);
          break;
      }
   }
   
   if (state->argc < 1 || state->argc > 3)
   {
      print_help(stdout);
      exit(NO_ERROR);
   }
   
   return state;
}
 
void print_help(FILE* out)
{
   fprintf(out, "\n\
\n\
Usage: enigmatic-tensors [options] cnfs.tptp\n\
   or: enigmatic-tensors [options] train.pos train.neg [train.cnf]\n\
\n\
Make Enigma tensors from TPTP cnf clauses.\n\
\n");
   PrintOptions(stdout, opts, NULL);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


