#include "sae_par.h"
#include "prm_par.h"
#include "star/hds.h"
#include "star/kaplibs.h"
#include "par.h"
#include "ast.h"
#include "ndf.h"
#include "mers.h"
#include "cupid.h"
#include "cupid.h"
#include <string.h>
#include <stdio.h>

/* Local Constants: */
#define MAXCAT   50   /* Max length of catalogue name */

void cupidStoreClumps( const char *param, HDSLoc *xloc, HDSLoc *obj, 
                       int ndim, double beamcorr[ 3 ], const char *ttl,
                       AstFrameSet *iwcs, int ilevel, int *status ){
/*
*+
*  Name:
*     cupidStoreClumps

*  Purpose:
*     Store properties of all clumps found by the CLUMPS command.

*  Language:
*     Starlink C

*  Synopsis:
*     void cupidStoreClumps( const char *param, HDSLoc *xloc, HDSLoc *obj, 
*                            int ndim, double beamcorr[ 3 ], const char *ttl, 
*                            AstFrameSet *iwcs, int ilevel, int *status )

*  Description:
*     This function optionally saves the clump properties in an output
*     catalogue, and then copies the NDF describing the found clumps into 
*     the supplied CUPID extension.

*  Parameters:
*     param
*        The ADAM parameter to associate with the output catalogue.
*     xloc
*        HDS locator for the CUPID extension of the NDF in which to store
*        the clump properties. May be NULL.
*     obj
*        A locator for an HDS array the clump NDF structures.
*     ndim
*        The number of pixel axes in the data.
*     beamcorr
*        An array holding the FWHM (in pixels) describing the instrumental 
*        smoothing along each pixel axis. The clump widths stored in the 
*        output catalogue are reduced to correct for this smoothing.
*     ttl
*        The title for the output catalogue (if any).
*     iwcs
*        The WCS FrameSet from the input data, or NULL.
*     ilevel
*        The level of information to display.
*     status
*        Pointer to the inherited status value.

*  Copyright:
*     Copyright (C) 2005 Particle Physics & Astronomy Research Council.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 59 Temple Place,Suite 330, Boston, MA
*     02111-1307, USA

*  Authors:
*     DSB: David S. Berry
*     {enter_new_authors_here}

*  History:
*     10-NOV-2005 (DSB):
*        Original version.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
*/

/* Local Variables: */

   AstFrame *frm1;              /* Frame describing clump parameters */
   AstFrame *frm2;              /* Frame describing clump centres */
   AstMapping *map;             /* Mapping from "frm1" to "frm2" */
   HDSLoc *aloc;                /* Locator for array of Clump structures */
   HDSLoc *ncloc;               /* Locator for array cell */
   HDSLoc *cloc;                /* Locator for array cell */
   HDSLoc *dloc;                /* Locator for cell value */
   char unit[ 10 ];             /* String for NDF Unit component */
   char attr[ 15 ];             /* AST attribute name */
   char cat[ MAXCAT + 1 ];      /* Catalogue name */
   const char *dom;             /* Pointer to domain string */
   const char **names;          /* Component names */
   double *cpars;               /* Array of parameters for a single clump */
   double *t;                   /* Pointer to next table value */
   double *tj;                  /* Pointer to next table entry to write*/
   double *tab;                 /* Pointer to catalogue table */
   int bad;                     /* Does clump touch an area of bad pixels? */
   int i;                       /* Index of next locator */
   int iclump;                  /* Usable clump index */
   int icol;                    /* Zero based column index */
   int ifrm;                    /* Frame index */
   int indf2;                   /* Identifier for copied NDF */
   int indf;                    /* Identifier for supplied NDF */
   int irow;                    /* One-based row index */
   int nbad;                    /* No. of clumps touching an area of bad pixels */
   int ncpar;                   /* Number of clump parameters */
   int nfrm;                    /* Total number of Frames */
   int nndf;                    /* Total number of NDFs */
   int nsmall;                  /* No. of clumps smaller than the beam size */
   int ok;                      /* Is the clump usable? */
   int place;                   /* Place holder for copied NDF */
   int there;                   /* Does component exist?*/

/* Abort if an error has already occurred. */
   if( *status != SAI__OK ) return;

/* Get the total number of NDFs supplied. */
   datSize( obj, (size_t *) &nndf, status );

/* If we are writing the information to an NDF extension, create an array 
   of "nndf" Clump structures in the extension, and get a locator to it. */
   if( xloc ) {
      aloc = NULL;
      datThere( xloc, "CLUMPS", &there, status );
      if( there ) datErase( xloc, "CLUMPS", status );
      datNew( xloc, "CLUMPS", "CLUMP", 1, &nndf, status );
      datFind( xloc, "CLUMPS", &aloc, status );
   } else {
      aloc = NULL;
   }

/* Indicate that no memory has yet been allocated to store the parameters
   for a single clump. */
   cpars = NULL;

/* Indicate we have not yet found any clumps smaller than the beam size. */
   nsmall = 0;

/* Indicate we have not yet found any clumps that touch any areas of
   bad pixels. */
   nbad = 0;

/* Number of CLUMP structures created so far. */
   iclump = 0;

/* Indicate that no memory has yet been allocated to store the full table
   of parameters for all clumps. */
   tab = NULL;

/* Loop round the non-null identifiers, keeping track of the one-based row 
   number corresponding to each one. */
   irow = 0;
   for( i = 1; i <= nndf && *status == SAI__OK; i++ ) {
      ncloc = NULL;
      datCell( obj, 1, &i, &ncloc, status );

      errBegin( status );
      ndfFind( ncloc, " ", &indf, status );
      errEnd( status );

      datAnnul( &ncloc,status );
      if( indf != NDF__NOID ) {
         irow++;

/* The Unit component of the NDF will be set to "BAD" if the clump
   touches any areas of bad pixels in the input data array. Count how
   many of these clumps there are. */
         unit[ 0 ] = 0;
         ndfCget( indf, "Unit", unit, 9, status );
         if( !strcmp( unit, "BAD" ) ){
            bad = 1;
            nbad++;
         } else {
            bad = 0;
         }

/* Calculate the clump parameters from the clump data values stored in the 
   NDF. This allocates memory if needed, and also returns some global
   information which is the same for every clump (the parameter names, the 
   indices of the parameters holding the clump central position, and the 
   number of parameters). */
         cpars = cupidClumpDesc( indf, beamcorr, cpars, &names, &ncpar, &ok, status );

/* If we have not yet done so, allocate memory to hold a table of clump 
   parameters. In this table, all the values for column 1 come first, 
   followed by all the values for column 2, etc (this is the format required 
   by KPG1_WRLST). */ 
         if( !tab ) tab = astMalloc( sizeof(double)*nndf*ncpar );
         if( tab ) {

/* Count the number of lumps which are smaller than the beam size. Also
   set the Unit component of the NDF to "BAD" to indicate that the clump
   should not be used. */
            if( bad ){
               ok = 0;

            } else if( !ok ) {
               ndfCput( "BAD", indf, "Unit", status );
               nsmall++;
            }

/* Put the clump parameters into the table. Store bad values if the clump
   is too small. */
            t = tab + irow - 1;
            for( icol = 0; icol < ncpar; icol++ ) {
               *t = ok ? cpars[ icol ] : VAL__BADD;
               t += nndf;
            }

/* If required, put the clump parameters into the current CLUMP structure. */
            if( aloc && ok ) {
                   
/* Get an HDS locator for the next cell in the array of CLUMP structures. */
               iclump++;
               cloc = NULL;
               datCell( aloc, 1, &iclump, &cloc, status );

/* Store each clump parameter in a component of this CLUMP structure. */
               dloc = NULL;
               for( icol = 0; icol < ncpar; icol++ ) {
                  datNew( cloc, names[ icol ], "_DOUBLE", 0, NULL, status );
                  datFind( cloc, names[ icol ], &dloc, status );
                  datPutD( dloc, 0, NULL, cpars + icol, status );
                  datAnnul( &dloc, status );
               }

/* Store the supplied NDF in a component called "MODEL" of the CLUMP
   structure. */
               ndfPlace( cloc, "MODEL", &place, status );
               ndfCopy( indf, &place, &indf2, status );
               ndfAnnul( &indf2, status );

/* Free the locator to the CLUMP structure. */
               datAnnul( &cloc, status );
            }
         }
         ndfAnnul( &indf, status );
      }
   }

/* Tell the user how many usable clumps there are and how many were rejected 
   due to being smaller than the beam size. */
   if( ilevel > 1 ) {

      if( nsmall == 1 ) {
         msgOut( "", "1 further clump rejected because it "
                 "is smaller than the beam width.", status );
      } else if( nsmall > 1 ) {
         msgSeti( "N", nsmall );
         msgOut( "", "^N further clumps rejected because "
                 "they are smaller than the beam width.", status );
      }

      if( nbad == 1 ) {
         msgOut( "", "1 further clump rejected because it includes "
                 "too many bad pixels.", status );
      } else if( nbad > 1 ) {
         msgSeti( "N", nbad );
         msgOut( "", "^N further clumps rejected because they include "
                 "too many bad pixels.", status );
      }
   }

   if( ilevel > 0 ) {
      if( iclump == 0 ) {
         msgOut( "", "No usable clumps found.", status );
      } else if( iclump == 1 ){
         msgOut( "", "One usable clump found.", status );
      } else {
         msgSeti( "N", iclump );
         msgOut( "", "^N usable clumps found.", status );
      }
      msgBlank( status );
   }

/* Resize the array of clump structures */
   if( aloc && iclump < nndf && iclump ) datAlter( aloc, 1, &iclump, status );

/* See if an output catalogue is to be created. If not, annull the null
   parameter error. */
   parGet0c( param, cat, MAXCAT, status );
   if( *status == PAR__NULL ) {
      errAnnul( status );
  
/* Otherwise create the catalogue. */
   } else if( tab && *status == SAI__OK ) {

/* Remove any rows in the table which descibe clumps smaller than the
   beam size (these will have been set to bad values above). The good
   rows are shuffled down to fill the gaps left by the bad rows. */
      iclump = 0;
      for( irow = 0; irow < nndf; irow++ ) {
         if( tab[ irow ] != VAL__BADD ) {
            if( irow != iclump ) {
               t = tab + irow;
               tj = tab + iclump;
               for( icol = 0; icol < ncpar; icol++ ) {
                  *tj = *t;
                  tj += nndf;
                  t += nndf;
               }
            }
            iclump++;
         } 
      }

/* Start an AST context. */
      astBegin;
   
/* Create a Frame with "ncpar" axes describing the table columns. Set the
   axis Symbols to the column names. */
      frm1 = astFrame( ncpar, "Domain=PARAMETERS,Title=Clump parameters" );
      for( icol = 0; icol < ncpar; icol++ ) {
         sprintf( attr, "Symbol(%d)", icol + 1 );
         astSetC( frm1, attr, names[ icol ] );
      }
   
/* Create a Mapping (a PermMap) from the Frame representing the "ncpar" clump
   parameters, to the "ndim" Frame representing clump centre pixel positions. 
   The inverse transformation supplies bad values for the other parameters. */
      map = (AstMapping *) astPermMap( ncpar, NULL, ndim, NULL, NULL, "" );
   
/* If no WCS FrameSet was supplied.... */
      if( !iwcs ) {

/* Create a Frame with "ndim" axes describing the pixel coords at the
   clump centre. */
         frm2 = astFrame( ndim, "Domain=PIXEL,Title=Pixel coordinates" );
         astSetC( frm2, "Symbol(1)", "P1" );
         if( ndim > 1 ) {
            astSetC( frm2, "Symbol(2)", "P2" );
            if( ndim > 2 ) astSetC( frm2, "Symbol(3)", "P3" );
         }
   
/* Create a FrameSet to store in the output catalogue. It has two Frames,
   the base Frame has "ncpar" axes - each axis describes one of the table
   columns. The current Frame has 2 axes and describes the clump (x,y)
   position. The ID value of FIXED_BASE is a special value recognised by 
   kpg1Wrlst. */
         iwcs = astFrameSet( frm1, "ID=FIXED_BASE" );
         astAddFrame( iwcs, AST__BASE, map, frm2 );
         astSetI( iwcs, "CURRENT", 1 );

/* If a WCS FrameSet was supplied, add in "frm1" as the base Frame,
   connecting it to the original PIXEL Frame using "map". */
      } else {


/* Loop round all Frames in the FrameSet, looking for one with Domain
   PIXEL (also remove the GRID and AXIS Frames). */
         nfrm = astGetI( iwcs, "NFrame" );
         for( ifrm = 0; ifrm < nfrm; ifrm++ ) {
            dom = astGetC( astGetFrame( iwcs, ifrm ), "Domain" );
            if( dom ){
               if( !strcmp( dom, "PIXEL" ) ) {

/* When found add in the new Frame and make it the base Frame. Then
   re-instate the original current Frame. */
                  astInvert( map );
                  astAddFrame( iwcs, ifrm, map, frm1 );
                  astSetI( iwcs, "Base", astGetI( iwcs, "Current" ) );

/* Remove the Frames introduced by the NDF library. */
               } else if( !strcmp( dom, "AXIS" ) ||
                          !strcmp( dom, "GRID" ) ) {
                  astRemoveFrame( iwcs, ifrm );
               }
            }
         }

/* Set the ID attribute of the FrameSet to "FIXED_BASE" in order to force
   kpg1_wrlst to write out the positions in the original base Frame. */
         astSet( iwcs, "ID=FIXED_BASE" );
      }
   
/* Create the output catalogue */
      if( iclump > 0 ) {
         kpg1Wrlst( param, nndf, iclump, ncpar, tab, AST__BASE, iwcs,
                    ttl, 1, NULL, 1, status );
      }
   
/* End the AST context. */
      astEnd;
   }

/* If required, annul the locator for the array of CLUMP structures. */
   if( aloc ) datAnnul( &aloc, status );

/* Free resources. */
   tab = astFree( tab );
   cpars = astFree( cpars );

}
