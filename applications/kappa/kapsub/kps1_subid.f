      SUBROUTINE KPS1_SUBID( DIM1, DIM2, ARRAY, BINX, BINY, ESTIMA,
     :                         NCLIP, CLIP, THRLO, THRHI, FRAC, MAXBIN,
     :                         BIN, TBIN, X, Y, Z, W, NBIN, STATUS )
 
*+
*  Name:
*     KPS1_SUBIx
 
*  Purpose:
*     Bins a 2-d array into rectangular bins via a variety of estimators
*     and thresholding.
 
*  Language:
*     Starlink Fortran 77
 
*  Invocation:
*     CALL KPS1_SUBIx( DIM1, DIM2, ARRAY, BINX, BINY, ESTIMA, NCLIP,
*                      CLIP, THRLO, THRHI, FRAC, MAXBIN, BIN, TBIN, X,
*                      Y, Z, W, NBIN, STATUS )
 
*  Description:
*     This routine bins a 2-dimensional array into a grid rectangular
*     sections spanning the whole array.  For each bin an estimator is
*     calculated from the non-bad array pixels to represent a global
*     value for the bin.  The estimators are the mean, mean after
*     standard-deviation clipping, the median and the mode.  Weights
*     are derived for the estimator, depending on the estimator
*     selected.  The mean and clipped mean use the standard deviation,
*     and the median and mode take the square root of the number of
*     contributing pixels.  Also computed is the average position of
*     the valid pixels for each bin.  Prior to determining the
*     estimator, each bin may be thresholded to restrict analysis to a
*     defined range.  Bins are ignored if there was an error deriving
*     the value, or when there are insufficient pixels in the bin to
*     compute a value, or when the fraction of good pixels is too
*     small.
*
*  Arguments:
*     DIM1 = INTEGER (Given)
*        The first dimension of the 2-d array.
*     DIM2 = INTEGER (Given)
*        The second dimension of the 2-d array.
*     ARRAY( DIM1, DIM2 ) = REAL (Given)
*        The input data array which is to be binned.
*     BINX = INTEGER (Given)
*        The number of pixels in a bin in the x direction
*     BINY = INTEGER (Given)
*        The number of pixels in a bin in the y direction
*     ESTIMA = CHARACTER (Given)
*        The estimator for the bin.  It must one of the following
*        values:- 'MEAN' for the mean value, 'KSIG' for the mean
*        with kappa-sigma clipping; 'MODE' for the mode, and 'MEDI'
*        for the median. If it is none of these, the mode is used.
*     NCLIP = INTEGER (Given)
*        The number of clipping cycles. This is only needed if %ESTIMA
*        is 'KSIG'.
*     CLIP( * ) = REAL (Given)
*        The array of standard deviation thresholds to define the
*        progressive clipping of the distribution.  Thus a value of
*        3.0 would eliminate points outside the range mean-3.*sigma
*        to mean+3.*sigma. This is only needed if %ESTIMA is 'KSIG'.
*     THRLO = REAL (Given)
*        Lower threshold below which values will be excluded from the
*        analysis to derive representative values for the bins.  If
*        it equals the undefined (bad) value, there will be no lower
*        threshold.
*     THRHI = REAL (Given)
*        Upper threshold above which values will be excluded from the
*        analysis to derive representative values for the bins.  If
*        it equals the undefined (bad) value, there will be no upper
*        threshold.
*     FRAC = REAL  (Given)
*        The minimum fraction of good pixels in a bin that permits
*        the bin to be included in the fit.  Here good pixels are
*        ones that participated in the calculation of the bin's
*        representative value. So they exclude both bad pixels and
*        ones rejected during estimation (e.g. ones beyond the
*        thresholds or were clipped).
*     MAXBIN = INTEGER (Given)
*        The maximum dimension of the data, weight and co-ordinate
*        vectors.
*     BIN( BINX, BINY ) = REAL (Returned)
*        A work array to store a single bin.
*     TBIN( * ) = REAL (Returned)
*        A work array to store a single bin after thresholding. If
*        there is no thresholding to be done, this can be a
*        single-element array, otherwise it should have %BINX * %BINY
*        elements.
*     X( MAXBIN ) = ? (Returned)
*        The mean x positions for each bin.
*     Y( MAXBIN ) = ? (Returned)
*        The mean y positions for each bin.
*     Z( MAXBIN ) = ? (Returned)
*        The global value as measured by the requested estimator for
*        each bin.
*     W( MAXBIN ) = ? (Returned)
*        The weight for each bin.
*     NBIN = INTEGER (Returned)
*        Number of bins with defined statistics.
*     STATUS  =  INTEGER (Given and Returned)
*        Global status value.
 
