      SUBROUTINE ASTIMP( STATUS )
*+
*  Name:
*     ASTIMP

*  Purpose:
*     Imports AST FrameSet information into NDFs.

*  Language:
*     Starlink Fortran 77

*  Type of Module:
*     ADAM A-task

*  Invocation:
*     CALL ASTIMP( STATUS )

*  Arguments:
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Description:
*     This task reads AST FrameSet information from a suitable file 
*     and uses it to modify the WCS components of the given NDFs.
*     A frame in a new domain is added (the same for each NDF) within
*     which a set of NDFs can be aligned.  This alignment may be used to 
*     assist FINDOFF in registering the images, or as a substititute
*     for performing registration by matching objects.  The newly 
*     added domain is made the Current domain of the WCS components.
*
*     AST files for use by this application are intended to be those 
*     written by the ASTEXP application, but with care frameset files
*     written from other applications or doctored by hand could be used.
*
*     If the WCS component of the NDF has a frame whose Domain has 
*     the same name as the Current frame of the frameset being imported,
*     it will be overwritten, and a warning message issued.

*  Usage:
*     ASTIMP in astfile

*  ADAM Parameters:
*     ASTFILE = LITERAL (Read)
*        A file containing a sequence of framesets describing the 
*        relative coordinate systems of NDFs from different sources.  
*
*        It is intended that this file should be one written by the 
*        ASTEXP application when a successful registration is made,
*        and the user need not be aware of its internal structure.
*        The files are readable text however, and can in principle be
*        written by other applications or doctored by hand, if this
*        is done with care, and with knowledge of AST objects (SUN/210).
*        The format of the file is explained in the Notes section.
*     IN = LITERAL (Read)
*        A list of NDF names whose WCS components are to be modified 
*        according to ASTFILE.  The NDF names may be specified using 
*        wildcards, or may be specified using an indirection file 
*        (the indirection character is "^").
*     LOGFILE = FILENAME (Read)
*        Name of the CCDPACK logfile.  If a null (!) value is given for
*        this parameter then no logfile will be written, regardless of
*        the value of the LOGTO parameter.
*
*        If the logging system has been initialised using CCDSETUP
*        then the value specified there will be used. Otherwise, the
*        default is "CCDPACK.LOG".
*        [CCDPACK.LOG]
*     LOGTO = LITERAL (Read)
*        Every CCDPACK application has the ability to log its output
*        for future reference as well as for display on the terminal.
*        This parameter controls this process, and may be set to any
*        unique abbreviation of the following:
*           -  TERMINAL  -- Send output to the terminal only
*           -  LOGFILE   -- Send output to the logfile only (see the
*                           LOGFILE parameter)
*           -  BOTH      -- Send output to both the terminal and the
*                           logfile
*           -  NEITHER   -- Produce no output at all
*
*        If the logging system has been initialised using CCDSETUP
*        then the value specified there will be used. Otherwise, the
*        default is "BOTH".
*        [BOTH]

*  Examples:
*     astimp data* camera.ast
*        This will apply the AST file "camera.ast" to all the NDFs in
*        the current directory with names beginning "data".  The file 
*        "camera.ast" has previously been written using REGISTER with
*        the parameter ASTFILE=camera.ast and a suitable FITSID 
*        parameter.
*
*     astimp astfile=instrum.ast in=! logto=terminal accept
*        This will simply report on the framesets contained within
*        the AST file "instrum.ast", writing the ID of each to the 
*        terminal only.

*  Notes:
*     ASTFILE Format:
*        The AST file consists of a sequence of framesets.  Each frameset
*        has an ID, and contains two frames (a Base frame and a Current
*        frame) and a mapping between them.  The domain of each Base
*        frame, and of each Current frame, is the same for each frameset.
*        The domain of the Base frame should be present in the WCS 
*        component of the NDFs to which the file is applied (probably 
*        PIXEL, but maybe SKY or some other value), and the domain of 
*        the Current frame is new to the WCS component (if a frame with 
*        the same domain exists it will be overwritten by the new one).
*
*        The ID of each frameset is used to determine, for each NDF,
*        which of the framesets in the file should be applied to it.
*        It must currently be of the form:
*
*           ID = "FITSID KEY VALUE"
*
*        which will match an NDF which contains the FITS header card
*
*           KEY     = VALUE
*
*        (VALUE must be surrounded by single quotes if it is of character
*        type).
*
*        Extensive error checking of the AST file is not performed, so
*        that unhelpful modifications to the WCS components of the 
*        target NDFs may occur if it is not in accordance with these
*        requirements.

