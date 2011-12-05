/*
*+
*  Name:
*     smf_stripsuffix

*  Purpose:
*     Strip suffix from string

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     smf_stripsuffix( const char *instr, const char *suffix,
*                      char *outstr, int *status);

*  Arguments:
*     instr = const char * (Given)
*        Source string
*     suffix = const char * (Given)
*        Case-sensitive suffix string to remove
*     outstr = char * (Given)
*        Output string
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Strip suffix from instr and store in outstr.  The longest
*     this string may be be is GRP__SZNAM+1 (including NULL
*     termination).

*  Notes:

*  Authors:
*     Edward Chapin (UBC)
*     {enter_new_authors_here}

*  History:
*     2008-09-30 (EC):
*        Initial Version
*     2011-11-21 (EC):
*        Add arbitrary suffix parameter, rename to smf_stripsuffix from
*        smf_model_stripsuffix
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2008,2011 University of British Columbia.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 3 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*     MA 02111-1307, USA

*  Bugs:
*     {note_any_bugs_here}
*-
*/

/* Starlink includes */
#include "mers.h"
#include "ndf.h"
#include "sae_par.h"
#include "prm_par.h"
#include "par_par.h"
#include "star/one.h"

/* SMURF includes */
#include "libsmf/smf.h"

/* Other includes */
#include <stdio.h>

#define FUNC_NAME "smf_stripsuffix"

void smf_stripsuffix( const char *instr, const char *suffix,
                      char *outstr, int *status) {

  /* Local Variables */
  int added;                    /* Number of names added to group */
  int flag;                     /* Flag */
  char grpex[GRP__SZNAM+1];     /* String for holding grpex */
  Grp *inname=NULL;             /* 1-element group to hold input string */
  size_t len;                   /* Length of buffer */
  size_t msize;                 /* Size of group */
  Grp *outname = NULL;          /* 1-element group to hold output string */
  char *pname=NULL;             /* Poiner to name */

  /* Main routine */
  if (*status != SAI__OK) return;

  inname = grpNew( "GRP", status );
  outname = grpNew( "GRP", status );
  grpPut1( inname, instr, 1, status );

  len = sizeof(grpex);
  one_strlcpy( grpex, "*|", len, status );
  one_strlcat( grpex, suffix, len, status );
  one_strlcat( grpex, "||", len, status );

  grpGrpex( grpex, inname, outname, &msize, &added, &flag, status );
  pname = outstr;
  grpGet( outname, 1, 1, &pname, GRP__SZNAM, status );

  grpDelet( &inname, status );
  grpDelet( &outname, status );
}