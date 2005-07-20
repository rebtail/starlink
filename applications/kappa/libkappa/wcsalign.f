      SUBROUTINE WCSALIGN( STATUS )
*+
*  Name:
*     WCSALIGN

*  Purpose:
*     Aligns a group of NDFs using World Co-ordinate System information.

*  Language:
*     Starlink Fortran 77

*  Type of Module:
*     ADAM A-task

*  Invocation:
*     CALL WCSALIGN( STATUS )

*  Arguments:
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Description:
*     This application resamples or rebins a group of input NDFs, producing 
*     corresponding output NDFs which are aligned pixel-for-pixel
*     with a specified reference NDF. 
*
*     The transformations needed to produce alignment are derived from the 
*     co-ordinate system information stored in the WCS components of the
*     supplied NDFs. For each input NDF, alignment is first attempted in 
*     the current co-ordinate Frame of the reference NDF. If this fails,
*     alignment is attempted in the current co-ordinate Frame of the input
*     NDF. If this fails, alignment occurs in the pixel co-ordinate Frame.
*     A message indicating which Frame alignment was achieved in is
*     displayed.
*     
*     Two algorithms are available for determining the output pixel
*     values: resampling and rebinning (the method used is determined by
*     the REBIN parameter). The resampling algorithm steps through every
*     pixel in the output image, sampling the input image at the corresponding 
*     position and storing the sampled input value in the output pixel.
*     The method used for sampling the input image is determined by the 
*     METHOD parameter. The rebinning algorithm steps through every pixel in 
*     the input image, dividing the input pixel value up between a group of 
*     neighbouring output pixels. The way in which the input sample is
*     divided up between the output pixels is determined by the METHOD
*     parameter.
*
*     The two algorithms behaviour quite differently if the transformation 
*     from input to output includes any significant change of scale. In
*     general, resampling will not alter the pixel values associated with
*     a source, even if the pixel size changes. On the other hand, the 
*     rebinning algorithm will change the pixel values in order to
*     correct for a change in pixel size. Thus, rebinning conserves the
*     total data value within a given region where as resampling does not.
*    
*     Resampling is appropriate if the input image represents the spatial
*     density of some physical value (e.g. surface brightness) because the 
*     output image will have the same normalisation as the input image. But
*     rebinning is probably more appropriarte if the image measures (for
*     instance) flux per pixel, since rebinning takes account of the
*     change in pixel size.
*  
*     Another difference is that resampling guarantees to fill the output
*     image with good pixel values (assuming the input image is filled with
*     good input pixel values), whereas holes can be left by the rebinning 
*     algorithm if the output image has smaller pixels than the input image.
*     Such holes occur at output pixels which receive no contributions
*     from any input pixels, and will be filled with the value zero in
*     the output image. If this problem occurs the solution is probably
*     to change the width of the pixel spreading function by assigning
*     a larger value to PARAMS(1) and/or PARAMS(2) (depending on the
*     specific METHOD value being used).
*
*     Two methods exist for determining the bounds of the output NDFs.
*     Firstly, the user can give values for parameters LBND and UBND
*     which are then used as the pixel index bounds for all output
*     NDFs. Secondly, if a null value is given for LBND or UBND,
*     default values are generated separately for each output NDF so
*     that the output NDF just encloses the entire area covered by the
*     corresponding input NDF. Using the first method will ensure that
*     all output NDFs have the same pixel origin, and so the resulting
*     NDFs can be directly compared. However, this may result in the
*     output NDFs being larger than necessary. In general, the second
*     method results in smaller NDFs being produced, in less time.
*     However, the output NDFs will have differing pixel origins which
*     need to be taken into account when comparing the aligned NDFs.

*  Usage:
*     wcsalign in out lbnd ubnd ref 