*  Behaviour of parameters:
*     All parameters retain their current value as default. The
*     "current" value is the value assigned on the last run of the
*     application. If the application has not been run then the
*     "intrinsic" defaults, as shown in the parameter help, apply.
*
*     Retaining parameter values has the advantage of allowing you to
*     define the default behaviour of the application but does mean
*     that additional care needs to be taken when using the application
*     on new datasets or after a break of sometime.  The intrinsic
*     default behaviour of the application may be restored by using the
*     RESET keyword on the command line.
*
*     Certain parameters (LOGTO and LOGFILE) have global values.
*     These global values will always take precedence, except when an 
*     assignment is made on the command line.  Global values may be set 
*     and reset using the CCDSETUP and CCDCLEAR commands.

*  Copyright:
*     Copyright (C) 1999 Central Laboratory of the Research Councils

*  Authors:
*     MBT: Mark Taylor (STARLINK)
*     {enter_new_authors_here}

*  History:
*     05-MAR-1999 (MBT):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'AST_PAR'          ! AST parameters
      INCLUDE 'FIO_PAR'          ! FIO parameters
      INCLUDE 'CCD1_PAR'         ! Private CCDPACK constants
      INCLUDE 'PAR_ERR'          ! PAR system error codes

*  Status:
      INTEGER STATUS             ! Global status

*  Local Constants:
      INTEGER MXFSET             ! Maximum framesets in one file
      PARAMETER( MXFSET = 100 )

*  Local Variables:
      CHARACTER * ( FIO__SZFNM ) ASTFIL ! Name of frameset file
      CHARACTER * ( CCD1__BLEN ) BUF ! Output buffer
      CHARACTER * ( AST__SZCHR ) DMBAS ! Domain of base frame
      CHARACTER * ( AST__SZCHR ) DMCUR ! Domain of current frame
      CHARACTER * ( AST__SZCHR ) DMBAS1 ! Reference domain of current frame
      CHARACTER * ( AST__SZCHR ) DMCUR1 ! Reference domain of current frame
      INTEGER FCHAN              ! AST pointer to frameset file channel
      INTEGER FDAST              ! FIO file descriptor for frameset file
      INTEGER FRCUR              ! AST pointer to Current frame of WCS frameset
      INTEGER FRMAT              ! AST pointer to Current frame of import frameset
      INTEGER FSET( MXFSET )     ! AST pointers to import framesets 
      INTEGER FSMAP              ! AST pointer to mapping frameset
      INTEGER FSMAT              ! AST pointer to matched frameset
      INTEGER I                  ! Loop variable
      INTEGER IAT                ! Position in string
      CHARACTER * ( CCD1__BLEN ) ID ! ID value of frameset
      INTEGER IFGRP              ! GRP identifier for frameset group
      INTEGER INDF               ! NDF identifier
      INTEGER INGRP              ! IRG identifier for NDF group
      INTEGER IWCS               ! AST pointer to WCS component of NDF
      INTEGER IX                 ! Index into frameset group
      INTEGER J                  ! Loop variable
      INTEGER JDUP               ! Index of duplicate frame
      INTEGER JFCUR              ! Index of current frame of import frameset
      INTEGER JMBAS              ! Index of base frame of import frameset
      INTEGER JWBAS              ! Index of base frame of WCS component
      INTEGER MAP                ! AST pointer to mapping frameset
      LOGICAL MATCH              ! Whether NDF matches frameset ID
      INTEGER NFSET              ! Number of framesets in group
      INTEGER NNDF               ! Number of NDFs

*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Start up the CCDPACK logging system.
      CALL CCD1_START( 'ASTIMP', STATUS )

*  Begin AST context.
      CALL AST_BEGIN( STATUS )

*  Initialise group to hold frameset ID values.
      CALL GRP_NEW( 'Framesets', IFGRP, STATUS )

*  Open AST frameset file and notify user.
      CALL FIO_ASSOC( 'ASTFILE', 'READ', 'LIST', 0, FDAST, STATUS )
      CALL FIO_FNAME( FDAST, ASTFIL, STATUS )
      CALL MSG_SETC( 'ASTFIL', ASTFIL )
      CALL CCD1_MSG( ' ', '  Framesets read from file ^ASTFIL:', 
     :               STATUS )
      IF ( STATUS .NE. SAI__OK ) GO TO 99

