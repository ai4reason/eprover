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

static void free_edges(PStack_p stack)
{
   while (!PStackEmpty(stack))
   {  
      PDArray_p edge = PStackPopP(stack);
      PDArrayFree(edge);
   }
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/


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

   if (junk->tmp_bank)
   {
      TBFree(junk->tmp_bank);
      junk->tmp_bank = NULL;
   }
   
   EnigmaTensorsCellFree(junk);
}
 
/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

