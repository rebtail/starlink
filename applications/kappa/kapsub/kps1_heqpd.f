      SUBROUTINE KPS1_HEQPD( BAD, EL, ARRAY, RSHADE, MAXV, MINV, NINTS,
     :                         PENS, X, Y, CUMUL, HIST, STATUS )
*+
*  Name:
*     KPS1_HEQPx
 
*  Purpose:
*     Allocates colours to pens by histogram equalisation of an array.
 
*  Language:
*     Starlink Fortran 77
 
*  Invocation:
*     CALL KPS1_HEQPx( BAD, EL, ARRAY, RSHADE, MAXV, MINV, NINTS, PENS,
*                      X, Y, CUMUL, HIST, STATUS )
 
*  Description:
*     An histogram of the array is produced, and is used to assign
*     colours to the pens in such a way that the resultant image should
*     contain as near a given distribution of colour as possible.  The
*     only available distribution is a linear one.  The gradient of the
*     amount of the colour against the colour is defined by the user,
*     and must lie in the range -1 to 1.  Assuming for the moment that
*     a grey scale is being used as the colour set, a gradient of 0
*     should give an even amount of each shade, a gradient of -1 will
*     give a dark image, and a gradient of +1 will give a rather white
*     image.
 
*  Arguments:
*     BAD = LOGICAL (Given)
*        If .TRUE., bad pixels will be processed.  This should not be
*        set to false unless the input array contains no bad pixels.
*     EL = INTEGER (Given)
*        The dimension of the array.
*     ARRAY( EL ) = ? (Given)
*        The array containing the scaled image.
*     RSHADE = REAL (Given)
*        Gradient of the amount of colour against colour in the range
*        -1 to 1.  In other words the weighting of the histogram
*        equalisation.  -1 gives a dark picture, 1 a light picture and a
*        true equalisation, i.e. even amounts of each shade.
*     MAXV = ? (Given)
*        Maximum value for scaling of the array.
*     MINV = ? (Given)
*        Minimum value for scaling of the array.
*     NINTS = INTEGER (Given)
*        The number of greyscale intensities available on the chosen
*        device and number of bins in histogram.
*     PENS( 0:NINTS-1 ) = INTEGER (Returned)
*        The array in which is returned the colour to be assigned to
*        each pen.
*     X( 0:NINTS-1 ) = DOUBLE PRECISION (Returned)
*        Work array for normalised pen numbers in the usable range,
*        range, used for the least-squares fit.
*     Y( 0:NINTS-1 ) = DOUBLE PRECISION (Returned)
*        Work array for normalised value of the cumulative-frequency
*        chart.
*     CUMUL( 0:NINTS-1 ) = INTEGER (Returned)
*        Work array for the discrete cumulative-frequency chart.
*     HIST( 0:NINTS-1 ) = INTEGER (Returned)
*        Work array for the histogram.
*     STATUS = INTEGER (Given and Returned)
*        Global status parameter.
 
*  Notes:
*     -  There is a routine for real and double-prcision data types:
*     replace "x" in the routine name by R or D as appropriate.  The
*     arguments ARRAY, MAXV, and MINV must have the data type specified.
 
*  Algorithm:
*     The histogram of the image between the scaling limits is formed.
*     Let the probability of a particular pen, r, being used be P(r),
*     and the probability of a particular colours being used be Q(s).
*     We require Q(s) = a + bs.  Let the function linking the pens with
*     the colours be r = T(s).  From probability theory we have that
*     P(r) = Q(s) * ds/dr.  Thus P(r)dr = Q(s)ds.  However, the integral
*     of P(r)dr is just the cumulative frequency value for a pen, say
*     C(r).  Hence we have that C(r) = as + bs * s/2 + c.  However, when
*     r=0, s=0.  Thus c = C( 0 ).  If the pens lie in the range 0 to n
*     and the colours in the range 0 to m, then when r = n, s = m and
*     C(n) = 1.  Hence 1 = am + bm * m/2 + C(0), from which we obtain
*     a = (1 - C(0))/m - bm/2.  We now have s as a unique function of r:
*     s = -a/b - sqrt( a * a + 2b (C(r) - C(0)) )/b.  The function C(r)
*     is approximated by fitting a sixth-order polynomial to the
*     discrete cumulative frquency obtained from the histogram. The
*     polynomial is generated by using least squares.  The solution of
*     the equations for the coefficients is done using Gaussian
*     elimination with partial pivoting.
 
*     For a much more detailed description of histogram equalisation
*     see 'Digital Image Processing' by R.C.Gonzalez and P.Wintz.
 