*  ADAM Parameters:
*     ABORT = _LOGICAL (Read)
*        This controls what happens if an error occurs whilst processing
*        one of the input NDFs. If a false value is supplied for ABORT, 
*        then the error message will be displayed, but the application will
*        attempt to process any remaining input NDFs. If a true value is 
*        supplied for ABORT, then the error message will be displayed, and 
*        the application will abort. [NO]
*     ACC = _REAL (Read)
*        The positional accuracy required, as a a number of pixels. For
*        highly non-linear projections, a recursive algorithm is used in
*        which successively smaller regions of the projection are fitted 
*        with a least squares linear transformation. If such a transformation 
*        results in a maximum positional error greater than the value 
*        supplied for ACC (in pixels), then a smaller region is used. High 
*        accuracy is paid for by larger run times. [0.5]
*     IN = NDF (Read)
*        A group of input NDFs (of any dimensionality). This should be given 
*        as  a comma separated list, in which each list element can be:
*
*        - an NDF name, optionally containing wild-cards and/or regular 
*        expressions ("*", "?", "[a-z]" etc.). 
*
*        - the name of a text file, preceded by an up-arrow character "^".
*        Each line in the text file should contain a comma separated list
*        of elements, each of which can in turn be an NDF name (with
*        optional wild-cards, etc), or another file specification
*        (preceded by an up-arrow). Comments can be included in the file 
*        by commencing lines with a hash character "#".
*
*        If the value supplied for this parameter ends with a minus
*        sign "-", then the user is re-prompted for further input until
*        a value is given which does not end with a minus sign. All the
*        NDFs given in this way are concatenated into a single group.
*     INSITU = _LOGICAL (Read)
*        If INSITU is set to a true value, then no output NDFs are created. 
*        Instead, the pixel origin of each input NDF is modified in order
*        to align the input NDFs with the reference NDF (which is a much
*        faster operation than a full resampling). This can only be done 
*        if the mapping from input pixel co-ordinates to reference pixel
*        co-ordinates is a simple integer pixel shift of origin. If this is 
*        not the case an error will be reported when the input is processed 
*        (what happens then is controlled by the ABORT parameter). Also, 
*        in-situ alignment is only possible if null values are supplied for 
*        LBND and UBND. [NO]
*     LBND() = _INTEGER (Read)
*        An array of values giving the lower pixel index bound on each axis 
*        for the output NDFs. The number of values supplied should equal
*        the number of axes in the reference NDF. The given values are used 
*        for all output NDFs.  If a null value (!) is given for this parameter 
*        or for parameter UBND, then separate default values are calculated for
*        each output NDF which result in the output NDF just encompassing 
*        the corresponding input NDF. The suggested defaults are the 
*        lower pixel index bounds from the reference NDF (see parameter REF).
*     MAXPIX = _INTEGER (Read)
*        A value which specifies an initial scale size in pixels for the
*        adaptive algorithm which approximates non-linear Mappings with
*        piece-wise linear transformations. If MAXPIX is larger than any
*        dimension of the region of the output grid being used, a first
*        attempt will be made to approximate the Mapping by a linear
*        transformation over the entire output region. If a smaller value
*        is used, the output region will first be divided into subregions
*        whose size does not exceed MAXPIX pixels in any dimension, and then
*        attempts will be made at approximation. [1000]
*     METHOD = LITERAL (Read)
*        The method to use when sampling the input pixel values (if
*        resampling), or dividing an input pixel value up between a group 
*        of neighbouring output pixels (if rebinning). For details 
*        on these schemes, see the descriptions of routines AST_RESAMPLEx
*        and AST_REBINx in SUN/210. METHOD can take the following 
*        values:
*
*        - "Bilinear" -- When resampling, the output pixel values are 
*        calculated by bi-linear interpolation among the four nearest pixels 
*        values in the input NDF. When rebinning, the input pixel value
*        is divided up bi-linearly between the four nearest output pixels.
*        Produces smoother output NDFs than the nearest neighbour scheme, but 
*        is marginally slower.
*
*        - "Nearest" -- When resampling, the output pixel values are assigned 
*        the value  of the single nearest input pixel. When rebinning,
*        the input pixel value is assigned completely to the single
*        nearest output pixel.
*
*        - "Sinc" -- use the sinc(pi*x) kernel, where x is the pixel
*        offset from the interpolation point (resampling) or transformed
*        input pixel centre (rebinning), and sinc(z)=sin(z)/z. Use of this 
*        scheme is not recommended.
*
*        - "SincSinc" -- uses the sinc(pi*x)sinc(k*pi*x) kernel. A
*        valuable general-purpose scheme, intermediate in its visual effect 
*        on NDFs between the bilinear and nearest neighbour schemes. 
*         
*        - "SincCos" -- uses the sinc(pi*x)cos(k*pi*x) kernel. Gives
*        similar results to the sincsinc scheme.
*
*        - "SincGauss" -- uses the sinc(pi*x)exp(-k*x*x) kernel. Good 
*        results can be obtained by matching the FWHM of the
*        envelope function to the point spread function of the
*        input data (see parameter PARAMS).
*
*        - "Gauss" -- uses the exp(-k*x*x) kernel. This option is only 
*        available when rebinning (i.e. if REBIN is set to a TRUE value).
*        The FWHM of the Gaussian is given by parameter PARAMS(2), and
*        the point at which to truncate the Gaussian to zero is given by 
*        parameter PARAMS(1).
*
*        All methods propagate variances from input to output, but the
*        variance estimates produced by these schemes other than
*        nearest neighbour need to be treated with care since the spatial 
*        smoothing produced by these methods introduces 
*        correlations in the variance estimates. Also, the degree of 
*        smoothing produced varies across the NDF. This is because a 
*        sample taken at a pixel centre will have no contributions from the 
*        neighbouring pixels, whereas a sample taken at the corner of a 
*        pixel will have equal contributions from all four neighbouring 
*        pixels, resulting in greater smoothing and lower noise. This 
*        effect can produce complex Moire patterns in the output 
*        variance estimates, resulting from the interference of the 
*        spatial frequencies in the sample positions and in the pixel 
*        centre positions. For these reasons, if you want to use the 
*        output variances, you are generally safer using nearest neighbour
*        interpolation. [current value]
*     OUT = NDF (Write)
*        A group of output NDFs corresponding one-for-one with the list
*        of input NDFs given for parameter IN. This should be given as 
*        a comma separated list, in which each list element can be:
*        - an NDF name. If the name contains an asterisk character "*",
*        the name of the corresponding input NDF (without directory or
*        file suffix) is substituted for the asterisk (for instance, "*_al" 
*        causes the output NDF name to be formed by appending the string 
*        "_al" to the corresponding input NDF name). Input NDF names
*        can also be edited by including original and replacement strings 
*        between vertical bars after the NDF name (for instance,
*        *_al|b4|B1| causes any occurrence of the string "B4" in the input 
*        NDF name to be replaced by the string "B1" before appending the
*        string "_al" to the result).
*
*        - the name of a text file, preceded by an up-arrow character "^".
*        Each line in the text file should contain a comma separated list
*        of elements, each of which can in turn be an NDF name (with
*        optional editing, etc), or another file specification
*        (preceded by an up-arrow). Comments can be included in the file 
*        by commencing lines with a hash character "#".
*
*        If the value supplied for this parameter ends with a minus
*        sign "-", then the user is re-prompted for further input until
*        a value is given which does not end with a minus sign. All the
*        NDFs given in this way are concatenated into a single group.
*
*        This parameter is only accessed if the INSITU parameter is given
*        a false value.
*     PARAMS( 2 ) = _DOUBLE (Read)
*        An optional array which consists of additional parameters
*        required by the Sinc, SincSinc, SincCos, SincGauss and Gauss
*        methods.
*
*        PARAMS( 1 ) is required by all the above schemes.
*        It is used to specify how many pixels are to contribute to the 
*        interpolated result on either side of the interpolation or binning 
*        point in each dimension. Typically, a value of 2 is appropriate and 
*        the minimum allowed value is 1 ( i.e. one pixel on each side ). A 
*        value of zero or less indicates that a suitable number of pixels 
*        should be calculated automatically. [0]
*
*        PARAMS( 2 ) is required only by the Gauss, SincSinc, SincCos, and 
*        SincGauss schemes. For the SincSinc and SincCos 
*        schemes, it specifies the number of pixels at which the envelope
*        of the function goes to zero. The minimum value is 1.0, and the
*        run-time default value is 2.0. For the Gauss and SincGauss scheme, it
*        specifies the full-width at half-maximum (FWHM) of the Gaussian 
*        envelope. The minimum value is 0.1, and the run-time default is
*        1.0. On astronomical NDFs and spectra, good results are often 
*        obtained by approximately matching the FWHM of the envelope 
*        function, given by PARAMS(2), to the point spread function of the 
*        input data. []
*     REBIN = LOGICAL_ (Read)
*        Determines the algorithm used to calculate the output pixel
*        values. If a TRUE value is given, a rebinning algorithm is used.
*        Otherwise, a resampling algorithm is used [current value]
*     REF = NDF (Read)
*        The NDF to which all the input NDFs are to be aligned. If a null 
*        value is supplied for this parameter, the first NDF supplied for 
*        parameter IN is used. 
*     UBND() = _INTEGER (Read)
*        An array of values giving the upper pixel index bound on each axis 
*        for the output NDFs. The number of values supplied should equal
*        the number of axes in the reference NDF. The given values are used 
*        for all output NDFs.  If a null value (!) is given for this parameter 
*        or for parameter LBND, then separate default values are calculated for
*        each output NDF which result in the output NDF just encompassing 
*        the corresponding input NDF. The suggested defaults are the 
*        upper pixel index bounds from the reference NDF (see parameter REF).
*     WLIM = _REAL (Read)
*        This parameter is only used if REBIN is set TRUE. It specifies the 
*        minimum number of good pixels which must contribute to an output pixel
*        for the output pixel to be valid. Note, fractional values are
*        allowed. A null (!) value causes a very small positive value to
*        be used resulting in output pixels being set bad only if they
*        receive no significant contribution from any input pixel. [!]

