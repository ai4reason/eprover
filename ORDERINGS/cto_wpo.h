/*-----------------------------------------------------------------------

File  : cto_wbo.h

Author: Jan Jakubuv

Contents

  Definitions for implementing WPO.

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

-----------------------------------------------------------------------*/

#ifndef CTO_WPO

#define CTO_WPO

#include <cto_ocb.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

bool          WPOGreater(OCB_p ocb, Term_p s, Term_p t, 
          DerefType deref_s, DerefType deref_t);

CompareResult WPOCompare(OCB_p ocb, Term_p t1, Term_p t2,
          DerefType deref_t1, DerefType deref_t2);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