*  Notes:
*     -  There is a routine for double precision or real data types:
*     replace "x" in the routine name by D or R as appropriate.  The
*     X, Y, Z, and W (but not ARRAY) arguments supplied to the routine
*     must have the data type specified.
*     -  Uses the magic-value method for bad or undefined pixels.
 
*  Algorithm:
*       - Scan through the bins.
*       - Initialise the bin.
*       - Get the minimum number of pixels for a valid bin value.
*       - Extract the bin from the data array.
*       - Threshold the values in the bin if r.equired
*       - Compute the required statistic.
*       - Given sufficient points and a good statistic, store the
*         result and weight (sqrt no. of points for mode and median,
*         and standard deviation for the mean).
*       - Find average x-y positions of the bin of the valid pixels.
*
*  Authors:
*     MJC: Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}
 
*  History:
*     1990 Jan 24 (MJC):
*        Original version.
*     1993 February 17 (MJC):
*        Uses a new routine to derive medians for bins containing few
*        pixels.
*     1996 October 15 (MJC):
*        Renamed from BINST, made generic, and reordered ARRAY argument.
*     {enter_further_changes_here}
 
*  Bugs:
*     {note_new_bugs_here}
 
*-
 
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing
 
*  Global Constants:
      INCLUDE 'SAE_PAR'          ! SSE global definitions
      INCLUDE 'PRM_PAR'          ! Magic-value definitions
 
*  Arguments Given:
      INTEGER DIM1, DIM2
      REAL ARRAY( DIM1, DIM2 )
      INTEGER BINX, BINY
      CHARACTER * ( * ) ESTIMA
      INTEGER NCLIP
      REAL CLIP( * )
      REAL THRLO
      REAL THRHI
      REAL FRAC
      INTEGER MAXBIN
 
*  Arguments Returned:
      REAL BIN( BINX, BINY )
      REAL TBIN( * )
      DOUBLE PRECISION X( MAXBIN )
      DOUBLE PRECISION Y( MAXBIN )
      DOUBLE PRECISION Z( MAXBIN )
      DOUBLE PRECISION W( MAXBIN )
      INTEGER NBIN
 
*  Status:
      INTEGER STATUS             ! Global status
 
*  Local Variables:
      INTEGER BINTOT             ! Number of pixels in the current bin
      INTEGER BX, BY             ! The number of pixels in the x-y
                                 ! directions in the current bin (allows
                                 ! edge effects)
      CHARACTER * ( 4 ) ESTIM    ! Uppercase version of the estimator
      LOGICAL HITHRS             ! Upper threshold to apply?
      INTEGER I                  ! Loop counter
      INTEGER IX, IY             ! Counters used to sum the x-y pixel
                                 ! positions within the bin
      INTEGER J                  ! Loop counter
      INTEGER K                  ! Loop counter
      INTEGER L                  ! Loop counter
      LOGICAL LOTHRS             ! Lower threshold to apply?
      INTEGER MAXPCL             ! Index of maximum value after clipping
      INTEGER MAXPOS             ! Index of maximum value
      REAL MAXMUM                ! Maximum value in bin (not used)
      REAL MAXCL                 ! Clipped maximum value (not used)
      REAL MEAN                  ! Mean value of the bin
      REAL MEANCL                ! Clipped mean value of the bin
      REAL MEDIAN                ! Median value of the bin
      REAL MINMUM                ! Mimimum value in bin (not used)
      REAL MINCL                 ! Clipped minimum value (not used)
      INTEGER MINPCL             ! Index of minimum value after clipping
      INTEGER MINPOS             ! Index of minimum value
      REAL MODE                  ! Mode value of the bin
      INTEGER NGMIN              ! Minimum number of good pixels
                                 ! permitted
      INTEGER NINVAL             ! Number of bad pixels in the bin
      LOGICAL NOSTAT             ! No statistics were obtained due to
                                 ! too few points?
      INTEGER NPIXCL             ! Number of values used to compute the
                                 ! clipped mean
      INTEGER NPT                ! Number of valid pixels in the bin
      INTEGER NREPLO             ! Number of pixels below low threshold
      INTEGER NREPHI             ! Number of pixels below high threshold
      INTEGER NSAM               ! Number of values used to compute the
                                 ! median or mode
      LOGICAL ORDERD             ! Mode or median estimator selected?
      REAL SKEW                  ! Skewness of the bin values
      REAL STDDEV                ! Standard deviation of the bin values
      REAL STDVCL                ! Standard deviation of the bin values
                                 ! after clipping
      REAL TOTAL                 ! Total value of the bin (not used)
      REAL TOTCL                 ! Clipped total value (not used)
      INTEGER XMIN, YMIN         ! Pixel numbers of the lower-bound
                                 ! pixel in the current bin
      INTEGER XMAX, YMAX         ! Pixel numbers of the upper-bound
                                 ! pixel in the current bin
 