*  Examples:
*     wcsalign image1 image1_al ref=image2 accept
*        This example resamples the NDF called image1 so that it is aligned 
*        with the NDF call image2, putting the output in image1_al. The output 
*        image has the same pixel index bounds as image2 and inherits WCS 
*        information from image2.
*     wcsalign m51* *_al lbnd=! accept
*        This example resamples all the NDFs with names starting with the 
*        string "m51" in the current directory so that 
*        they are aligned with the first input NDF. The output NDFs
*        have the same names as the input NDFs, but extended with the
*        string "_al". Each output NDF is just big enough to contain all 
*        the pixels in the corresponding input NDF.
*     wcsalign ^in.lis ^out.lis lbnd=! accept
*        This example is like the previous example, except that the names
*        of the input NDFs are read from the text file in.lis, and the
*        names of the corresponding output NDFs are read from text file
*        out.lis.

*  Notes:
*     -  WCS information (including the current co-ordinate Frame) is 
*     propagated from the reference NDF to all output NDFs. 
*     -  QUALITY is propagated from input to output only if parameter
*     METHOD is set to Nearest and REBIN is set to FALSE.

*  Related Applications:
*     KAPPA: WCSFRAME, RESAMPLE; CCDPACK: TRANNDF.

*  Implementation Status:
*     -  This routine correctly processes the DATA, VARIANCE, LABEL, 
*     TITLE, UNITS, WCS and HISTORY components of the input NDFs (see the
*     METHOD parameter for notes on the interpretation of output variances).
*     -  Processing of bad pixels and automatic quality masking are
*     supported.
*     -  All non-complex numeric data types can be handled. If REBIN is
*     TRUE, the data type will be converted to one of _INTEGER, _DOUBLE
*     or _REAL for processing.