*  Authors:
*     KFH: K.F.Hartley (RGO)
*     APH: A.P.Horsfield (RGO)
*     SC: S.Chan (RGO)
*     MJC:Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}
 
*  History:
*     28 July 1983 (KFH):
*        Original based on ADHC written by P.T.Wallace, K.F.Hartley and
*        W.F.Lupton.
*     1986 Sep 18 (MJC):
*        Completely revamped; renamed from KFH_HIST; standardised to
*        RAPI2D style; renamed parameters section to arguments and
*        added access; made more general by making the number of
*        bins/colour indices an argument, and also four associated work
*        arrays; removed PAR calls - parameters now supplied as
*        arguments; removed tabs; relocated 'local' variables to import
*        etc.; corrected error-handling and tidied.
*     1988 June 22 (MJC):
*        Added identification to error reporting.
*     1989 August 7 (MJC):
*        Passed array dimensions as separate variables.
*     1990 April 7 (MJC):
*        Constrained the fit to lie in the defined bounds so that
*        evaluating the root of the quadratic equation does not result
*        in an undefined value, and improved the commentary.
*     1995 May 1 (MJC):
*        Made generic and renamed from HSTEQP with a revised calling
*        sequence.  Processes a vector.  Add BAD argument.  Used
*        modern-style commenting and prologue.
*     {enter_further_changes_here}
 
*  Bugs:
*     {note_any_bugs_here}
 
*-
 
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing
 
*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Global SSE definitions
      INCLUDE 'PRM_PAR'          ! PRIMDAT public constants
 
*  Arguments Given:
      LOGICAL BAD
      INTEGER EL
      DOUBLE PRECISION ARRAY( EL )
      INTEGER NINTS
      REAL RSHADE
      DOUBLE PRECISION MAXV
      DOUBLE PRECISION MINV
 
*  Arguments Returned:
      INTEGER PENS( 0:NINTS-1 )
      DOUBLE PRECISION X( 0:NINTS-1 )
      DOUBLE PRECISION Y( 0:NINTS-1 )
      INTEGER CUMUL( 0:NINTS-1 )
      INTEGER HIST( 0:NINTS-1 )
 
*  Status:
      INTEGER STATUS             ! Global status
 
*  Local Variables:
      DOUBLE PRECISION A         ! Coefficient in the expression
                                 ! relating colour to pen number
      DOUBLE PRECISION C0        ! Minimum value of the calculated
                                 ! cumulative frequency chart
      DOUBLE PRECISION COEFF( 0:7, 0:6 ) ! Coefficients of the equations to be
                                 ! solved for least squares fit
      INTEGER I                  ! General variable
      INTEGER J                  ! General variable
      INTEGER K                  ! General variable
      INTEGER LOWER              ! Lowest part of the cumulative
                                 ! frequency chart to which the
                                 ! polynomial will be fitted.
      DOUBLE PRECISION MAXVAL    ! Maximum value of cofficient. Used for
                                 ! partial pivoting
      DOUBLE PRECISION POLCO( 0:6 ) ! Polynomial coefficients
      INTEGER POS                ! Row containing the largest coefficient
      DOUBLE PRECISION SHADE     ! Relative amounts of each colour wanted
      DOUBLE PRECISION STEP      ! Difference in pen value between
                                 ! successive pens
      DOUBLE PRECISION SUM( 0:12 ) ! sums of powers of pen numbers between
                                 ! 0 and 12
      DOUBLE PRECISION TEMP      ! General variable
      INTEGER UPPER              ! Largest part of the cumulative
                                 ! frequency chart to be used for the
                                 ! least-squares fit
      DOUBLE PRECISION XI        ! Pen number used for evaluating the
                                 ! colour from the polynomial
      DOUBLE PRECISION XJ        ! Pen number raised to some(?) power
      DOUBLE PRECISION XMAX      ! Pen number at the highest point used
                                 ! for fit
      DOUBLE PRECISION XMIN      ! Pen number at the lowest point used
                                 ! for fit
 
*  Internal References:
      INCLUDE 'NUM_DEC_CVT'      ! NUM declarations for conversions
      INCLUDE 'NUM_DEF_CVT'      ! NUM definitions for conversions
 
*.
 
*  Check the inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN
 
*  Report an error and return should the maximum and minimum be the
*  same.
      IF ( MAXV .EQ. MINV ) THEN
         STATUS = SAI__ERROR
         CALL ERR_REP( 'ERR_KPS1_HEQPx_EQL',
     :     'KPS1_HEQPx: Maximum and minimum are equal.', STATUS )
         GOTO 999
      END IF
 
      SHADE = DBLE( RSHADE )
 