*  Create AST channel on open frameset file.
      CALL CCD1_ACHAN( FDAST, ' ', FCHAN, STATUS )

*  Print header for per-frameset information.
      BUF = ' '
      BUF( 6: ) = 'N'
      BUF( 11: ) = 'Base domain'
      BUF( 31: ) = 'Current domain'
      BUF( 51: ) = 'Frameset ID'
      CALL CCD1_MSG( ' ', BUF, STATUS )
      BUF( 6: ) = '--'
      BUF( 11: ) = '-----------'
      BUF( 31: ) = '--------------'
      BUF( 51: ) = '-----------'
      CALL CCD1_MSG( ' ', BUF, STATUS )

*  Read frameset objects one by one from file.
      DO I = 1, MXFSET

*  Read next object or exit loop
         FSET( I ) = AST_READ( FCHAN, STATUS )
         IF ( FSET( I ) .EQ. AST__NULL ) GO TO 1
         NFSET = I

*  Get object ID string and frame domain names, and output to the user.
         ID = AST_GETC( FSET( I ), 'ID', STATUS )
         CALL AST_INVERT( FSET( I ), STATUS )
         DMBAS = AST_GETC( FSET( I ), 'Domain', STATUS )
         CALL AST_INVERT( FSET( I ), STATUS )
         DMCUR = AST_GETC( FSET( I ), 'Domain', STATUS )

*  First time round, set up reference values for validation.
         IF ( I .EQ. 1 ) THEN
            DMCUR1 = DMCUR
            DMBAS1 = DMBAS
         END IF

*  Output basic information to the user.
         BUF = ' '
         CALL CHR_ITOC( I, BUF( 6: ), IAT )
         BUF( 11:28 ) = DMBAS
         BUF( 31:48 ) = DMCUR
         BUF( 51: ) = ID
         CALL CCD1_MSG( ' ', BUF, STATUS )

*  Check ID is unique; if it is enter it into the group.  If not, warn
*  the user.
         CALL GRP_INDEX( ID, IFGRP, 1, IX, STATUS )
         IF ( IX .EQ. 0 ) THEN
            CALL GRP_PUT( IFGRP, 1, ID, I, STATUS )
         ELSE
            CALL MSG_SETC( 'ID', ID )
            CALL CCD1_MSG( ' ', 
     :                     '  Warning: Duplicate frameset ID "^ID"', 
     :                     STATUS )
         END IF

*  Check Base and Current frame domains are the same as the reference
*  values.
         IF ( DMBAS .NE. DMBAS1 )
     :      CALL CCD1_MSG( ' ', 
     :'  Warning: Base frame does not match first one', STATUS )
         IF ( DMCUR .NE. DMCUR1 ) 
     :      CALL CCD1_MSG( ' ',
     :'  Warning: Current frame does not match first one', STATUS )
         

      END DO

*  Abort if the maximum number of framesets in the AST file is exceeded.
      STATUS = SAI__ERROR
      CALL CCD1_ERREP( 'ASTIMP_FSLIMIT', 
     :     'ASTIMP: Too many framesets in AST file', STATUS )
      GO TO 99

*  All framesets read in.
 1    CONTINUE

*  Annul frameset file and channel.
      CALL AST_ANNUL( FCHAN, STATUS )
      CALL FIO_ANNUL( FDAST, STATUS )

*  Begin NDF context.
      CALL NDF_BEGIN

*  Get group of NDFs to operate on.
      CALL CCD1_NDFGR( 'IN', 'UPDATE', INGRP, NNDF, STATUS )
      IF ( STATUS .EQ. PAR__NULL ) CALL ERR_ANNUL( STATUS )
      IF ( STATUS .NE. SAI__OK ) GO TO 99

*  Loop over NDFs
      DO I = 1, NNDF
         CALL IRG_NDFEX( INGRP, I, INDF, STATUS )

*  Output name of NDF.
         CALL CCD1_MSG( ' ', ' ', STATUS )
         CALL NDF_MSG( 'NDF', INDF )
         CALL CCD1_MSG( ' ', '  Processing NDF ^NDF', STATUS )
         
*  Go through list of IDs to see if any match this NDF.
         MATCH = .FALSE.
         DO J = 1, NFSET
            CALL GRP_GET( IFGRP, J, 1, ID, STATUS )
            CALL CCD1_NMID( INDF, ID, MATCH, STATUS )
            IF ( MATCH ) THEN
               FSMAT = FSET( J )
               GO TO 2
            END IF
         END DO
 2       CONTINUE