*  Authors:
*     DSB: David Berry (STARLINK)
*     TDCA: Tim Ash (STARLINK)
*     {enter_new_authors_here}

*  History:
*     6-OCT-1998 (DSB):
*        Original version, based on IRAS90:SKYALIGN. 
*     8-JUL-1999 (TDCA):
*        Modified to use AST_RESAMPLE
*     5-AUG-1999 (DSB):
*        Tidied up.
*     19-SEP-2001 (DSB):
*        Allow use with 1-dimensional NDFs by changing kpg1_asget EXACT
*        argument to .false.
*     31-OCT-2002 (DSB):
*        Make N-dimensional.
*     12-NOV-2004 (DSB):
*        Add INSITU and ABORT parameters.
*     19-JUL-2005 (DSB):
*        Add REBIN parameter.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'PRM_PAR'          ! VAL__ constants
      INCLUDE 'PAR_ERR'          ! PAR error constants
      INCLUDE 'GRP_PAR'          ! GRP constants
      INCLUDE 'NDF_PAR'          ! NDF constants
      INCLUDE 'AST_PAR'          ! AST constants
      INCLUDE 'KAP_ERR'          ! KAPPA error constants

*  Status:
      INTEGER STATUS             ! Global status

*  Local Variables:
      CHARACTER METHOD*13           ! Interpolation method to use.
      CHARACTER MODE*6           ! Access mode for input NDFs
      CHARACTER NDFNAM*(GRP__SZNAM) ! The name of an NDF.
      DOUBLE PRECISION PARAMS( 2 ) ! Param. values passed to AST_RESAMPLE<x>
      INTEGER I                  ! Index into input and output groups
      INTEGER IGRP1              ! GRP id. for group holding input NDFs
      INTEGER IGRP2              ! GRP id. for group holding output NDFs
      INTEGER INDF1              ! NDF id. for the input NDF
      INTEGER INDF2              ! NDF id. for the output NDF
      INTEGER INDFR              ! NDF id. for the reference NDF
      INTEGER IWCSR              ! WCS FrameSet for reference NDF
      INTEGER J                  ! Axis index
      INTEGER LBND( NDF__MXDIM ) ! Indices of lower left corner of outputs
      INTEGER LBNDR( NDF__MXDIM )! Lower pixel bounds of reference NDF
      INTEGER MAP                ! AST id for (pix_in -> pix_out) Mapping
      INTEGER MAP4               ! AST id for (grid_in -> pix_in) Mapping
      INTEGER MAXPIX             ! Initial scale size in pixels
      INTEGER METHOD_CODE        ! Integer corresponding to interp. method 
      INTEGER NDIMR              ! Number of pixel axes in reference NDF
      INTEGER NPAR               ! No. of required interpolation parameters
      INTEGER ORIGIN( NDF__MXDIM )! New pixel origin
      INTEGER SHIFT( NDF__MXDIM )! Pixel axis shifts 
      INTEGER SIZE               ! Total size of the input group
      INTEGER SIZEO              ! Total size of the output group
      INTEGER UBND( NDF__MXDIM ) ! Indices of upper right corner of outputs
      INTEGER UBNDR( NDF__MXDIM )! Upper pixel bounds of reference NDF
      LOGICAL ABORT              ! Abort upon first error?
      LOGICAL AUTOBN             ! Determine output bounds automatically?
      LOGICAL INSITU             ! Modify input NDFs in-situ?
      LOGICAL MORE               ! Continue looping?
      LOGICAL REBIN              ! Create output pixels by rebinning?
      REAL ERRLIM                ! Positional accuracy in pixels
      REAL WLIM                  ! Minimum good output weight
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Begin an AST context.
      CALL AST_BEGIN( STATUS )

