/*-----------------------------------------------------------------------

File  : che_to_geofgen.c

Author: Stephan Schulz, Jan Jakubuv

Contents

  Functions implementing several simple coefecient generation schemes for
  the WPO.

  Copyright 1998, 1999, 2018 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

-----------------------------------------------------------------------*/

#include "che_to_coefgen.h"
#include "cto_orderings.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

char* TOCoefGenNames[]=
{
   "none",
   "constant",
   "arity",
   "invarity",
   "firstmax",
   "firstmin",
   "ascend",
   "descend",
   NULL
};



/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: generate_constant_weights()
//
//   Assign the constant W_DEFAULT_WEIGHT to all smbols.
//
// Global Variables: -
//
// Side Effects    : Sets weights in ocb.
//
/----------------------------------------------------------------------*/

static void generate_constant_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=1; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = ALGEBRA_DEFAULT_COEF;
      }
   }
}

static void generate_arity_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=SIG_TRUE_CODE; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = (double)arity;
      }
   }
}

static void generate_invarity_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=SIG_TRUE_CODE; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = (1.0/arity);
      }
   }
}

static void generate_firstmax_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=SIG_TRUE_CODE; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = (j==0) ? 2 : 1;
      }
   }
}

static void generate_firstmin_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=SIG_TRUE_CODE; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = (j==0) ? 1 : 2;
      }
   }
}

static void generate_ascend_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=SIG_TRUE_CODE; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = j+1;
      }
   }
}

static void generate_descend_coefs(OCB_p ocb)
{
   FunCode f;
   int j;

   for(f=SIG_TRUE_CODE; f<=ocb->sig_size; f++)
   {
      int arity = SigFindArity(ocb->sig, f);
      for (j=0; j<arity; j++) 
      {
         *OCBAlgebraCoefPos(ocb,f,j) = arity-j;
      }
   }
}

/*-----------------------------------------------------------------------
//
// Function: set_user_weights()
//
//   Given a user weight string, set the symbols to the desired
//   weight.
//
// Global Variables: -
//
// Side Effects    : May fail with syntax error
//
/----------------------------------------------------------------------*/

void set_user_coefs(OCB_p ocb, char* pre_coefs)
{
   Scanner_p in = CreateScanner(StreamTypeUserString, pre_coefs,
                                true, NULL);

   TOCoefsParse(in, ocb);

   DestroyScanner(in);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/


/*-----------------------------------------------------------------------
//
// Function: TOTranslateWeightGenMethod()
//
//   Given a string, return the corresponding TOWeightGenMethod
//   token.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/


TOCoefGenMethod TOTranslateCoefGenMethod(char* name)
{
   int method;

   method = StringIndex(name, TOCoefGenNames);

   if(method == -1)
   {
      method = CNoMethod;
   }
   return (TOCoefGenMethod)method;
}


/*-----------------------------------------------------------------------
//
// Function: TOGenerateWeights()
//
//   Given a pre-initialized OCB, assign weights to the function
//   symbols. Some methods require a precedence, some require the
//   axioms.
//
// Global Variables: -
//
// Side Effects    : Sets weights in ocb.
//
/----------------------------------------------------------------------*/

void TOGenerateCoefs(OCB_p ocb, char *pre_coefs, TOCoefGenMethod method)
{
   assert(ocb);
   assert(ocb->sig);
   assert(ocb->algebra_coefs);

   switch(method)
   {
      case CConstantCoefs:
         generate_constant_coefs(ocb);
         break;
      case CArityCoefs:
         generate_arity_coefs(ocb);
         break;
      case CInvArityCoefs:
         generate_invarity_coefs(ocb);
         break;
      case CFirstMaxCoefs:
         generate_firstmax_coefs(ocb);
         break;
      case CFirstMinCoefs:
         generate_firstmin_coefs(ocb);
         break;
      case CAscendCoefs:
         generate_ascend_coefs(ocb);
         break;
      case CDescendCoefs:
         generate_descend_coefs(ocb);
         break;
      default:
         Error("Unsupported algebra coeficient generation method (%d)\n", USAGE_ERROR, method);
         break;
   }

   if(pre_coefs)
   {
      set_user_coefs(ocb, pre_coefs);
   }

}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


