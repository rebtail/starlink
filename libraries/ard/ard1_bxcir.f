      SUBROUTINE ARD1_BXCIR( NDIM, LBND, UBND, MSKSIZ, VALUE, LBOX,
     :                       UBOX, NPAR, PAR, B, STATUS )
*+
*  Name:
*     ARD1_BXCIR

*  Purpose:
*     Assign a constant value to a circle region specified in user
*     co-ordinates.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL ARD1_BXCIR( NDIM, LBND, UBND, MSKSIZ, VALUE, LBOX, UBOX, 
*                      NPAR, PAR, B, STATUS )

*  Description:
*     All pixels which are within the supplied bounding box and also
*     within the circle defined by PAR are assign the value given by
*     argument VALUE. A "sledge-hammer" approach is used in which
*     each pixel in the supplied bounding box is transformed into user
*     coordinates, and then the distance from the circle centre is
*     found. The algorithm works in any number of dimensions.

*  Arguments:
*     NDIM = INTEGER (Given)
*        The number of dimensions in the array.
*     LBND( NDIM ) = INTEGER (Given)
*        The lower pixel index bounds of the array.
*     UBND( NDIM ) = INTEGER (Given)
*        The upper pixel index bounds of the array.
*     MSKSIZ = INTEGER (Given)
*        The total number of elements in the array.
*     VALUE = INTEGER (Given)
*        The value to be assigned to the circle.
*     LBOX( NDIM ) = INTEGER (Given)
*        The lower pixel bounds of a box enclosing the circle. A value
*        of VAL__MAXI for element 1 is used to indicate an infinite
*        box, and a value of VAL__MINI for element 1 is used to
*        indicate a zero sized box.
*     UBOX( NDIM ) = INTEGER (Given)
*        The upper pixel bounds of the box.
*     NPAR = INTEGER (Given)
*        No. of values in PAR.
*     PAR( NPAR ) = REAL (Given)
*        Parameters; Coeffs of user to pixel transformation, followed by
*        user coords of circle centre, followed by the radius in user
*        co-ordinates.
*     B( MSKSIZ ) = INTEGER (Given and Returned)
*        The array (in vector form).
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Authors:
*     DSB: David Berry (STARLINK)
*     {enter_new_authors_here}

*  History:
*     11-APR-1994 (DSB):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
      
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'ARD_CONST'        ! ARD private constants
      INCLUDE 'PRM_PAR'          ! VAL_ constants

*  Arguments Given:
      INTEGER NDIM
      INTEGER LBND( NDIM )
      INTEGER UBND( NDIM )
      INTEGER MSKSIZ
      INTEGER VALUE
      INTEGER LBOX( NDIM )
      INTEGER UBOX( NDIM )
      INTEGER NPAR
      REAL PAR( NPAR )

*  Arguments Given and Returned:
      INTEGER B( MSKSIZ )

*  Status:
      INTEGER STATUS             ! Global status

*  Local Variables:
      INTEGER
     :        BDIM,              ! A dimension size in the box
     :        BINDEX( ARD__MXDIM ),! Current Cartesian co-ordinates
     :        BOXEL,             ! Vector address within the box
     :        BOXSIZ,            ! No. of pixels in the box
     :        CEN0,              ! Offset to centre coordinates in PAR
     :        I,                 ! Loop count
     :        LLBOX( ARD__MXDIM ),! Local copy of LBOX
     :        LUBOX( ARD__MXDIM ),! Local copy of UBOX
     :        MDIM,              ! A dimension size in supplied array
     :        P,                 ! No. of pixels in (N-1)-Dim. object 
     :        VA,                ! Vector address within supplied array
     :        VAINC( ARD__MXDIM) ! VA increment between N-D objects

      REAL
     :        C( ARD__MXDIM*(1+ARD__MXDIM) ),! Inverse transformation
     :        PCO(ARD__MXDIM),   ! Pixel coordinates
     :        R2,                ! Square of pixel-centre distance
     :        RSQ,               ! Square of required radius
     :        UCO(ARD__MXDIM)    ! User coordinates

*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  If the box is null, return without doing anything.                  
      IF( LBOX( 1 ) .NE. VAL__MINI ) THEN

*  Find the inverse of the supplied transformation (i.e. from pixel to
*  user co-ordinates).
         CALL ARD1_INVRS( NDIM, PAR, C, STATUS )

*  Store the offset (within PAR) to the centre co-ordinates.
         CEN0 = NDIM*( 1 + NDIM )

*  Store the square of the circle radius.
         RSQ = PAR( CEN0 + NDIM + 1 )**2

