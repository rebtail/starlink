*+ ARX_ANNULUS - write description of annulus
      SUBROUTINE ARX_ANNULUS(ARDID,INDEX,MODE,EXCLUDE,XC,YC,IRAD,ORAD,
     :                                                         STATUS)
*    Description :
*    Deficiencies :
*    Bugs :
*    Authors :
*     BHVAD::RJV
*    History :
*    Type definitions :
      IMPLICIT NONE
*    Global constants :
      INCLUDE 'SAE_PAR'
      INCLUDE 'DAT_PAR'
*    Global variables :
*    Import :
      INTEGER ARDID
      INTEGER INDEX
      CHARACTER*(*) MODE
      LOGICAL EXCLUDE
      REAL XC,YC,IRAD,ORAD
*    Export :
*    Status :
      INTEGER STATUS
*    Function declarations :
      INTEGER CHR_LEN
*    Local constants :
*    Local variables :
      CHARACTER*80 TEXT
      INTEGER L
*-

      IF (STATUS.EQ.SAI__OK) THEN


        IF (MODE.EQ.'AND') THEN
          TEXT=' .AND.'
          L=7
        ELSEIF (MODE.EQ.'OR'.OR.MODE.EQ.'ADD') THEN
          TEXT=' '
          L=2
        ELSEIF (MODE.EQ.'XOR') THEN
          TEXT=' .XOR.'
          L=7
        ELSEIF (MODE.EQ.'EQV') THEN
          TEXT=' .EQV.'
          L=7
        ELSE
          TEXT=' '
          L=2
        ENDIF

        IF (EXCLUDE) THEN
          TEXT(L:)=' .NOT. (CIRCLE( '
        ELSE
          TEXT(L:)=' (CIRCLE( '
        ENDIF
        L=CHR_LEN(TEXT)

        CALL MSG_SETR('XC',XC)
        CALL MSG_SETR('YC',YC)
        CALL MSG_SETR('RAD',ORAD)
        CALL MSG_MAKE(TEXT(:L)//' ^XC , ^YC , ^RAD )',TEXT,L)

        CALL ARX_PUT(ARDID,INDEX,TEXT(:L),STATUS)


        TEXT='     .AND..NOT.CIRCLE('
        L=CHR_LEN(TEXT)

        CALL MSG_SETR('XC',XC)
        CALL MSG_SETR('YC',YC)
        CALL MSG_SETR('RAD',IRAD)
        CALL MSG_MAKE(TEXT(:L)//' ^XC , ^YC , ^RAD ))',TEXT,L)
        IF (EXCLUDE) THEN
          L=L+1
          TEXT(L:)=')'
        ENDIF

        CALL ARX_PUT(ARDID,0,TEXT(:L),STATUS)




        IF (STATUS.NE.SAI__OK) THEN
          CALL ERR_REP(' ','from ARX_ANNULUS',STATUS)
        ENDIF

      ENDIF

      END
