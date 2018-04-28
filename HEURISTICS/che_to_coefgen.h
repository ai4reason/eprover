/*-----------------------------------------------------------------------

File  : che_to_weightgen.h

Author: Stephan Schulz, Jan Jakubuv

Contents

  Routines for generating weights for term orderings

  Copyright 1998, 1999, 2018 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

-----------------------------------------------------------------------*/

#ifndef CHE_TO_COEFGEN

#define CHE_TO_COEFGEN

#include <clb_simple_stuff.h>
#include <cto_ocb.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef enum
{
   CInvalidEntry = -1,
   CNoMethod = 0,         /* Nothing */
   CConstantCoefs,        /* All coefs 1 */
   CArityCoefs,           /* coef(f,i) = arity(f) */
   CInvArityCoefs,        /* coef(f,i) = 1/arity(f) TODO: non-terminating! */
   CFirstMaxCoefs,        /* coef(f,1)=2, coef(f,_)=1 */
   CFirstMinCoefs,        /* coef(f,1)=1, coef(f,_)=2 */
   CAscendCoefs,          /* coef(f,1)=1, coef(f,n+1) = coef(f,n)+1 */
   CDescendCoefs          /* coef(f,1)=n, coef(f,n+1) = coef(f,n)-1 */
}TOCoefGenMethod;

#define ALGEBRA_DEFAULT_COEF 1


/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/


extern char* TOCoefGenNames[];

#define TOGetCoefGenName(method) \
        (TOWeightGenNames[(method)])

TOCoefGenMethod TOTranslateCoefGenMethod(char* name);

#define TOGenerateDefaultCoefs(ocb) \
        TOGenerateCoefs((ocb), NULL, CConstantCoefs)

void TOGenerateCoefs(OCB_p ocb, char *pre_coefs, 
   TOCoefGenMethod method);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

