/*-----------------------------------------------------------------------

File  : fofshared.c

Author: Josef Urban

Contents
 
  Read an initial set of fof terms and print the (shared) codes of all subterms
  present in them. If file names given (or file and stdin), read both
  in, but only print the codes for the second one (this is intended to
  allow consistent codes over several runs).
 

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri Nov 28 00:27:40 MET 1997

-----------------------------------------------------------------------*/

#include <stdio.h>
#include <cio_commandline.h>
#include <cio_output.h>
#include <cte_termbanks.h>
#include <ccl_formulafunc.h>
#include <ccl_proofstate.h>
#include <che_enigma.h>

/*---------------------------------------------------------------------*/
/*                  Data types                                         */
/*---------------------------------------------------------------------*/

typedef enum
{
   OPT_NOOPT=0,
   OPT_HELP,
   OPT_VERBOSE,
   OPT_OUTPUT
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
    {OPT_NOOPT,
        '\0', NULL,
        NoArg, NULL,
        NULL}
};

char *outname = NULL;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[]);
void print_help(FILE* out);

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void term_features_string(DStr_p str, Term_p term, Sig_p sig, char* sym1, char* sym2)
{
   char* sym3;

   if (TermIsVar(term)) 
   {
      sym3 = ENIGMA_VAR;
   }
   else if ((strncmp(SigFindName(sig,term->f_code),"esk",3) == 0)) 
   {
      sym3 = ENIGMA_SKO;
   }
   else 
   {
      sym3 = SigFindName(sig, term->f_code);
   }

   DStrAppendStr(str, sym1); DStrAppendChar(str, ':');
   DStrAppendStr(str, sym2); DStrAppendChar(str, ':');
   DStrAppendStr(str, sym3); DStrAppendChar(str, ' ');
   
   if (TermIsVar(term)||(TermIsConst(term))) {
      return;
   }
   for (int i=0; i<term->arity; i++)
   {
      term_features_string(str, term->args[i], sig, sym2, sym3);
   }
}

static void clause_features_string(DStr_p str, Clause_p clause, Sig_p sig)
{
   for(Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      char* sym1 = EqnIsPositive(lit)?ENIGMA_POS:ENIGMA_NEG;
      if (lit->rterm->f_code == SIG_TRUE_CODE)
      {
         char* sym2 = SigFindName(sig, lit->lterm->f_code);
         for (int i=0; i<lit->lterm->arity; i++) // here we ignore prop. constants
         {
            term_features_string(str, lit->lterm->args[i], sig, sym1, sym2);
         }
      }
      else
      {
         char* sym2 = ENIGMA_EQ;
         term_features_string(str, lit->lterm, sig, sym1, sym2);
         term_features_string(str, lit->rterm, sig, sym1, sym2);
      }
   }
}

static DStr_p get_conjecture_features_string(char* filename, TB_p bank)
{
   DStr_p str = DStrAlloc();
   Scanner_p in = CreateScanner(StreamTypeFile, filename, true, NULL);
   ScannerSetFormat(in, TSTPFormat);
   while (TestInpId(in, "cnf"))
   {
      Clause_p clause = ClauseParse(in, bank);
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture) {
         clause_features_string(str, clause, bank->sig);
      }
      ClauseFree(clause);
   }
   CheckInpTok(in, NoToken);
   DestroyScanner(in);
   DStrDeleteLastChar(str);
   return str;
}

static void dump_features_strings(FILE* out, char* filename, TB_p bank, char* prefix, char* conj)
{
   DStr_p str = DStrAlloc();
   Scanner_p in = CreateScanner(StreamTypeFile, filename, true, NULL);
   ScannerSetFormat(in, TSTPFormat);
   while (TestInpId(in, "cnf"))
   {
      Clause_p clause = ClauseParse(in, bank);
      clause_features_string(str, clause, bank->sig);
      DStrDeleteLastChar(str);
      fprintf(out, "%s|%s|%s\n", prefix, DStrView(str), conj);
      DStrReset(str);
      ClauseFree(clause);
   }
   CheckInpTok(in, NoToken);
   DestroyScanner(in);
   DStrFree(str);
}

int main(int argc, char* argv[])
{
   InitIO(argv[0]);
   CLState_p args = process_options(argc, argv);
   OutputFormat = TSTPFormat;
   Sig_p sig = SigAlloc(SortTableAlloc());
   // free numbers hack:
   sig->distinct_props = sig->distinct_props&(~(FPIgnoreProps|FPIsInteger|FPIsRational|FPIsFloat));
   TB_p bank = TBAlloc(sig);
  
   DStr_p conj = get_conjecture_features_string(args->argv[0], bank); 
   dump_features_strings(GlobalOut, args->argv[1], bank, "+", DStrView(conj));
   dump_features_strings(GlobalOut, args->argv[2], bank, "-", DStrView(conj));

   DStrFree(conj);
   TBFree(bank);
   SigFree(sig);
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
      default:
          assert(false);
          break;
      }
   }

   if (state->argc != 3)
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
Usage: enigma-features [options] train.cnf train.pos train.neg\n\
\n\
Read a set of terms and print it with size and depth information.\n\
\n");
   PrintOptions(stdout, opts, NULL);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