*  Internal References:
      INCLUDE 'NUM_DEC_CVT'      ! NUM declarations for conversions
      INCLUDE 'NUM_DEF_CVT'      ! NUM definitions for conversions
 
*.
 
*  Check the inherited global status.
      IF ( STATUS .NE. SAI__OK ) GOTO 999
 
*  Set up some useful variables.  First to do with the statistic to be
*  obtained for each bin, and second the thresholding to be applied.
      ESTIM = ESTIMA
      CALL CHR_UCASE( ESTIM )
      IF ( ESTIM .NE. 'MODE' .AND. ESTIM .NE. 'MEDI' .AND.
     :     ESTIM .NE. 'MEAN' .AND. ESTIM .NE. 'KSIG' ) ESTIM = 'MODE'
 
      ORDERD = ESTIM .EQ. 'MODE' .OR. ESTIM .EQ. 'MEDI'
 
      LOTHRS = THRLO .NE. VAL__BADR
      HITHRS = THRHI .NE. VAL__BADR
 
*  Initialise counter for number of bins containing valid pixels.
      NBIN = 0
 
*  Scan through bins, calculating the minimum and maximum x and y
*  co-ordinates.
       DO YMIN = 1, DIM2, BINY
         YMAX = MIN( YMIN + BINY - 1, DIM2 )
 
         DO XMIN = 1, DIM1, BINX
            XMAX = MIN( XMIN + BINX - 1, DIM1 )
 
            BX = XMAX - XMIN + 1
            BY = YMAX - YMIN + 1
            BINTOT = BX * BY
 
*  Initialise the bin, since not all bins are the same size.  Otherwise
*  data from earlier and larger bins may be included.
            IF ( BX .LT. BINX .OR. BY .LT. BINY ) THEN
               DO  L = BY, BINY
                  DO  K = BX, BINX
                     BIN( K, L ) = VAL__BADR
                  END DO
               END DO
            END IF
 
*  Get the minimum number of pixels that can be used to derive the bin
*  value for the value to be valid.
            NGMIN = MAX( 1, MIN( BINTOT,
     :              INT( REAL( BINTOT ) * FRAC ) + 1 ) )
 
*  Extract the rectangular section of the array.
            CALL CPSECR( ARRAY, DIM1, DIM2, XMIN, YMIN, XMAX, YMAX,
     :                   BX, BY, BIN, STATUS )
 
            IF ( STATUS .NE. SAI__OK ) THEN
               CALL MSG_SETI( 'XS', XMIN )
               CALL MSG_SETI( 'YS', YMIN )
               CALL MSG_SETI( 'XE', XMAX )
               CALL MSG_SETI( 'YE', YMAX )
               CALL ERR_REP( 'KPS1_SUBIx__EXBIN',
     :           'Failed to extract bin at ^XS, ^YS to ^XE, ^YE.',
     :           STATUS )
               GOTO 999
            END IF
 
*  Perform thresholding if required.  SURFIT supplies bad values to
*  indicate that no threshold is to be set, but the threshold routines
*  need two valid threshold values should one of the thresholds not be
*  required.  The limits are the maximum and minimum valid data values.
            IF ( LOTHRS .OR. HITHRS ) THEN
               IF ( LOTHRS .AND. .NOT. HITHRS ) THEN
                  CALL KPG1_THRSR( .TRUE., BINTOT, BIN, THRLO,
     :                             VAL__MAXR, VAL__BADR, VAL__BADR,
     :                             TBIN, NREPLO, NREPHI, STATUS )
 
               ELSE IF ( .NOT. LOTHRS .AND. HITHRS ) THEN
                  CALL KPG1_THRSR( .TRUE., BINTOT, BIN, VAL__MINR,
     :                             THRHI, VAL__BADR, VAL__BADR,
     :                             TBIN, NREPLO, NREPHI, STATUS )
 
               ELSE
                  CALL KPG1_THRSR( .TRUE., BINTOT, BIN, THRLO,
     :                             THRHI, VAL__BADR, VAL__BADR,
     :                             TBIN, NREPLO, NREPHI, STATUS )
 
               END IF
 
*  Put the result of the thresholding back into the original bin array,
*  for later processing.
               CALL COPAR( BINTOT, TBIN, BIN, STATUS )
            END IF
 
*  There is a choice of estimators for the value of the bin.  These
*  fall into broad groups: ordered statistics --- the median and mode,
*  and normal statistics --- mean, mean plus clipping.
            IF ( ORDERD ) THEN
 
