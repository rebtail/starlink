      SUBROUTINE KPG1_LASTO( STRING, CVAL, IAT, STATUS )
*+
*  Name:
*     KPG1_LASTO

*  Purpose:
*     Locates the last occurrence of CVAL in STRING.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL KPG1_LASTO( STRING, CVAL, IAT, STATUS )

*  Description:
*     The routine locates the last occurence of the single character
*     CVAL in STRING. If an occurence is not located then IAT is
*     returned as 0.

*  Arguments:
*     STRING = CHARACTER * ( * ) (Given)
*        String to be searched for occurences of CVAL.
*     CVAL = CHARACTER * ( 1 ) (Given)
*        Character whose last occurence is to be located.
*     IAT = INTEGER (Returned)
*        Position within STRING at which the last occurence of CVAL is
*        located.
*     STATUS = INTEGER (Given and Returned)
*        The global status.   

*  Authors:
*     PDRAPER: Peter Draper (STARLINK)
*     MJC: Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}

*  History:
*     28-FEB-1992 (PDRAPER):
*        Original version.
*     1997 May 19 (MJC):
*        Rebadged and edited for KAPPA.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
      
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Arguments Given:
      CHARACTER * ( * ) STRING
      CHARACTER * ( 1 ) CVAL

*  Arguments Returned:
      INTEGER IAT

*  Status:
      INTEGER STATUS             ! Global status

*  Local Variables:
      INTEGER STRLEN             ! Length of STRING
      INTEGER WASAT              ! Previous position within STRING
      INTEGER NOWAT              ! Current position within STRING
      LOGICAL MORE               ! Flag for more loops required

*.

*  Check the inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Get the length of the string.
      STRLEN = LEN ( STRING )

*  Initialise the string positions.
      NOWAT = 0
      WASAT = 1

*  Initialise the loop flag.
      MORE = .TRUE. 

*  Loop while occurrences are still located and the string length is not
*  exceeded.
   10 CONTINUE                   ! Start of 'DO WHILE' loop
      IF ( MORE ) THEN
         NOWAT = INDEX( STRING( WASAT : ) , CVAL )
         IF ( NOWAT .EQ. 0 ) THEN

*  There are no more occurrences.
            MORE = .FALSE.
            IAT = WASAT - 1
         ELSE

*  There is more to do.  Increment the position within STRING.
            WASAT = NOWAT + WASAT 

*  If WASAT now exceeds the string length, the last occurrence was at
*  the end of the string.
            IF ( WASAT .GT. STRLEN ) THEN
               MORE = .FALSE.
               IAT = STRLEN
            END IF
         END IF

         GO TO 10
      END IF     

      END