*  Get a group containing the names of the NDFs to be processed.
      CALL KPG1_RGNDF( 'IN', 0, 1, '  Give more NDFs...', 
     :                 IGRP1, SIZE, STATUS )

*  Begin an NDF context.
      CALL NDF_BEGIN

*  Abort if an error has occurred.
      IF ( STATUS .NE. SAI__OK ) GO TO 999

*  Get the reference NDF.
      CALL LPG_ASSOC( 'REF', 'READ', INDFR, STATUS )

*  If a null value was supplied, annul the error and use the first NDF
*  supplied for IN.
      IF( STATUS .EQ. PAR__NULL ) THEN
         CALL ERR_ANNUL( STATUS )
         CALL NDG_NDFAS( IGRP1, 1, 'READ', INDFR, STATUS )
      END IF

*  Get the associated WCS FrameSet. 
      CALL KPG1_GTWCS( INDFR, IWCSR, STATUS )

*  Get the dimensionality and pixel bounds of the reference NDF.
      CALL NDF_BOUND( INDFR, NDF__MXDIM, LBNDR, UBNDR, NDIMR, STATUS ) 

*  Set the suggested default for LBND and UBND.
      CALL PAR_DEF1I( 'LBND', NDIMR, LBNDR, STATUS )
      CALL PAR_DEF1I( 'UBND', NDIMR, UBNDR, STATUS )

*  Abort if an error has occurred.
      IF ( STATUS .NE. SAI__OK ) GO TO 999

