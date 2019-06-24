/*-----------------------------------------------------------------------

File  : che_enigma.c

Author: Jan Jakubuv

Contents
 
  Copyright 2016 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

<1> Sat Jul  5 02:28:25 MET DST 1997
    New

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMA

#define CHE_ENIGMA

#include <clb_fixdarrays.h>
#include <clb_numtrees.h>
#include <ccl_clauses.h>

#include "linear.h"
#include "svdlib.h"

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef enum
{
   EFNone = 0,
   EFVertical = 1,
   EFHorizontal = 2,
   EFSymbols = 4,
   EFLengths = 8,
   EFConjecture = 16,
   EFProofWatch = 32,
   EFVariables = 64,
   EFHashing = 128,
   EFComprehension = 256,  // John
   EFAll = 0xFFFF
}EnigmaFeature;

typedef long EnigmaFeatures;

typedef struct enigmapcell
{
   Sig_p sig;
   EnigmaFeatures version;
   
   StrTree_p feature_map;
   long feature_count;

} EnigmapCell, *Enigmap_p;

#define EnigmapCellAlloc() (EnigmapCell*) \
        SizeMalloc(sizeof(EnigmapCell))
#define EnigmapCellFree(junk) \
        SizeFree(junk, sizeof(EnigmapCell))

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define ENIGMA_POS "+"
#define ENIGMA_NEG "-"
#define ENIGMA_VAR "*"
#define ENIGMA_SKO "?"
#define ENIGMA_EQ "="

Enigmap_p EnigmapAlloc(void);
void EnigmapFree(Enigmap_p junk);

Enigmap_p EnigmapLoad(char* features_filename, Sig_p sig);

EnigmaFeatures ParseEnigmaFeaturesSpec(char *spec);

DStr_p FeaturesGetTermHorizontal(char* top, Term_p term, Sig_p sig);
DStr_p FeaturesGetEqHorizontal(Term_p lterm, Term_p rterm, Sig_p sig);

void FeaturesClauseVariablesExtend(NumTree_p* stat, Clause_p clause, int* distinct, int offset);
void FeaturesClauseVariablesStat(NumTree_p* stat, long* out);
void FeaturesClauseVariables(Clause_p clause, long* out);

int FeaturesClauseExtend(NumTree_p* counts, Clause_p clause, Enigmap_p enigmap);
void FeaturesAddClauseStatic(NumTree_p* counts, Clause_p clause, Enigmap_p enigmap, int *len);
NumTree_p FeaturesClauseCollect(Clause_p clause, Enigmap_p enigmap, int* len);

void FeaturesSvdTranslate(DMat matUt, double* sing, 
   struct feature_node* in, struct feature_node* out);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

