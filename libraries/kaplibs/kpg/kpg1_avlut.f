      SUBROUTINE KPG1_AVLUT( PNLUT, NDFL, PNTRI, EL, STATUS )
*+
*  Name:
*     KPG1_AVLUT

*  Purpose:
*     Associates, validates and maps an lookup table stored in an NDF.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL KPG1_AVLUT( PNLUT, NDFL, PNTRI, EL, STATUS )

*  Description:
*     This routine associates for read access an NDF that is presumed
*     to contain a lookup table in its data array.  A series of
*     validation checks are made: the array must be two-dimensional, the
*     first dimension must be 3, the range of values must lie in the
*     range 0.0--1.0.  The last of these requires that the data array
*     is mapped therefore for convenience and efficiency a pointer and
*     length are returned.  The lookup table mapped with type _REAL.

*  Arguments:
*     PNLUT = CHARACTER * ( * ) (Given)
*        The ADAM parameter to be associated with the NDF.
*     NDFL = INTEGER (Returned)
*        The identifier for the NDF containing the lookup table.
*     PNTRI( 1 ) = INTEGER (Returned)
*        The pointer to the mapped NDF lookup table.
*     EL = INTEGER (Returned)
*        The length of the mapped NDF.
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  [optional_subroutine_items]...
*  Copyright:
*     Copyright (C) 1991 Science & Engineering Research Council.
*     Copyright (C) 2004 Central Laboratory of the Research Councils.
*     Copyright (C) 2010 Science & Technology Facilities Council.
*     All Rights Reserved.

*  Licence:
*     This programme is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either Version 2 of
*     the License, or (at your option) any later version.
*
*     This programme is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE.  See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with this programme; if not, write to the Free Software
*     Foundation, Inc., 59, Temple Place, Suite 330, Boston, MA
*     02111-1307, USA.

*  Authors:
*     MJC: Malcolm J. Currie (STARLINK)
*     TIMJ: Tim Jenness (JAC, Hawaii)
*     {enter_new_authors_here}

*  History:
*     1991 April 15 (MJC):
*        Original version.
*     2004 September 1 (TIMJ):
*        Use CNF_PVAL.
*     2010 August 26 (MJC):
*        Call KPG_DIMLS instead of DIMLST.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE            ! No implicit typing allowed

*  Global Constants:
      INCLUDE 'SAE_PAR'        ! Global SSE definitions
      INCLUDE 'CNF_PAR'        ! For CNF_PVAL function

*  Arguments Given:
      CHARACTER * ( * )
     :  PNLUT

*  Arguments Returned:
      INTEGER
     :  NDFL,
     :  PNTRI( 1 ),
     :  EL

*  Status:
      INTEGER STATUS

*  Local Constants:
      INTEGER NDIM             ! Dimensionality of the NDFs
      PARAMETER ( NDIM = 2 )

      INTEGER NPRICL           ! Number of primary colours
      PARAMETER ( NPRICL = 3 )

*  Local Variables:
      INTEGER
     :  LDIMS( NDIM ),         ! Dimensions of lookup table in image
                               ! file
     :  MAXPOS,                ! Position of the maximum (not used)
     :  MINPOS,                ! Position of the minimum (not used)
     :  NCDIM,                 ! Number characters in dimension list
     :  NDIMS,                 ! Total number of NDF dimensions
     :  NINVAL                 ! Number of bad values in the input array

      CHARACTER * 72
     :  DIMSTR                 ! List of dimensions of an input array

      REAL
     :  RMAXV,                 ! Minimum value in the array
     :  RMINV                  ! Maximum value in the array

      LOGICAL                  ! True if :
     :  BAD                    ! The array contains bad pixels

*    Check the inherited status.

      IF ( STATUS .NE. SAI__OK ) RETURN

*    Obtain the NDF identifier of the input lookup table.

      CALL LPG_ASSOC( PNLUT, 'READ', NDFL, STATUS )

*    Obtain the array dimensions.

      CALL NDF_DIM( NDFL, NDIM, LDIMS, NDIMS, STATUS )

*    Must have a 2-d array.  A bad status will be generated by NDF_DIM
*    if there are greater than 2 significant dimensions.

      IF ( NDIM .LT. NDIMS .AND. STATUS .EQ. SAI__OK ) THEN
         STATUS = SAI__ERROR
         CALL NDF_MSG( 'NDFL', NDFL )
         CALL ERR_REP( 'KPG1_AVLUT_IVDIM',
     :     'Lookup table in NDF ^NDFL is not two-dimensional.', STATUS )
      END IF

*    Explicetly check whether or not bad pixels are present.

      CALL NDF_BAD( NDFL, 'Data', .TRUE., BAD, STATUS )

      IF ( BAD .AND. STATUS .EQ. SAI__OK ) THEN
         STATUS = SAI__ERROR
         CALL NDF_MSG( 'NDFL', NDFL )
         CALL ERR_REP( 'KPG1_AVLUT_IVDIM',
     :     'Lookup table NDF ^NDFL has unsupported bad values.',
     :     STATUS )
      END IF

*    Check that the dimensions are valid.

      IF ( STATUS .EQ. SAI__OK .AND. LDIMS( 1 ) .NE. NPRICL ) THEN

*       Construct the token containing the dimensions.

         CALL KPG_DIMLS( NDIM, LDIMS, NCDIM, DIMSTR, STATUS )
         CALL MSG_SETC( 'DIMSTR', DIMSTR( 1:NCDIM ) )

*       Report the error.

         STATUS = SAI__ERROR
         CALL NDF_MSG( 'NDFL', NDFL )
         CALL ERR_REP( 'KPG1_AVLUT_LUTSIZE',
     :     ' Lookup table ^NDFL has invalid dimensions '/
     :     /'^DIMSTR.', STATUS )
      END IF

*    Map the input LUT.

      PNTRI( 1 ) = 0
      CALL KPG1_MAP( NDFL, 'Data', '_REAL', 'READ', PNTRI, EL, STATUS )

*    Obtain the maximum and minimum values.

      CALL KPG1_MXMNR( .FALSE., EL, %VAL( CNF_PVAL( PNTRI( 1 ) ) ),
     :                 NINVAL,
     :                 RMAXV, RMINV, MAXPOS, MINPOS, STATUS )

*    To be a valid LUT, the values must lie in the range 0.0--1.0.

      IF ( ( RMINV .LT. 0.0 .OR. RMAXV .GT. 1.0 ) .AND.
     :     STATUS .EQ. SAI__OK ) THEN
         STATUS = SAI__ERROR
         CALL NDF_MSG( 'NDFL', NDFL )
         CALL ERR_REP( 'KPG1_AVLUT_RANGE',
     :     'NDF ^NDFL has values outside the permitted range for a '/
     :     /'lookup table.', STATUS )
      END IF

      END