*  Get the bounds required for the output NDFs.
      CALL PAR_EXACI( 'LBND', NDIMR, LBND, STATUS )
      CALL PAR_EXACI( 'UBND', NDIMR, UBND, STATUS )

*  If a null value was supplied for LBND or UBND, annul the error and
*  put bad values in them.
      IF( STATUS .EQ. PAR__NULL ) THEN
         CALL ERR_ANNUL( STATUS )
         DO I = 1, NDIMR 
            LBND( I ) = VAL__BADI
            UBND( I ) = VAL__BADI
         END DO
         AUTOBN = .TRUE.
      ELSE
         AUTOBN = .FALSE.
      END IF

*  If the alignment involves a simple shift of origin, there is the
*  option to just change the origin of the input NDF rather than creating
*  a whole new output NDF. This can only be done if null values were
*  supplied for the bounds parameters.
      CALL PAR_GET0L( 'INSITU', INSITU, STATUS )
      IF( INSITU .AND. .NOT. AUTOBN .AND. STATUS .EQ. SAI__OK ) THEN
         STATUS = SAI__ERROR
         CALL ERR_REP( 'WCSALIGN_ERR1', 'Cannot perform alignment '//
     :                 'in-situ because the output image bounds have '//
     :                 'been specified explicitly.', STATUS )
      END IF

*  In-situ alignment requires update access to the input NDFs.
      IF( INSITU ) THEN
         MODE = 'UPDATE'
      ELSE
         MODE = 'READ'
      END IF

*  Get a group containing the names of the output NDFs.  Base
*  modification elements on the group containing the input NDFs.
*  Do not do this if alignment is being performed in situ.
      IF( INSITU ) THEN
         IGRP2 = GRP__NOID
      ELSE 
         CALL KPG1_WGNDF( 'OUT', IGRP1, SIZE, SIZE,
     :                    '  Give more NDFs...',
     :                     IGRP2, SIZEO, STATUS )
      END IF

*  See if the application should abort if any input NDF cannot be
*  processed.Otherwise, the error is annulled and the application continues
*  to process remaining inputs. 
      CALL PAR_GET0L( 'ABORT', ABORT, STATUS )

*  Get the algorithm to use.
      CALL PAR_GET0L( 'REBIN', REBIN, STATUS )

*  Get the interpolation/spreading method to be used.
      MORE = .TRUE.
      DO WHILE( MORE .AND. STATUS .EQ. SAI__OK )
         CALL PAR_CHOIC( 'METHOD', 'SincSinc', 'Nearest,Bilinear,'//
     :                   'Sinc,Gauss,SincSinc,SincCos,SincGauss', 
     :                   .TRUE., METHOD, STATUS )
         IF( .NOT. REBIN .AND. METHOD( 1 : 1 ) .EQ. 'G' ) THEN
            CALL MSG_OUT( ' ', 'Method "Gauss" cannot be used '//
     :                    'because REBIN is set false.', STATUS )
            CALL MSG_OUT( ' ', 'Please supply a new value for '//
     :                    'parameter METHOD.', STATUS )
            CALL PAR_CANCL( 'METHOD', STATUS )
         ELSE
            MORE = .FALSE.
         END IF
         CALL MSG_BLANK( STATUS )
      END DO

