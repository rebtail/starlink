/*
*+
*  Name:
*     smf_history_check

*  Purpose:
*     Determine whether a particular action has been performed on a file

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     C function

*  Invocation:
*     int smf_history_check( const smfData * data, const char * appl,
*                            int * status );

*  Arguments:
*     data = const smfData* (Given)
*        Data struct to check. This must be associated with an open
*        file.
*     appl = const char * (Given)
*        Name of "application" to check. Will only compare the
*        application name against exactly the length of this supplied
*        string. ie EXTINCTION and EXTINCTIONS will both match "EXTINCTION".
*     status = int* (Given and Returned)
*        Pointer to global status. Will be bad on return if this smfData
*        is not associated with a file.

*  Return Value:
*     int = smf_history_check
*        Returns true if the application has been run, false otherwise.

*  Description:
*     This function compares the supplied application name with those
*     stored in the file history. Only the characters present in this
*     reference name are compared.

*  Authors:
*     Tim Jenness (JAC, Hawaii)
*     Andy Gibb (UBC)
*     {enter_new_authors_here}

*  History:
*     2006-01-24 (TIMJ):
*        Initial version.
*     2006-04-20 (AGG):
*        Use history AstKeyMap instead of file history
*     2006-10-12 (AGG):
*        Remove refappl variable

*  Notes:
*     - Applications names are compared case sensitively. Uppercase
*       is recommended.
*     - If the data struct corresponds to a raw data file, routine
*       always returns false since by definition the application
*       could not have been run on it.
*     - Be careful when comparing substrings...
*     - See also smf_history_write

*  Copyright:
*     Copyright (C) 2006 Particle Physics and Astronomy Research
*     Council and the University of British Columbia.  All Rights
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

/* System includes */
#include <string.h>

/* Starlink includes */
#include "ast.h"
#include "sae_par.h"
#include "mers.h"

/* SMURF includes */
#include "smf.h"
#include "smf_typ.h"
#include "smf_err.h"

/* Simple default string for errRep */
#define FUNC_NAME "smf_history_check"

int smf_history_check( const smfData* data, const char * appl, int *status) {
  size_t applen;       /* Length of supplied appl string */
  int i = 0;           /* Loop counter */
  int nrec = 0;        /* Number of history records */
  int retval = 0;      /* Return value */
  
  /* Check entry status */
  if (*status != SAI__OK) return retval;

  /* check that we have a smfData */
  if ( data == NULL ) {
    *status = SAI__ERROR;
    errRep( FUNC_NAME,
	    "Supplied smfData is a NULL pointer. Possible programming error.",
	    status);
    return retval;
  }

  applen = strlen( appl );

  nrec = astMapSize( data->history );

  if ( nrec != 0 ) {
    for ( i=0; i<nrec; i++ ) {
      if ( strncmp( astMapKey( data->history, i ), appl, applen ) == 0 ) {
	retval = 1;
	break;
      }
    }
  }

  return retval;
}
