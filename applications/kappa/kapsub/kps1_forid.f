      SUBROUTINE KPS1_FORID( M, N, A, B, STATUS )
*+
*  Name:
*     KPS1_FORIx
 
*  Purpose:
*     Converts data held in modulus and phase form to complex form
*     (separate real and imaginary parts).
 
*  Language:
*     Starlink Fortran 77
 
*  Invocation
*     CALL KPS1_FORIx( M, N, A, B, STATUS )
 
*  Description:
*     Converts a 2-dimensional array held in modulus and phase form to
*     complex form (separate real and imaginary parts).
 
*  Arguments:
*     M = INTEGER (Given)
*        Number of pixels per line of the FFT.
*     N = INTEGER (Given)
*        Number of lines in the FFT.
*     A( M, N ) = ? (Given and Returned)
*        On input it is `power' = modulus of complex value.
*        On output it is the real part of the FFT.
*     B( M, N ) = ? (Given and Returned)
*        On input it is `phase' = arctan(imag/real) in radians.
*        On output it is the imaginary part of the FFT.
*     STATUS = INTEGER (Given)
*        The global status.
 
*  Notes:
*     -  There is a routine for real and double-precision floating-
*     point data: replace "x" in the routine name by D or R as
*     appropriate.  The arrays supplied to the routine must have the
*     data type specified.
 
*  Algorithm:
*     - The input complex data is overwritten by the power and phase
*       data.  `Power' is here used to refer to the modulus of the
*       complex value, and phase is arctan(imaginary/real) in
*       radians.
 
*  Authors:
*     DSB: D.S. Berry (STARLINK)
*     MJC: Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}
 
*  History:
*     1988 Jun 6 (DSB):
*        Original version.
*     1990 Mar 19 (MJC):
*        Converted to KAPPA style and reordered the arguments.
*     9-JAN-1995 (DSB):
*        Converted to double precision. Re-format comments in
*        edstar-style.
*     1995 March 29 (MJC):
*        Used the modern style of variable declaration, and minor
*        stylistic changes.
*     1995 March 30 (MJC):
*        Made generic from REALIM.
*     {enter_further_changes_here}
 
*  Bugs:
*     {note_any_bugs_here}
 
*-
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing
 
*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
 
*  Arguments Given:
      INTEGER M
      INTEGER N
 
*  Arguments Given and Returned:
      DOUBLE PRECISION A( M, N )
      DOUBLE PRECISION B( M, N )
 
*  Status:
      INTEGER STATUS             ! Global status
 
*  Local variables:
      DOUBLE PRECISION IM                 ! Imaginary part of complex vector
      INTEGER J                  ! Pixel count
      INTEGER K                  ! Line count
      DOUBLE PRECISION MODUL              ! Modulus of complex vector
      DOUBLE PRECISION PHASE              ! Phase value
      DOUBLE PRECISION RL                 ! Real part of complex vector
 
*.
 
*  Check the inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN
 
*  Just do it a pixel at a time.
      DO K = 1, N
         DO J = 1, M
 
            MODUL = A( J, K )
            PHASE = B( J, K )
 
            RL = MODUL * COS( PHASE )
            IM = MODUL * SIN( PHASE )
 
            A( J, K ) = RL
            B( J, K ) = IM
 
         END DO
      END DO
 
      END