*  Tell the user what method is being used, and convert value of
*  METHOD to one of the values expected by AST_RESAMPLE<x>. 
      IF( REBIN ) THEN
         CALL MSG_SETC( 'W', 'binning' )
      ELSE
         CALL MSG_SETC( 'W', 'interpolation' )
      END IF

      NPAR = 0
      IF( METHOD( 1 : 1 ) .EQ. 'N' ) THEN
         METHOD_CODE = AST__NEAREST
         CALL MSG_OUT( 'WCSALIGN_MSG1', 
     :                 '  Using nearest neighbour ^W.', 
     :                 STATUS ) 

      ELSE IF( METHOD( 1 : 1 ) .EQ. 'B' ) THEN
         METHOD_CODE = AST__LINEAR
         CALL MSG_OUT( 'WCSALIGN_MSG2', 
     :                 '  Using bi-linear ^W.', STATUS ) 

      ELSE IF( METHOD( 1 : 1 ) .EQ. 'G' ) THEN
         NPAR = 2
         PARAMS( 1 ) = 0.0
         PARAMS( 2 ) = 2.0
         METHOD_CODE = AST__GAUSS
         CALL MSG_OUT( 'WCSALIGN_MSG2', 
     :                 '  Using a Gaussian ^W kernel.', STATUS ) 

      ELSE IF ( METHOD( 1 : 4 ) .EQ. 'SINC' ) THEN
         NPAR = 2
         PARAMS( 1 ) = 0.0
         PARAMS( 2 ) = 2.0

         IF ( METHOD( 5 : 5 ) .EQ. 'S' ) THEN
            METHOD_CODE = AST__SINCSINC
            CALL MSG_OUT( 'WCSALIGN_MSG3', 
     :                    '  Using sincsinc ^W kernel.', STATUS ) 

         ELSE IF( METHOD( 5 : 5 ) .EQ. 'C' ) THEN
            METHOD_CODE = AST__SINCCOS
            CALL MSG_OUT( 'WCSALIGN_MSG4', 
     :                    '  Using sinccos ^W kernel.', STATUS ) 

         ELSE IF( METHOD( 5 : 5 ) .EQ. 'G' ) THEN
            METHOD_CODE = AST__SINCGAUSS
            PARAMS( 2 ) = 1.0
            CALL MSG_OUT( 'WCSALIGN_MSG5', 
     :                    '  Using sincgauss ^W kernel.', STATUS ) 

         ELSE
            NPAR = 1
            METHOD_CODE = AST__SINC
            CALL MSG_OUT( 'WCSALIGN_MSG6', 
     :                    '  Using sinc ^W kernel.', STATUS ) 

         END IF

      END IF

*  If required, set the dynamic defaults for PARAMS.
      IF( NPAR .GT. 0 ) THEN
         CALL PAR_DEF1D( 'PARAMS', NPAR, PARAMS, STATUS )

*  Get the required number of interpolation parameters.
         CALL PAR_EXACD( 'PARAMS', NPAR, PARAMS, STATUS ) 
      END IF

*  Get the positional accuracy required.
      CALL PAR_GET0R( 'ACC', ERRLIM, STATUS )      
      ERRLIM = MAX( 0.0001, ERRLIM )

*  Get the minimum acceptable output weight
      IF( STATUS .EQ. SAI__OK .AND. REBIN ) THEN
         CALL PAR_GET0R( 'WLIM', WLIM, STATUS )      
         IF( STATUS .EQ. PAR__NULL ) THEN
            CALL ERR_ANNUL( STATUS )
            WLIM = 1.0E-10
         END IF
      END IF

*  Get a value for MAXPIX.
      CALL PAR_GET0I( 'MAXPIX', MAXPIX, STATUS )
      MAXPIX = MAX( 1, MAXPIX )

*  Abort if an error has occurred.
      IF ( STATUS .NE. SAI__OK ) GO TO 999

*  Loop round each NDF to be processed.
      DO I = 1, SIZE
         CALL MSG_BLANK( STATUS )

*  Get an NDF identifier for the input NDF.
         CALL NDG_NDFAS( IGRP1, I, MODE, INDF1, STATUS )

*  Tell the user which input NDF is currently being procesed.
         CALL NDF_MSG( 'NDF', INDF1 )
         CALL MSG_OUT( 'WCSALIGN_MSG7', '  Processing ^NDF...', STATUS )

*  Find the Mapping from input pixel co-ordinates to reference (i.e.
*  output) pixel co-ordinates. This also determines if the Mapping is a
*  simple integer pixel shift of origin. If it is, it returns the new pixel 
*  origin in ORIGIN.
         CALL KPS1_WALA7( INDF1, IWCSR, MAP, MAP4, ORIGIN, STATUS )

*  If the input pixel->output pixel Mapping is a shift of origin, we do
*  not need to do a full resampling or rebinning. However, if the output 
*  bounds have been specified explcitly, then we cannot simply shift the 
*  input pixel origin. 
         IF( ORIGIN( 1 ) .NE. VAL__BADI .AND. AUTOBN ) THEN

