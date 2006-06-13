#include "sae_par.h"
#include "cupid.h"
#include <string.h>

void cupidCFXfer( CupidPixelSet *ps1, CupidPixelSet *ps2, int *ipa, 
                  int skip[3], int *status ){
/*
*+
*  Name:
*     cupidCFXfer

*  Purpose:
*     Transfer all the pixels in one PixelSet into another.

*  Language:
*     Starlink C

*  Synopsis:
*     void cupidCFXfer( CupidPixelSet *ps1, CupidPixelSet *ps2, int *ipa, 
*                       int skip[3], int *status )

*  Description:
*     This function transfer all the pixels in PixelSet "ps1" into
*     PixelSet "ps2". This involves changing the index value (stored in
*     the "ipa" array) associated with each pixel in the source PixelSet,
*     and extending the bounding box of the destination PixelSet to
*     encompass the source PixelSet.

*  Parameters:
*     ps1
*        Pointer to the source PixelSet structure containing the pixels to
*        be moved.
*     ps2
*        Pointer to the destination PixelSet structure to receive the pixels 
*        moved from "ps1".
*     ipa
*        Pointer to the start of the array holding the integer index
*        (if any) associated with each pixel in the data array. This
*        array should be the same shape and size as the data array.
*     skip
*        The increment in 1D vector index required to move a distance of 1 
*        pixel along each axis. This allows conversion between indexing
*        the array using a single 1D vector index and using nD coords.
*        Unused trailing elements should be filled with zero.
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
*     26-NOV-2005 (DSB):
*        Original version.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
*/

/* Local Variables: */

   int *v1;         /* Pointer to element at start of this row */
   int *v2;         /* Pointer to element at start of this plane */
   int *v;          /* Pointer to next array element */
   int i;           /* GRID axis 1 value of next array element */
   int j;           /* GRID axis 2 value of next array element */
   int k;           /* GRID axis 3 value of next array element */
   int new_index;   /* Index value to assign to the transferred pixels */
   int old_index;   /* Original index value of the transferred pixels */

/* Check inherited status. Also return without action if the two
   PointSets are the same.  */
   if( *status != SAI__OK || ps1 == ps2 ) return;

/* Get the index value of the source PixelSet. */
   old_index = ps1->index;

/* Get the index value of the destination PixelSet. */
   new_index = ps2->index;

/* Get a pointer to the first pixel in the source PixelSet bounding box. If 
   the data  has less than 3 axes, the unused upper and lower bounds will be 
   set to [1,1] and so we can always pretend there are 3 axes. */ 
   v = ipa + ( ps1->lbnd[ 0 ] - 1 ) + ( ps1->lbnd[ 1 ] - 1 )*skip[ 1 ] + 
             ( ps1->lbnd[ 2 ] - 1 )*skip[ 2 ];

/* Loop round the pixels in the source PixelSet bounding box. */
   for( k = ps1->lbnd[ 2 ]; k <= ps1->ubnd[ 2 ]; k++ ) {
      v2 = v;
      for( j = ps1->lbnd[ 1 ]; j <= ps1->ubnd[ 1 ]; j++ ) {
         v1 = v;
         for( i = ps1->lbnd[ 0 ]; i <= ps1->ubnd[ 0 ]; i++ ) {

/* Assign the new index to the pixel, if the pixel was originally a member 
   of the source PixelSet.  */
            if( *v == old_index ) *v = new_index;

/* Get the pointer to the next pixel in the source PixelSet bounding box. */
            v++;
         }
         v = v1 + skip[ 1 ];
      }
      v = v2 + skip[ 2 ];
   }

/* Update the bounds of the destination PixelSet so that they encompass
   the bounds of the source PixelSet. */
   if( ps1->lbnd[ 0 ] < ps2->lbnd[ 0 ] ) ps2->lbnd[ 0 ] = ps1->lbnd[ 0 ];
   if( ps1->lbnd[ 1 ] < ps2->lbnd[ 1 ] ) ps2->lbnd[ 1 ] = ps1->lbnd[ 1 ];
   if( ps1->lbnd[ 2 ] < ps2->lbnd[ 2 ] ) ps2->lbnd[ 2 ] = ps1->lbnd[ 2 ];

   if( ps1->ubnd[ 0 ] > ps2->ubnd[ 0 ] ) ps2->ubnd[ 0 ] = ps1->ubnd[ 0 ];
   if( ps1->ubnd[ 1 ] > ps2->ubnd[ 1 ] ) ps2->ubnd[ 1 ] = ps1->ubnd[ 1 ];
   if( ps1->ubnd[ 2 ] > ps2->ubnd[ 2 ] ) ps2->ubnd[ 2 ] = ps1->ubnd[ 2 ];

/* Update the populations of the two PixelSets. */
   ps2->pop += ps1->pop;
   ps1->pop = 0;

/* If the source PixelSet touches the edge, then so does the destination 
   PixelSet. */
   if( ps1->edge ) ps2->edge = 1;

/* If the peak value in the source PixelSet is greater than in the
   destination PixcelSet, use the source peak instead of the original
   destination peak. */
   if( ps1->vpeak > ps2->vpeak ) {
      ps2->vpeak = ps1->vpeak;
      ps2->peak[ 0 ] = ps1->peak[ 0 ];
      ps2->peak[ 1 ] = ps1->peak[ 1 ];
      ps2->peak[ 2 ] = ps1->peak[ 2 ];
   }

/* Add an list of neigbours contained in the source PixelSet into the
   list of neighbours in the destination pixelet. */
   if( ps1->nneb > 0 ) {
      ps2->nebs = astGrow( ps2->nebs, ps1->nneb + ps2->nneb, sizeof( int ) );
      if( astOK ) {
         memcpy( ps2->nebs + ps2->nneb,
                 ps1->nebs, ps1->nneb*sizeof( int ) );
         ps2->nneb += ps1->nneb;
      }
      ps1->nebs = astFree( ps1->nebs );
      ps1->nneb = 0;
   }

}