*  Clear the histogram array.
      DO I = 0, NINTS-1, 1
         HIST( I ) = 0
      END DO
 
*  Create the histogram.
      CALL KPG1_GHSTD( BAD, EL, ARRAY, NINTS, MAXV, MINV, HIST,
     :                 STATUS )
 
*  Create the cumulative frequency chart.
      CUMUL( 0 ) = HIST( 0 )
 
      DO  I = 1, NINTS-1, 1
         CUMUL( I ) = CUMUL( I-1 ) + HIST( I )
      END DO
 
*  Set up data for least-squares fit.
      DO  I = 0, NINTS-1, 1
 
*  Save the pen numbers.
         X( I ) = NUM_DTOD( MINV ) + NUM_DTOD( ( MAXV-MINV ) ) *
     :            DBLE( I ) / DBLE( NINTS-1 )
 
*  Save the normalized cumulative-frequency chart.
         Y( I ) = DBLE( CUMUL( I ) ) / DBLE( CUMUL( NINTS-1 ) )
 
      END DO
 
*    The fit is done between the 2% and 96% points of the
*    cumulative frequency chart.
*
*    Find the 2% point (approximately).
      LOWER = 0
 
      DO WHILE ( Y( LOWER ) .LT. 0.02 )
         LOWER = LOWER + 1
      END DO
 
*  Find the 96% point (approximately).
      UPPER = NINTS - 1
 
      DO WHILE ( Y( UPPER ) .GT. 0.96 )
         UPPER = UPPER - 1
      END DO
 
*  If there are more than 20 points then do the fit.
      IF ( UPPER - LOWER .LT. 20 ) THEN
         STATUS = SAI__ERROR
         CALL MSG_SETI( 'FITPOINTS', UPPER-LOWER )
         CALL ERR_REP( 'ERR_KPS1_HEQPx_ISFP',
     :     'KPS1_HEQPx: Insufficient points (^FITPOINTS) to do the '/
     :     /'fit.  Choose a smaller dynamic range.', STATUS )
         GOTO 999
      END IF
 
*  For maximum accuracy normalize the pen numbers to the range 0 to 1.
      XMIN = X( LOWER )
      XMAX = X( UPPER )
 
      DO  I = LOWER, UPPER, 1
         X( I ) = ( X( I ) - XMIN ) / ( XMAX - XMIN )
      END DO
 
*  Set to zero the tables into which the sums will be put.
      DO  I = 0, 12, 1
         SUM( I ) = 0.0D0
      END DO
 
      DO  I = 0, 6, 1
         COEFF( 7, I ) = 0.0D0
      END DO
 
*  Form the sums of the powers of x (the pen numbers).
      DO  I = LOWER, UPPER, 1
         XJ = 1.0D0
 
         DO  J = 0, 12, 1
 
            SUM( J ) = SUM( J )+XJ
            IF ( J .LT. 7 ) COEFF( 7, J ) = COEFF( 7, J ) - Y( I ) * XJ
            XJ = XJ * X( I )
 
         END DO
      END DO
 
*  Transfer the sums to the coefficients array ready for solving the
*  linear simultaneous equations to obtain the coefficients of the
*  polynomial.
      DO  I = 0, 6, 1
         DO  J = 0, 6, 1
            COEFF( J, I ) = SUM( J + I )
         END DO
      END DO
 
*  Do Gaussian elimination to calculate the polynomial coefficients.
      I = 0
 
      DO WHILE ( I .LT. 7 )
 
*  Find the maximum value in the column.
         MAXVAL = ABS( COEFF( I, I ) )
         POS = I
 
         DO J = I, 6, 1
            IF ( ABS( COEFF( I, J ) ) .GT. MAXVAL ) THEN
               MAXVAL = ABS( COEFF( I, J ) )
               POS = J
            END IF
         END DO
 
*  If the maximum value is greater than 1E-10 then do the operations on
*  the row, otherwise stop the routine, as the results are going to be
*  very inaccurate.
         IF ( MAXVAL .LT. 1.0D-10 ) THEN
            STATUS = SAI__ERROR
            CALL ERR_REP( 'ERR_KPS1_HEQPx_INAF',
     :        'KPS1_HEQPx: Inaccurate polynomial fit for equalising '/
     :        /'the pens.', STATUS )
            GOTO 999
         END IF
 
