/*
*+
*  Name:
*     smf_open_related_model

*  Purpose:
*     Like smf_open_related for DIMM model component files

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     SMURF subroutine

*  Invocation:
*     smf_open_related_model( const smfGroup *group, const int subindex, 
*                             const char *accmode, smfArray **relfiles,
*                             int *status );

*  Arguments:
*     group = const smfGroup* (Given)
*        Input smfGroup of model component filenames
*     subindex = const int (Given)
*        Subgroup index
*     accmode = const char* (Given)
*        Access mode for opened files
*     relfiles = smfArray** (Returned)
*        smfArray containing opened files
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This routine takes a smfGroup and opens the files within the
*     subgroup specified by the value of subindex. The smfArray is
*     returned with the smfData pointers set.

*  Notes:
*     - smf_close_related is used to close when finished

*  Authors:
*     Ed Chapin (UBC)
*     {enter_new_authors_here}

*  History:
*     2007-07-10 (EC):
*        Initial version cloned from smf_open_related

*  Copyright:
*     Copyright (C) 2006 University of British Columbia.  All Rights
*     Reserved.

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

#if HAVE_CONFIG_H
#include <config.h>
#endif

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Starlink includes */
#include "sae_par.h"
#include "mers.h"
#include "ndf.h"
#include "star/ndg.h"
#include "star/grp.h"
#include "msg_par.h"

/* SMURF routines */
#include "smf.h"
#include "smf_typ.h"
#include "smf_err.h"

#define FUNC_NAME "smf_open_related_model"

void smf_open_related_model( const smfGroup *group, const int subindex, 
			     const char *accmode, smfArray **relfiles, 
			     int *status ) {

  /* Local variables */
  smfData *data = NULL;     /* Data struct for file */
  Grp *grp = NULL;          /* Grp stored within smfGroup */
  int i;                    /* Loop counter */
  int *indices = NULL;      /* Array of indices */
  int nrelated;             /* Number of related files */
  int index;                /* Index into the subgroups within the group */
  int **subgroups = NULL;   /* Pointer to array of subgroups */

  if ( *status != SAI__OK ) return;

  subgroups = group->subgroups;

  /* Retrieve grp */
  grp = group->grp;
  nrelated = group->nrelated;

  *relfiles = smf_create_smfArray( status );

  for (i=0; i<nrelated; i++) {
    /* Select correct array of indices */
    indices = subgroups[subindex];
    /* Pick out current index - will be 0 if no index is stored at
       that location */
    index = indices[i];
    /* Open file with this index and add to smfArray */
    if ( index != 0 ) {
      smf_open_model( grp, index, accmode, &data, status );
      smf_addto_smfArray( *relfiles, data, status );
    }
  }

}