*  Working in a variable number of dimensions introduces complications
*  not met if the dimensionality is fixed; particularly, arrays cannot
*  be declared with the correct number of dimensions (since the number
*  is only known at run-time). They can therefore only be declared as
*  1-d vectors. The position of each pixel in the box found above can
*  be described in several ways;
*
*  1. By Cartesian pixel indices.
*
*  2. By a single vector address starting with 1 at the lower bounds
*  corner of the box and increasing to BOXSIZ at the upper bounds
*  corner (where BOXSIZ = total number of pixels in the box).
*
*  3. By a single vector address starting with 1 at the lower bounds
*  corner of the array B and increasing to MSKSIZ at the upper bounds
*  corner (where MSKSIZ = total number of pixels in array B).
*
*  The pixels forming the box will have continuus vector addresses
*  using system 2. above, but will have gaps using system 3.  The
*  first task is to establish constants needed to be able to transform
*  positions from system 2. to system 3. (only system 3. can be used to
*  actually access the arrays).
*
*  Initialise things.
         P = 1
         BOXSIZ = 1
         VA = 1

*  Loop round every dimension.
         DO I = 1, NDIM

*  Limit the USED box to the bounds of the mask.
            LLBOX( I ) = MAX( LBOX( I ), LBND( I ) )
            LUBOX( I ) = MIN( UBOX( I ), UBND( I ) )

*  If the box is null, return without doing anything.
            IF( LLBOX( I ) .GT. LUBOX( I ) ) GO TO 999

*  Set the Cartesian index of the first box pixel to be processed to the
*  lower box bound.
            BINDEX( I ) = LLBOX( I )

*  Store the size of the current box dimension, and include it as a
*  factor in the total number of pixels in the box.
            BDIM = LUBOX( I ) - LLBOX( I ) + 1
            BOXSIZ = BOXSIZ*BDIM

*  Update the B array vector address (system 3.) of the lower bounds
*  corner of the box.
            VA = VA + ( LLBOX( I ) - LBND( I ) )*P

*  Store the size of the current dimension of the input arrays.
            MDIM = UBND( I ) - LBND( I ) + 1

*  Store the increase in the B array vector address which is required to
*  move from the end of one row, plane, cube, etc (depending on the
*  current dimension) of the box, to the start of the next row, plane,
*  cube, etc.
            VAINC( I ) = P*( MDIM - BDIM )

*  Find the number of pixels included in a row,plane,cube,etc, of array
*  B.
            P = P*MDIM

         END DO

*  Loop round every element in the box. The Cartesian indices of the
*  current box pixel are contained in BINDEX. The B array vector address
*  (system 3.) of the current pixel is VA. The box vector address
*  (system 2.) of the current pixel is BOXEL.
         DO BOXEL = 1, BOXSIZ

*  Store the pixel coordinates corresponding to the centre of the
*  current pixel.
            DO I = 1, NDIM
               PCO( I ) = REAL( BINDEX( I ) ) - 0.5
            END DO

*  Transform these pixel co-ordinates into user co-ordinates.
            CALL ARD1_TRANS( NDIM, C, PCO, UCO )

*  Find the square of the distance of this pixel from the circle centre.
            R2 = 0.0
            DO I = 1, NDIM
               R2 = R2 + ( UCO( I ) - PAR( CEN0 + I ) )**2
            END DO

*  If the pixel is inside the user box, assign the value to the
*  current pixel. 
            IF( R2 .LE. RSQ ) B( VA ) = VALUE

*  Increment the B array vector address of the next pixel to be
*  processed
            VA = VA + 1

*  Increment the Cartesian index on the first axis of the box.
            BINDEX( 1 ) = BINDEX( 1 ) + 1

*  If the index is now larger than the corresponding upper bound of the
*  box...
            I = 1
            DO WHILE( BINDEX( I ) .GT. LUBOX( I ) ) 

*  Reset the index to the corresponding lower bound.
               BINDEX( I ) = LLBOX( I )

*  If the last dimension has been reached, leave things as they are so
*  that the DO WHILE loop will exit.
               IF( I .LT. NDIM ) THEN

*  Increase the B array vector address to point to the start of the next
*  row, plane, cube, etc (depending on the current dimension) of the
*  box.
                  VA = VA + VAINC( I )

*  Increment the index for the next higher dimension.
                  I = I + 1
                  BINDEX( I ) = BINDEX( I ) + 1
   
               END IF
   
            END DO
   
         END DO

      END IF

 999  CONTINUE

      END
