      SUBROUTINE FTSIZE( INSIZ, OUTSIZ, STATUS )
*+
*  Name:
*     FTSIZE

*  Purpose:
*     Calculates a dimension of the Fourier transform.

*  Language:
*     Starlink Fortran 77

*  Invocation
*     CALL FTSIZE( INSIZ, OUTSIZ, STATUS )

*  Description:
*     Calculates the size of an array dimension which can be used by
*     the Fourier-transform routines. If the input value is acceptable
*     then it is returned as the output value. Otherwise the next
*     largest acceptable size is returned.

*  Arguments:
*     INSIZ = INTEGER (Given)
*        The input axis size, i.e. the dimension of the input array
*        to be transformed elsewhere.
*     OUTSIZ = INTEGER (Returned)
*        The output axis size.
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Algorithm:
*     - The current FFT routines accept any image size, so just return the 
*       supplied image size.

*  Authors:
*     DSB: D.S. Berry (STARLINK)
*     MJC: Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}

*  History:
*     1988 Apr 20 (DSB):
*        Original version.
*     1990 Mar 14 (MJC):
*        Converted to KAPPA/ADAM.
*     17-FEB-1995 (DSB):
*        Converted from NAG to FFTPACK.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Arguments Given:
      INTEGER
     :  INSIZ

*  Arguments Returned:
      INTEGER
     :  OUTSIZ

*  Status:
      INTEGER STATUS             ! Global status

*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Return the supplied dimension.
      OUTSIZ = INSIZ

      END
