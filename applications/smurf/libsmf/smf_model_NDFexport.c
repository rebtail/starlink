/*
*+
*  Name:
*     smf_model_NDFexport

*  Purpose:
*     Export DIMM model component stored in smfData to NDF file

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     C function

*  Invocation:
*     smf_model_NDFexport( const smfData *data, const char *name, 
*                           int *status );

*  Arguments:
*     data = const smfData* (Given)
*        Input smfData (any variety) that will have its DATA array copied.
*     name = const char* (Given)
*        Name of the NDF container
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This function creates a new smfData struct with associated NDF
*  contained, with the same dimensions as the input, and memcpy's the DATA
*  array over. This routine can be used to write any smfData not associated
*  with a file to an NDF container although it should probably be modified
*  to check/copy over more information (such as history, or other components if
*  they exist).
*
*  Authors:
*     EC: Edward Chapin (UBC)
*     {enter_new_authors_here}

*  History:
*     2007-06-15 (EC):
*        Initial version.
*     2008-04-03 (EC):
*        Asset ICD data-order before writing file.

*  Notes:
*
*  Copyright:
*     Copyright (C) 2005-2006 Particle Physics and Astronomy Research Council.
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
#include "star/ndg.h"
#include "prm_par.h"
#include "par_par.h"

/* SMURF includes */
#include "libsmf/smf.h"
#include "libsmf/smf_err.h"

#define FUNC_NAME "smf_model_NDFexport"

void smf_model_NDFexport( const smfData *data, const char *name, int *status ){
  /* Local Variables */
  int added=0;                  /* Number of names added to group */
  size_t datalen;               /* Length in bytes of data array */
  int flag=0;                   /* Flag */
  int i;                        /* Counter */
  int lbnd[NDF__MXDIM];         /* Dimensions of container */
  Grp *inname = NULL;           /* 1-element group to hold input filename */
  int msize=0;                  /* Number of files in name group */
  size_t ndata;                 /* Number of elements in data array */
  int osize=0;                  /* Number of files in model group */
  Grp *outname = NULL;          /* 1-element group to hold output filename */
  smfData *tempdata=NULL;       /* Temporary smfData pointer */
  int ubnd[NDF__MXDIM];         /* Dimensions of container */

  char ndfname[GRP__SZNAM+1];  /* Input NDF name, derived from GRP */
  char *pname=NULL;

  if (*status != SAI__OK ) return;

  /* Assert ICD-compliant data order */
  smf_dataOrder( data, 1, status );
  if( *status = SMF__WDIM ) {
    /* fails if not 3-dimensional data. Just annul and write out data
       with other dimensions as-is */
    errAnnul(status);
  }

  /* Make a 1-element group containing the name of the new file */
  inname = grpNew( "GRP", status );
  outname = grpNew( "GRP", status );
  grpPut1( inname, name, 1, status );
  grpGrpex( "*|dimm|sdf|", inname, outname, &msize, &added, &flag, status );

  /* Create lbnd and ubnd arrays, and calculate buffer size */
  if( *status == SAI__OK ) {
    ndata = 1;
    for( i=0; i<data->ndims; i++ ) {
      lbnd[i] = 1;
      ubnd[i] = data->dims[i];
      ndata *= data->dims[i];
    }

    datalen = ndata * smf_dtype_sz( data->dtype, status );
  }

  /* Make a new empty container with associated smfData struct */
  smf_open_newfile( outname, 1, data->dtype, data->ndims, lbnd, ubnd, 
		    0, &tempdata, status );

  /* Copy the data array to new smfData */
  if( *status == SAI__OK ) {
    memcpy( (tempdata->pntr)[0], (data->pntr)[0], datalen );
  }

  /* Close files and clean up */  
  smf_close_file( &tempdata, status );
  grpDelet( &inname, status );
  grpDelet( &outname, status );
}
