      SUBROUTINE KPS1_AGNCV( X1, Y1, XW, YW, STATUS )
*+ 
*  Name:
*     KPS1_AGNCV
 
*  Purpose:
*     Displays text telling the user what the latest value is for the
*     cursor position in ARDGEN.
 
*  Language:
*     Starlink Fortran 77
 
*  Invocation:
*     CALL KPS1_AGNCV( X1, Y1, XW, YW, STATUS )
 
*  Description:
*     The routine displays the latest value for the cursor position.
*     When an image is being displayed output is in the form of pixel
*     co-ordinates.
 
*  Arguments:
*     X1 = REAL (Given)
*        X pixel co-ordinate of the left-hand edge of the image.
*     Y1 = REAL (Given)
*        Y pixel co-ordinate of the bottom edge of the image.
*     XW = REAL (Given)
*        X pixel co-ordinate.
*     YW = REAL (Given)
*        Y pixel co-ordinate.
*     STATUS = INTEGER (Given and Returned)
*        The global status.
 
*  Authors:
*    GJP: Grant Privett (STARLINK)
*    DSB: David Berry (STARLINK)
*    MJC: Malcolm J. Currie (STARLINK)
*    {enter_new_authors_here}
 
*  History:
*     15-Mar-1993 (GJP)
*        Original version
*     5-DEC-1994 (DSB)
*        Tidied up.  Name changed from ARDG1_CURVD to KPS1_AGNCV.
*        Layout of displayed text modified.
*     1995 March 15 (MJC):
*        Shortened long lines, corrected typo's, and prologue
*        indentation.  Removed the "data" co-ordinates which were
*        `relative' to the picture origin to avoid confusion with
*        genuine data or axis co-ordinates.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}
 
*- 
 
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing
 
*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
 
*  Arguments Given:
      REAL XW                    ! X pixel co-ordinate
      REAL X1                    ! X pixel co-ordinate of the image
                                 ! edge
      REAL YW                    ! Y pixel co-ordinate
      REAL Y1                    ! Y pixel co-ordinate of the image
                                 ! bottom
 
*  Status:
      INTEGER STATUS             ! Global status
 
*.
 
*  Check the inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN
 
      CALL MSG_BLANK( STATUS )
      CALL MSG_OUT( 'KPS1_AGNCV_MSG1', 'Cursor position...', STATUS )

*  Put the pixel co-ordinates into message tokens.
      CALL MSG_SETR( 'XVALW', XW )
      CALL MSG_SETR( 'YVALW', YW )
 
*  Display the current X and Y values.
      CALL MSG_OUT( 'KPS1_AGNCV_MSG3', '   Pixel co-ordinates: '/
     :              /'(^XVALW, ^YVALW)', STATUS )
      CALL MSG_BLANK( STATUS )
 
*  The following call achieves graphics/text synchronisation.
      CALL MSG_SYNC( STATUS )
 
      END