*  If the alignment is being performed in-situ, get a clone of the input
*  NDF identifier. Otherwise take a copy of the input NDF.
            IF( INSITU ) THEN
               CALL NDF_CLONE( INDF1, INDF2, STATUS )
            ELSE
               CALL NDG_NDFCO( INDF1, IGRP2, I, INDF2, STATUS )
            END IF

*  Determine the pixel shifts to apply to INDF2.
            CALL NDF_BOUND( INDF2, NDIMR, LBND, UBND, NDIMR, STATUS )
            DO J = 1, NDIMR
               SHIFT( J ) = ORIGIN( J ) - LBND( J )
            END DO

*  Apply the shifts.
            CALL NDF_SHIFT( NDIMR, SHIFT, INDF2, STATUS )

*  If the input pixel->output pixel Mapping is not just a shift of origin, 
*  we do a full resampling or rebinning. We cannot do this if "in-situ" 
*  alignment was requested. 
         ELSE IF( .NOT. INSITU ) THEN

*  Create the output NDF by propagation from the input NDF. The default
*  components HISTORY, TITLE, LABEL and all extensions are propagated,
*  together with the UNITS component. The NDF is initially created with
*  the same bounds as the input NDF.
            CALL NDG_NDFPR( INDF1, 'UNITS', IGRP2, I, INDF2, STATUS )

*  Process this pair of input and output NDFs.
            CALL KPS1_WALA0( NDIMR, INDF1, INDF2, MAP, MAP4, IWCSR, 
     :                       METHOD_CODE, PARAMS, LBND, UBND, ERRLIM, 
     :                       MAXPIX, REBIN, WLIM, STATUS )

*  Report an error if in-situ alignment was requested but the Mapping is
*  not a shift of origin.
         ELSE
            STATUS = KAP__WALA0
            CALL ERR_REP( 'WCSALIGN_ERR2', 'Cannot perform alignment '//
     :                    'in-situ because the Mapping is not a '//
     :                    'simple shift of origin.', STATUS )
         END IF

*  Annul the input NDF identifier.
         CALL NDF_ANNUL( INDF1, STATUS )

*  If an error has occurred, delete any output NDF, otherwise just 
*  annul its identifier.
         IF( STATUS .NE. SAI__OK .AND. .NOT. INSITU ) THEN
            CALL NDF_DELET( INDF2, STATUS )
         ELSE
            CALL NDF_ANNUL( INDF2, STATUS )
         END IF

*  If an error occurred processing the current input NDF...
         IF( STATUS .NE. SAI__OK  ) THEN

*  If the user has opted to abort processing if an error occurs,abort.
            IF( ABORT ) GO TO 999 

*  Flush the error.
            CALL ERR_FLUSH( STATUS )

*  Give a warning telling the user that no output NDF will be created 
*  for the current input NDF.
            IF( INSITU ) THEN
               CALL GRP_GET( IGRP1, I, 1, NDFNAM, STATUS )
               CALL MSG_SETC( 'NDF', NDFNAM )
               CALL MSG_OUT( 'WCSALIGN_MSG8', '  WARNING: Input ^NDF'//
     :                       ' cannot be aligned', STATUS )
            ELSE
               CALL GRP_GET( IGRP2, I, 1, NDFNAM, STATUS )
               CALL MSG_SETC( 'NDF', NDFNAM )
               CALL MSG_OUT( 'WCSALIGN_MSG9', '  WARNING: Output ^NDF'//
     :                       ' cannot be produced', STATUS )
            END IF
         END IF

*  Process the next input NDF.
      END DO

*  Display a blank line.
      CALL MSG_BLANK( STATUS )

*  Tidy up.
*  ========
 999  CONTINUE

*  End the NDF context.
      CALL NDF_END( STATUS )

*  Delete all groups.
      CALL GRP_DELET( IGRP1, STATUS )
      IF( .NOT. INSITU ) CALL GRP_DELET( IGRP2, STATUS )

*  End the AST context.
      CALL AST_END( STATUS )

*  Add a context report if anything went wrong.
      IF ( STATUS .NE. SAI__OK ) THEN
         CALL ERR_REP( 'WCSALIGN_ERR', 'WCSALIGN: Failed to align a '//
     :                 'group of NDFs using WCS information.', STATUS )
      END IF

      END
