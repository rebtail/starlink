      SUBROUTINE KPG1_LUDCD( N, EL, ARRAY, PINDEX, SCALE, EVEN,
     :                         STATUS )
*+
*  Name:
*     KPG1_LUDCx
 
*  Purpose:
*     Performs an LU decomposition of a square matrix.
 
*  Language:
*     Starlink Fortran 77
 
*  Invocation:
*     CALL KPG1_LUDCx( N, EL, ARRAY, PINDEX, SCALE, EVEN, STATUS )
 
*  Description:
*     This routine performs a decomposition of a square matrix into
*     lower and upper triangular matrices using Crout's method with
*     partial pivoting.  Implicit pivoting is also used to select
*     the pivots.
 
*  Arguments:
*     N = INTEGER (Given)
*        The number of rows and columns in the matrix to be decomposed.
*     EL = INTEGER (Given)
*        The size of the first dimension of the array as declared in the
*        calling routine.  This should be at least equal to N.
*     ARRAY( EL, N ) = ? (Given and Returned)
*        On input this is the array to be decomposed into LU form.
*        On exit it is the LU decomposed form of the rowwise-permuted
*        input ARRAY, with the diagonals being part of the upper
*        triangular matrix.  The permutation is given by PINDEX.
*     SCALE( N ) = ? (Returned)
*        Workspace to store the implicit scaling used to normalise the
*        rows during implicit pivoting.
*     PINDEX( N ) = INTEGER (Returned)
*        An index of the row permutations caused by the partial
*        pivoting.
*     EVEN = LOGICAL (Returned)
*        If EVEN is .TRUE., there was an even number of row
*        interchanges during the decomposition.  If EVEN is .FALSE.,
*        there was an odd number.
*     STATUS = INTEGER (Given and Returned)
*        The global status.
 
*  Notes:
*     -  There is a routine for the data types real or double precision:
*     replace "x" in the routine name by R or D respectively, as
*     appropriate.  The matrix and the workspace should have this data
*     type as well.
 
*  [optional_subroutine_items]...
*  Authors:
*     MJC: Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}
 
*  History:
*     1993 March 3 (MJC):
*        Original version.
*     {enter_changes_here}
 
*  Bugs:
*     {note_any_bugs_here}
 
*-
 
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing
 
*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'PRM_PAR'          ! PRIMDAT public constants
 
*  Arguments Given:
      INTEGER N
      INTEGER EL
 
*  Arguments Given and Returned:
      DOUBLE PRECISION ARRAY( EL, N )
 
*  Arguments Returned:
      INTEGER PINDEX( N )
      DOUBLE PRECISION SCALE( N )
      LOGICAL EVEN
 
*  Status:
      INTEGER STATUS             ! Global status
 
*  Local Variables:
      DOUBLE PRECISION COLMAX              ! Column maximum
      DOUBLE PRECISION DUMMY               ! Dummy value used for swapping
      INTEGER I                  ! Loop counter
      INTEGER J                  ! Loop counter
      INTEGER K                  ! Loop counter
      DOUBLE PRECISION MERIT               ! Degree of merit of a pivot
      INTEGER PIVOT              ! Pivot index
      DOUBLE PRECISION SUM                 ! Stores summation
 
*.
 
*  Check the inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN
 
*  Initialise the number of interchanges.  Zero is even.
      EVEN = .TRUE.
 
*  Determine whether the matrix is singular.  Find the largest
*  (absolute) value in the matrix column.
      DO I = 1, N
         COLMAX = 0.0D0
         DO J = 1, N
            COLMAX = MAX( COLMAX, ABS( ARRAY( I, J ) ) )
         END DO
 
*  Abort and report when a singular matrix is found.
         IF ( COLMAX .LT. VAL__EPSD ) THEN
            STATUS = SAI__ERROR
            CALL ERR_REP( 'KPG1_LUDCx_SINGUL',
     :        'A matrix cannot be LU decomposed because it is '/
     :        /'singular.', STATUS )
            GOTO 999
         END IF
 
*  Store the scaling.
         SCALE( I ) = 1.0D0 / COLMAX
      END DO
 
*  Apply Crout's algorithm.
*  ========================
*
*  This solves the N*N + N equations for the triangular matrix elements
*  by just arranging the equations in a certain order.
*  Loop over the columns.
      DO J = 1, N
         IF ( J .GT. 1 ) THEN
            DO I = 1, J - 1
 
*  Form the summation and replace the original value by the sum.  The
*  decomposition is done in situ because the original matrix elements
*  are only used once.  Here we are finding the upper triangular
*  matrix, once the element of the lower matrix is known (i.e. J is not
*  1).
               SUM = ARRAY( I, J )
               DO K = 1, I - 1
                  SUM = SUM - ARRAY( I, K ) * ARRAY( K, J )
               END DO
               ARRAY( I, J ) = SUM
            END DO
         END IF
 
*  Search for the largest pivot item.
         COLMAX = 0.0D0
         DO I = J, N
 
*  Form the summation and replace the original value by the sum.  The
*  decomposition is done in situ because the original matrix elements
*  are only used once.  Here we are finding the lower triangular matrix.
            SUM = ARRAY( I, J )
            DO K = 1, J - 1
               SUM = SUM - ARRAY( I, K ) * ARRAY( K, J )
            END DO
            ARRAY( I, J ) = SUM
 
*  Find how good the pivot is.  We have to scale the values.
            MERIT = SCALE( I ) * ABS( SUM )
 
*  If it is the best so far, record it and its position.
            IF ( MERIT .GT. COLMAX ) THEN
               PIVOT = I
               COLMAX = MERIT
            END IF
         END DO
 
*  Do we need to interchange the rows.
         IF ( J .NE. PIVOT ) THEN
 
*  Yes, so swap.
            DO K = 1, N
               DUMMY = ARRAY( PIVOT, K )
               ARRAY( PIVOT, K ) = ARRAY( J, K )
               ARRAY( J, K ) = DUMMY
            END DO
 
*  Record the parity of the interchanges.
            EVEN = .NOT. EVEN
 
*  Interchange the scale factor.
            SCALE( PIVOT ) = SCALE( J )
         END IF
 
*  Record the index of the pivot.
         PINDEX( J ) = PIVOT
 
*  The diagonal must not be zero, otherwise the matrix is singular.
*  Therefore assign it a small non-zero value.  This is needed to
*  prevent a division by zero below.
         IF ( ABS( ARRAY( J, J ) ) .LT. VAL__EPSD ) THEN
            ARRAY( J, J ) = VAL__EPSD
         END IF
 
*  Divide by the pivot element.
         IF ( J .NE. N ) THEN
            DO I = J + 1, N
               ARRAY( I, J ) = ARRAY( I, J ) / ARRAY( J, J )
            END DO
         END IF
      END DO
 
  999 CONTINUE
      END