*  Swap the present row with that containing the largest value.
         IF ( I .NE. POS ) THEN
            DO  J = 0, 7, 1
               TEMP = COEFF( J, I )
               COEFF( J, I ) = COEFF( J, POS )
               COEFF( J, POS ) = TEMP
            END DO
         END IF
 
*  Eliminate the first I-1 variables.
         IF ( I .NE. 0 ) THEN
            DO  J = 0, I - 1, 1
               DO  K = 7, J, -1
                  COEFF( K, I ) = COEFF( K, I ) - COEFF( K, J ) *
     :                            COEFF( J, I )
               END DO
            END DO
         END IF
 
*  Divide throuought the Ith row by the Ith element, so that the Ith
*  element becomes one.
         DO J = 7, I, -1
            COEFF( J, I ) = COEFF( J, I ) / COEFF( I, I )
         END DO
 
*  Point to the next row.
         I = I + 1
      END DO
 
*  Substitute back to obtain the coefficients.
      POLCO( 6 ) = -COEFF( 7, 6 )
 
      DO I = 5, 0, -1
         POLCO( I ) = -COEFF( 7, I )
 
         DO  J = 6, I+1, -1
            POLCO( I ) = POLCO( I ) - COEFF( J, I ) * POLCO( J )
         END DO
 
      END DO
 
*  Assign colours to the pens.
      STEP = NUM_DTOD( MAXV - MINV )/( ( XMAX - XMIN ) *
     :       DBLE( NINTS - 1 ) )
 
      XI = 0.0D0
 
*  Calculate the minimum value for the cumulative frequency chart.
      C0 = POLCO( 0 )
 
*  Even distribution.
      IF ( ABS( SHADE ) .LT. VAL__EPSD ) THEN
 
*  Assign colours to the pens in such a way that they are
*  used in equal amounts in the displayed image.
         DO I = 0, NINTS-1, 1
 
*  Constrain the fit beyond the 2- and 96-per-cent limits.
            IF ( I .LT. LOWER ) THEN
               TEMP = 0.0D0
            ELSEIF ( I .GT. UPPER ) THEN
               TEMP = 1.0D0 - C0
            ELSE
 
*  Evaluate the polynomial fit to the normalised cumulative histogram,
*  allowing for the offset.
               TEMP = ( POLCO( 0 ) + XI * ( POLCO( 1 ) + XI *
     :                ( POLCO( 2 ) + XI * ( POLCO( 3 ) + XI *
     :                ( POLCO( 4 ) + XI * ( POLCO( 5 ) + XI *
     :                ( POLCO( 6 ) ) ) ) ) ) ) ) - C0
 
*  Move to the next data value.
               XI = XI + STEP
            END IF
 
*  Assign colours to the pen numbers.
            PENS( I ) = MIN( MAX( INT( DBLE( NINTS ) * TEMP ), 0 ),
     :                  NINTS - 1 )
         END DO
 
      ELSE
 
*  Assign colours to the pens such that the distribution required by
*  the user is obtained.
         A = 1.0D0 - C0 - SHADE / 2.0
 
         DO  I = 0, NINTS - 1, 1
 
*  Constrain the fit beyond the 2- and 96-per-cent limits.
            IF ( I .LT. LOWER ) THEN
               TEMP = 0.0D0
            ELSEIF ( I .GT. UPPER ) THEN
               TEMP = 1.0D0 - C0
            ELSE
 
*  Evaluate the polynomial fit to the normalised cumulative histogram,
*  allowing for the offset.
               TEMP = ( POLCO( 0 ) + XI * ( POLCO( 1 ) + XI *
     :                ( POLCO( 2 ) + XI * ( POLCO( 3 ) + XI *
     :                ( POLCO( 4 ) + XI * ( POLCO( 5 ) + XI *
     :                ( POLCO( 6 ) ) ) ) ) ) ) ) - C0
 
*  Ensure that the fitted value lies in the physical region.
               TEMP = MIN( MAX ( 0.0D0, TEMP ), 1.0D0 - C0 )
 
*  Move to the next data value.
               XI = XI + STEP
            END IF
 
*  Assign the colours to the pens.  Solve the quadratic equation that
*  arises from integrating the required cumulative histogram form, as
*  given by the linear equation SHADE * PEN + C0 - SHADE/2.
            PENS( I ) = MIN( MAX( INT( DBLE( NINTS ) * ( -A/SHADE
     :                  + SQRT( A * A + 2.0D0 * SHADE * TEMP ) /
     :                  SHADE ) ), 0 ), NINTS - 1 )
         END DO
 
      END IF
 
 999  CONTINUE
 
      END