*  ICMMM imposes a minimum of 5 values to derive the ordered
*  statistics.  It also iterates to a solution, removing the outliers,
*  so only allow those samples with an adequate sample size to be
*  passed to ICMMM, otherwise just compute the simple median.
               IF ( ESTIM .EQ. 'MEDI' .AND. BINTOT .LT. 12 ) THEN
                  CALL KPG1_MEDUR( .TRUE., BINTOT, BIN, MEDIAN, NSAM,
     :                             STATUS )
               ELSE
 
*  The mode and median are obtained and stored.  The mode is reliable
*  for moderately skew distributions.
                  CALL ICMMM( BIN, BINTOT, MEAN, MEDIAN, MODE, STDDEV,
     :                        SKEW, NSAM, NINVAL, STATUS )
               END IF
 
*  Only store the results when there were sufficient points.  Unused
*  bins are not put in the list.
               NOSTAT = NSAM .LT. NGMIN
               IF ( STATUS .EQ. SAI__OK .AND. ( .NOT. NOSTAT ) ) THEN
                  NBIN = NBIN + 1
                  IF ( ESTIM .EQ. 'MODE' ) THEN
                     Z( NBIN ) = NUM_RTOD( MODE )
                  ELSE IF ( ESTIM .EQ. 'MEDI' ) THEN
                     Z( NBIN ) = NUM_RTOD( MEDIAN )
                  END IF
                  W( NBIN ) = SQRT( NUM_ITOD( NSAM ) )
               END IF
 
            ELSE
 
*  Obtain the mean and standard deviation before and after clipping.
*  Note if the number of clips is zero only the unclipped statistics
*  will be computed.
               CALL STATV( BIN, BINTOT, NCLIP, CLIP, MAXMUM, MINMUM,
     :                     TOTAL, MEAN, STDDEV, NINVAL, MAXPOS, MINPOS,
     :                     MAXCL, MINCL, TOTCL, MEANCL, STDVCL, NPIXCL,
     :                     MAXPCL, MINPCL, STATUS )
 
*  Only store the results when there were sufficient points.  Unused
*  bins are not put in the list.
               NOSTAT = ( ( NPIXCL .LT. NGMIN ) .AND.
     :                    ( ESTIM .EQ. 'KSIG' ) ) .OR.
     :                  ( ( BINTOT - NINVAL .LT. NGMIN ) .AND.
     :                    ( ESTIM .EQ. 'MEAN' ) )
               IF ( STATUS .EQ. SAI__OK .AND. ( .NOT. NOSTAT ) ) THEN
                  NBIN = NBIN + 1
                  IF ( ESTIM .EQ. 'MEAN' ) THEN
                     Z( NBIN ) = NUM_RTOD( MEAN )
                     W( NBIN ) = NUM_RTOD( STDDEV )
                  ELSE
                     Z( NBIN ) = NUM_RTOD( MEANCL )
                     W( NBIN ) = NUM_RTOD( STDVCL )
                  END IF
               END IF
 
            END IF
 
*  If the status is bad or there were insufficient points to define
*  representative value, then the bin is to be ignored.
            IF ( STATUS .EQ. SAI__OK .AND. .NOT. NOSTAT ) THEN
               IX = 0
               IY = 0
               NPT = 0
 
*  Scan through the pixels within the bin, forming sums of valid
*  pixels' positions.
               DO J = 1, BY
                  DO I = 1, BX
 
                     IF ( BIN( I, J ) .NE. VAL__BADR ) THEN
                        IX = IX + I
                        IY = IY + J
                        NPT = NPT + 1
                     END IF
 
                  END DO
               END DO
 
*  This should not happen given the above code...but its better to be
*  safe than sorry. Overwrite this bin.
               IF ( NPT .LT. 1 ) THEN
                  NBIN = NBIN - 1
               ELSE
 
*  Compute the average x-y positions of the bin.  Note the sums were
*  within the current bin, so add the x-y offsets of the bin within the
*  input array.  Ideally, for the Kappa-sigma estimator the mean x-y
*  positions after clipping should be computed separately, but using
*  all the valid pixels is unlikely to make much difference.
                  X( NBIN ) = NUM_ITOD( IX ) / NUM_ITOD( NPT ) +
     :                        NUM_RTOD( XMIN - 1.0 )
                  Y( NBIN ) = NUM_ITOD( IY ) / NUM_ITOD( NPT ) +
     :                        NUM_RTOD( YMIN - 1.0 )
               END IF
 
            ELSE
 
*  Want to continue to the next bin, so must annul the error.
               CALL ERR_ANNUL( STATUS )
            END IF
         END DO
      END DO
 
  999 CONTINUE
 
      END