*  No matching frameset in file; inform user.
         IF ( .NOT. MATCH ) THEN
            CALL MSG_SETC( 'ASTFIL', ASTFIL )
            CALL CCD1_MSG( ' ',  
     :                     '    No matching frameset in file ^ASTFIL', 
     :                     STATUS )
            
*  Matching frameset was found; incorporate the new frame information 
*  into the WCS component of the NDF.
         ELSE

*  Inform user match has occurred.
            CALL MSG_SETC( 'ID', ID )
            CALL CCD1_MSG( ' ', '    Matched with frameset ID "^ID"',
     :                     STATUS )

*  Get WCS component from NDF.
            CALL CCD1_GTWCS( INDF, IWCS, STATUS )

*  Check whether the WCS component has any frames with the same domain as
*  the Current frame of the import frameset; if it does, remove them, 
*  since we have to add another one of the same domain and we don't 
*  want multiple frames with the same domain in the WCS component.
            DMCUR = AST_GETC( FSMAT, 'Domain', STATUS )
 3          CONTINUE
            CALL CCD1_FRDM( IWCS, DMCUR, JDUP, STATUS )
            IF ( JDUP .NE. 0 ) THEN
               CALL MSG_SETC( 'DOM', DMCUR )
               CALL CCD1_MSG( ' ', 
     :'    A frame in domain "^DOM" already exists - removing',
     :                        STATUS )
               CALL AST_REMOVEFRAME( IWCS, JDUP, STATUS )
               GO TO 3
            END IF

*  Get a mapping between the current frame of the import frameset and 
*  the current frame of the WCS component.  The base frames get reset
*  by AST_CONVERT, so steps are taken to preserve them.
            JMBAS = AST_GETI( FSMAT, 'Base', STATUS )
            JWBAS = AST_GETI( IWCS, 'Base', STATUS )
            FSMAP = AST_CONVERT( IWCS, FSMAT, ' ', STATUS )
            MAP = AST_GETMAPPING( FSMAP, AST__BASE, AST__CURRENT, 
     :                            STATUS )
            CALL AST_SETI( IWCS, 'Base', JWBAS, STATUS )
            CALL AST_SETI( FSMAT, 'Base', JMBAS, STATUS )

*  Error occurred - abort.
            IF ( STATUS .NE. SAI__OK ) THEN
               GO TO 99

*  Mapping failed - inform user.
            ELSE IF ( MAP .EQ. AST__NULL ) THEN
               CALL MSG_SETC( 'DOM1', 
     :                        AST_GETC( IWCS, 'Domain', STATUS ) )
               CALL MSG_SETC( 'DOM2',
     :                        AST_GETC( FSMAT, 'Domain', STATUS ) )
               CALL CCD1_MSG( ' ', 
     :         '    Conversion from domain ^DOM1 to ^DOM2 failed', 
     :                        STATUS )

*  Mapping succeeded - add the new frame and mapping to the WCS component,
*  and write it back to the NDF.  The new frame becomes the Current frame 
*  of the WCS frameset.
            ELSE
               FRMAT = AST_GETFRAME( FSMAT, AST__CURRENT, STATUS )
               CALL AST_ADDFRAME( IWCS, AST__CURRENT, MAP, FRMAT, 
     :                            STATUS )
               CALL NDF_PTWCS( IWCS, INDF, STATUS ) 
               CALL MSG_SETC( 'DOM', 
     :                        AST_GETC( FSMAT, 'Domain', STATUS ) )
               CALL CCD1_MSG( ' ',
     :         '    New frame in domain "^DOM" added', STATUS )
            END IF

         END IF

      END DO

*  Exit with error label.  Tidy up after this.
 99   CONTINUE

*  End NDF context.
      CALL NDF_END( STATUS )

*  Close IRH.
      CALL IRH_CLOSE( STATUS )

*  Delete frameset group.
      CALL GRP_DELET( IFGRP, STATUS )

*  End AST context.
      CALL AST_END( STATUS )

*  If an error occurred, then report a contextual message.
      IF ( STATUS .NE. SAI__OK ) THEN
          CALL CCD1_ERREP( 'ASTIMP_ERR',
     :                     'ASTIMP: Error applying AST file.', STATUS )
      END IF

*  Close down logging system.
      CALL CCD1_END( STATUS ) 

      END
* $Id$
