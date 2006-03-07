/*
*  Name:
*     kaplibs.c

*  Purpose:
*     Implement the C interface to the standalone routines in the KAPLIBS 
*     library.

*  Description:
*     This module implements C-callable wrappers for the public non-ADAM 
*     routines in the KAPLIBS library. The interface to these wrappers
*     is defined in kaplibs.h.

*  Notes:
*     - Given the size of the KAPLIBS library, providing a complete C
*     interface is probably not worth the effort. Instead, I suggest that 
*     people who want to use KAPLIBS from C extend this file (and
*     kaplibs.h) to include any functions which they need but which are
*     not already included.

*  Authors:
*     DSB: David S Berry
*     TIMJ: Tim Jenness (JAC, Hawaii)
*     {enter_new_authors_here}

*  History:
*     29-SEP-2005 (DSB):
*        Original version.
*     03-NOV-2005 (TIMJ):
*        Update GRP interface
*     7-MAR-2006 (DSB):
*        Added KPG1_RGNDF and KPG1_WGNDF.
*     {enter_further_changes_here}
*/

/* Header files. */
/* ============= */
#include "f77.h"
#include "sae_par.h"
#include "mers.h"
#include "star/grp.h"
#include "star/hds_fortran.h"
#include "kaplibs.h"
#include "kaplibs_private.h"
#include <string.h>

/* Wrapper function implementations. */
/* ================================= */

F77_SUBROUTINE(kpg1_fillr)( REAL(VALUE), 
                            INTEGER(EL), 
                            REAL_ARRAY(ARRAY), 
                            INTEGER(STATUS) );

void kpg1Fillr( float value, int el, float *array, int *status ){
   DECLARE_REAL(VALUE);
   DECLARE_INTEGER(EL);
   DECLARE_REAL_ARRAY_DYN(ARRAY);
   DECLARE_INTEGER(STATUS);

   F77_CREATE_REAL_ARRAY( ARRAY, el );

   F77_EXPORT_REAL( value, VALUE );
   F77_EXPORT_INTEGER( el, EL );
   F77_ASSOC_REAL_ARRAY( ARRAY, array );
   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(kpg1_fillr)( REAL_ARG(&VALUE),
                         INTEGER_ARG(&EL),
                         REAL_ARRAY_ARG(ARRAY),
                         INTEGER_ARG(&STATUS) );

   F77_IMPORT_INTEGER( STATUS, *status );
   F77_IMPORT_REAL_ARRAY( ARRAY, array,el );
   F77_FREE_REAL( ARRAY );
}

/* ------------------------------- */

F77_SUBROUTINE(kpg1_gausr)( REAL(SIGMA), 
                            INTEGER(IBOX), 
                            LOGICAL(SAMBAD),
                            REAL(WLIM),
                            INTEGER(NX),
                            INTEGER(NY),
                            LOGICAL(BAD),
                            LOGICAL(VAR),
                            REAL_ARRAY(A),
                            REAL_ARRAY(B),
                            LOGICAL(BADOUT),
                            REAL_ARRAY(WEIGHT),
                            REAL_ARRAY(AMAR),
                            REAL_ARRAY(WMAR),
                            INTEGER(STATUS) );

void kpg1Gausr( float sigma, int ibox, int sambad, float wlim, int nx,
                int ny, int bad, int var, float *a, float *b, int *badout, 
                float *weight, float *amar, float *wmar, int *status ){

   DECLARE_REAL(SIGMA);
   DECLARE_INTEGER(IBOX);
   DECLARE_LOGICAL(SAMBAD);
   DECLARE_REAL(WLIM);
   DECLARE_INTEGER(NX);
   DECLARE_INTEGER(NY);
   DECLARE_LOGICAL(BAD);
   DECLARE_LOGICAL(VAR);
   DECLARE_REAL_ARRAY_DYN(A);
   DECLARE_REAL_ARRAY_DYN(B);
   DECLARE_LOGICAL(BADOUT);
   DECLARE_REAL_ARRAY_DYN(WEIGHT);
   DECLARE_REAL_ARRAY_DYN(AMAR);
   DECLARE_REAL_ARRAY_DYN(WMAR);
   DECLARE_INTEGER(STATUS);

   int nxy, nw;
   nxy = nx*ny;
   nw = 2*ibox + 1;

   F77_CREATE_REAL_ARRAY( A, nxy );
   F77_CREATE_REAL_ARRAY( B, nxy );
   F77_CREATE_REAL_ARRAY( WEIGHT, nw );
   F77_CREATE_REAL_ARRAY( AMAR, nx );
   F77_CREATE_REAL_ARRAY( WMAR, nx );

   F77_EXPORT_REAL( sigma, SIGMA );
   F77_EXPORT_INTEGER( ibox, IBOX );
   F77_EXPORT_LOGICAL( sambad, SAMBAD );
   F77_EXPORT_REAL( wlim, WLIM );
   F77_EXPORT_INTEGER( nx, NX );
   F77_EXPORT_INTEGER( ny, NY );
   F77_EXPORT_LOGICAL( bad, BAD );
   F77_EXPORT_LOGICAL( var, VAR );
   F77_EXPORT_REAL_ARRAY( a, A, nxy );
   F77_ASSOC_REAL_ARRAY( B, b );
   F77_ASSOC_REAL_ARRAY( WEIGHT, weight );
   F77_ASSOC_REAL_ARRAY( AMAR, amar );
   F77_ASSOC_REAL_ARRAY( WMAR, wmar );
   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(kpg1_gausr)( REAL_ARG(&SIGMA), 
                         INTEGER_ARG(&IBOX), 
                         LOGICAL_ARG(&SAMBAD),
                         REAL_ARG(&WLIM),
                         INTEGER_ARG(&NX),
                         INTEGER_ARG(&NY),
                         LOGICAL_ARG(&BAD),
                         LOGICAL_ARG(&VAR),
                         REAL_ARRAY_ARG(A),
                         REAL_ARRAY_ARG(B),
                         LOGICAL_ARG(&BADOUT),
                         REAL_ARRAY_ARG(WEIGHT),
                         REAL_ARRAY_ARG(AMAR),
                         REAL_ARRAY_ARG(WMAR),
                         INTEGER_ARG(&STATUS) );

   F77_IMPORT_LOGICAL( BADOUT, *badout );
   F77_IMPORT_INTEGER( STATUS, *status );
   F77_IMPORT_REAL_ARRAY( B, b, nxy );

   F77_FREE_REAL( A );
   F77_FREE_REAL( B );
   F77_FREE_REAL( WEIGHT );
   F77_FREE_REAL( AMAR );
   F77_FREE_REAL( BMAR );
}

/* ------------------------------- */

void kpg1Kygrp( AstKeyMap *keymap, Grp **igrp, int *status ){
  kpg1Kygp1( keymap, igrp, NULL, status );
}

/* ------------------------------- */

void kpg1Kymap( Grp *igrp, AstKeyMap **keymap, int *status ){
  kpg1Kymp1( igrp, keymap, status );
}

/* ------------------------------- */

/* NB The supplied axis indices should be one based, not zero based. */

F77_SUBROUTINE(kpg1_manir)( INTEGER(NDIMI), 
                            INTEGER_ARRAY(DIMI),
                            REAL_ARRAY(IN),
                            INTEGER(NDIMO), 
                            INTEGER_ARRAY(DIMO),
                            INTEGER_ARRAY(AXES),
                            INTEGER_ARRAY(COLOFF),
                            INTEGER_ARRAY(EXPOFF),
                            REAL_ARRAY(OUT),
                            INTEGER(STATUS) );

void kpg1Manir( int ndimi, int *dimi, float *in, int ndimo, int *dimo, 
                int *axes, int *coloff, int *expoff, float *out, int *status ){

   DECLARE_INTEGER(NDIMI);
   DECLARE_INTEGER_ARRAY_DYN(DIMI);
   DECLARE_REAL_ARRAY_DYN(IN);
   DECLARE_INTEGER(NDIMO);
   DECLARE_INTEGER_ARRAY_DYN(DIMO);
   DECLARE_INTEGER_ARRAY_DYN(AXES);
   DECLARE_INTEGER_ARRAY_DYN(COLOFF);
   DECLARE_INTEGER_ARRAY_DYN(EXPOFF);
   DECLARE_REAL_ARRAY_DYN(OUT);
   DECLARE_INTEGER(STATUS);

   int ncoloff, nexpoff, i, j, use, nin, nout;

   nin = 1;
   ncoloff = 1;
   for( i = 0; i < ndimi; i++ ) {
      use = 1;      
      for( j = 0; j < ndimo; j++ ) {
         if( axes[ j ] == i ) {
            use = 0;
            break;
         }
      }
      if( use ) ncoloff *= dimi[ i ];
      nin *= dimi[ i ];
   }

   nout = 1;
   nexpoff = 1;
   for( j = 0; j < ndimo; j++ ) {
      if( axes[ j ] == 0 ) nexpoff *= dimo[ j ];
      nout *= dimo[ j ];
   }

   F77_CREATE_INTEGER_ARRAY( DIMI, ndimi );
   F77_CREATE_REAL_ARRAY( IN, nin );
   F77_CREATE_INTEGER_ARRAY( DIMO, ndimo );
   F77_CREATE_INTEGER_ARRAY( AXES, ndimo );
   F77_CREATE_INTEGER_ARRAY( COLOFF, ncoloff );
   F77_CREATE_INTEGER_ARRAY( EXPOFF, nexpoff );
   F77_CREATE_REAL_ARRAY( OUT, nout );

   F77_EXPORT_INTEGER( ndimi, NDIMI );
   F77_EXPORT_INTEGER_ARRAY( dimi, DIMI, ndimi );
   F77_EXPORT_REAL_ARRAY( in, IN, nin );
   F77_EXPORT_INTEGER( ndimo, NDIMO );
   F77_EXPORT_INTEGER_ARRAY( dimo, DIMO, ndimo );
   F77_EXPORT_INTEGER_ARRAY( axes, AXES, ndimo );
   F77_ASSOC_INTEGER_ARRAY( COLOFF, coloff );
   F77_ASSOC_INTEGER_ARRAY( EXPOFF, expoff );
   F77_ASSOC_REAL_ARRAY( OUT, out );
   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(kpg1_manir)( INTEGER_ARG(&NDIMI), 
                         INTEGER_ARRAY_ARG(DIMI),
                         REAL_ARRAY_ARG(IN),
                         INTEGER_ARG(&NDIMO), 
                         INTEGER_ARRAY_ARG(DIMO),
                         INTEGER_ARRAY_ARG(AXES),
                         INTEGER_ARRAY_ARG(COLOFF),
                         INTEGER_ARRAY_ARG(EXPOFF),
                         REAL_ARRAY_ARG(OUT),
                         INTEGER_ARG(&STATUS) );

   F77_IMPORT_INTEGER( STATUS, *status );
   F77_IMPORT_REAL_ARRAY( OUT, out, nout );

   F77_FREE_REAL( DIMI );
   F77_FREE_REAL( IN );
   F77_FREE_REAL( DIMO );
   F77_FREE_REAL( AXES );
   F77_FREE_REAL( COLOFF );
   F77_FREE_REAL( EXPOFF );
   F77_FREE_REAL( OUT );

}

/* ------------------------------- */

F77_SUBROUTINE(kpg1_pseed)( INTEGER(STATUS) );

void kpg1Pseed( int *status ){
   DECLARE_INTEGER(STATUS);
   F77_EXPORT_INTEGER( *status, STATUS );
   F77_CALL(kpg1_pseed)( INTEGER_ARG(&STATUS) );
   F77_IMPORT_INTEGER( STATUS, *status );
}

/* ------------------------------- */

F77_SUBROUTINE(irq_delet)( INTEGER(INDF),
                           INTEGER(STATUS) );

void irqDelet( int indf, int *status ){
   DECLARE_INTEGER(INDF);
   DECLARE_INTEGER(STATUS);
   F77_EXPORT_INTEGER( indf, INDF );
   F77_EXPORT_INTEGER( *status, STATUS );
   F77_CALL(irq_delet)( INTEGER_ARG(&INDF),
                        INTEGER_ARG(&STATUS)  );
   F77_IMPORT_INTEGER( STATUS, *status );
}

/* ------------------------------- */

F77_SUBROUTINE(irq_rlse)( CHARACTER_ARRAY(LOCS), 
                          INTEGER(STATUS)
                          TRAIL(LOCS) );

void irqRlse( IRQLocs **locs, int *status ){
   DECLARE_CHARACTER_ARRAY(LOCS,DAT__SZLOC,5);  
   DECLARE_INTEGER(STATUS);

   if( !locs || !*locs ) return;

/* Convert the C HDSLocs to DAT__SZLOC character locators, freeing the
   memory used to store the HDSLocs. */
   datExportFloc( &( (*locs)->loc[0] ), 1, DAT__SZLOC, LOCS[0], status );
   datExportFloc( &( (*locs)->loc[1] ), 1, DAT__SZLOC, LOCS[1], status );
   datExportFloc( &( (*locs)->loc[2] ), 1, DAT__SZLOC, LOCS[2], status );
   datExportFloc( &( (*locs)->loc[3] ), 1, DAT__SZLOC, LOCS[3], status );
   datExportFloc( &( (*locs)->loc[4] ), 1, DAT__SZLOC, LOCS[4], status );
   free( *locs );
   *locs = NULL;

/* Free the DAT__SZLOC character locators, etc. */
   F77_EXPORT_INTEGER( *status, STATUS );
   F77_CALL(irq_rlse)( CHARACTER_ARRAY_ARG(LOCS),
                       INTEGER_ARG(&STATUS) 
                       TRAIL_ARG(LOCS) );

   F77_IMPORT_INTEGER( STATUS, *status );
}

/* ------------------------------------- */

F77_SUBROUTINE(irq_new)( INTEGER(INDF),
                         CHARACTER(XNAME), 
                         CHARACTER_ARRAY(LOCS), 
                         INTEGER(STATUS)
                         TRAIL(XNAME)
                         TRAIL(LOCS) );

void irqNew( int indf, const char *xname, IRQLocs **locs, int *status ){

   DECLARE_INTEGER(INDF);
   DECLARE_CHARACTER_DYN(XNAME);  
   DECLARE_CHARACTER_ARRAY(LOCS,DAT__SZLOC,5);  
   DECLARE_INTEGER(STATUS);
   int j;

   if( !locs ) return;
   *locs= NULL;

   F77_EXPORT_INTEGER( indf, INDF );
   F77_CREATE_CHARACTER( XNAME, strlen( xname ) );
   F77_EXPORT_CHARACTER( xname, XNAME, XNAME_length );
   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(irq_new)( INTEGER_ARG(&INDF),
                      CHARACTER_ARG(XNAME),
                      CHARACTER_ARRAY_ARG(LOCS),
                      INTEGER_ARG(&STATUS) 
                      TRAIL_ARG(XNAME)
                      TRAIL_ARG(LOCS) );

   F77_FREE_CHARACTER( XNAME );
   F77_IMPORT_INTEGER( STATUS, *status );

   if( *status == SAI__OK ) {
      *locs = malloc( sizeof( IRQLocs ) );
      if( *locs ) {
         for( j = 0; j < 5; j++ ) (*locs)->loc[j] = NULL;
         for( j = 0; j < 5; j++ ) {
            HDS_IMPORT_FLOCATOR( LOCS[j], &((*locs)->loc[j]), status );
         }
      } else {
         F77_CALL(irq_rlse)( CHARACTER_ARRAY_ARG(LOCS),
                             INTEGER_ARG(&STATUS) 
                             TRAIL_ARG(LOCS) );

         if( *status == SAI__OK ) {
            *status = SAI__ERROR;
            errRep( "IRQNEW_ERR", "Cannot allocate memory for a new "
                    "IRQLocs structure.", status );
         }
      }
   }
}

/* ------------------------------- */

F77_SUBROUTINE(irq_addqn)( CHARACTER_ARRAY(LOCS), 
                           CHARACTER(QNAME), 
                           LOGICAL(DEFLT),
                           CHARACTER(COMMNT), 
                           INTEGER(STATUS)
                           TRAIL(LOCS)
                           TRAIL(QNAME)
                           TRAIL(COMMNT) );

void irqAddqn( IRQLocs *locs, const char *qname, int deflt,
               const char *commnt, int *status ){
   DECLARE_CHARACTER_ARRAY(LOCS,DAT__SZLOC,5);  
   DECLARE_CHARACTER_DYN(QNAME);  
   DECLARE_LOGICAL(DEFLT);
   DECLARE_CHARACTER_DYN(COMMNT);  
   DECLARE_INTEGER(STATUS);

   HDS_EXPORT_CLOCATOR( locs->loc[0], LOCS[0], status );
   HDS_EXPORT_CLOCATOR( locs->loc[1], LOCS[1], status );
   HDS_EXPORT_CLOCATOR( locs->loc[2], LOCS[2], status );
   HDS_EXPORT_CLOCATOR( locs->loc[3], LOCS[3], status );
   HDS_EXPORT_CLOCATOR( locs->loc[4], LOCS[4], status );

   F77_CREATE_CHARACTER( QNAME, strlen( qname ) );
   F77_EXPORT_CHARACTER( qname, QNAME, QNAME_length );
   F77_EXPORT_LOGICAL( deflt, DEFLT );
   F77_CREATE_CHARACTER( COMMNT, strlen( commnt ) );
   F77_EXPORT_CHARACTER( commnt, COMMNT, COMMNT_length );
   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(irq_addqn)( CHARACTER_ARRAY_ARG(LOCS),
                        CHARACTER_ARG(QNAME),
                        LOGICAL_ARG(&DEFLT),
                        CHARACTER_ARG(COMMNT),
                        INTEGER_ARG(&STATUS) 
                        TRAIL_ARG(LOCS) 
                        TRAIL_ARG(QNAME) 
                        TRAIL_ARG(COMMNT) );

   F77_FREE_CHARACTER( QNAME );
   F77_FREE_CHARACTER( COMMNT );
   F77_IMPORT_INTEGER( STATUS, *status );
}

/* ------------------------------- */

F77_SUBROUTINE(irq_setqm)( CHARACTER_ARRAY(LOCS), 
                           LOGICAL(BAD),
                           CHARACTER(QNAME), 
                           INTEGER(SIZE),
                           REAL_ARRAY(MASK), 
                           INTEGER(SET),
                           INTEGER(STATUS)
                           TRAIL(LOCS)
                           TRAIL(QNAME) );

void irqSetqm( IRQLocs *locs, int bad, const char *qname, int size,
               float *mask, int *set, int *status ){

   DECLARE_CHARACTER_ARRAY(LOCS,DAT__SZLOC,5);  
   DECLARE_LOGICAL(BAD);
   DECLARE_CHARACTER_DYN(QNAME);  
   DECLARE_INTEGER(SIZE);
   DECLARE_REAL_ARRAY_DYN(MASK);  
   DECLARE_INTEGER(SET);
   DECLARE_INTEGER(STATUS);

   HDS_EXPORT_CLOCATOR( locs->loc[0], LOCS[0], status );
   HDS_EXPORT_CLOCATOR( locs->loc[1], LOCS[1], status );
   HDS_EXPORT_CLOCATOR( locs->loc[2], LOCS[2], status );
   HDS_EXPORT_CLOCATOR( locs->loc[3], LOCS[3], status );
   HDS_EXPORT_CLOCATOR( locs->loc[4], LOCS[4], status );

   F77_EXPORT_LOGICAL( bad, BAD );
   F77_CREATE_CHARACTER( QNAME, strlen( qname ) );
   F77_EXPORT_CHARACTER( qname, QNAME, QNAME_length );
   F77_EXPORT_INTEGER( size, SIZE );
   F77_CREATE_REAL_ARRAY( MASK, size );
   F77_EXPORT_REAL_ARRAY( mask, MASK, size );
   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(irq_setqm)( CHARACTER_ARRAY_ARG(LOCS),
                        LOGICAL_ARG(&BAD),
                        CHARACTER_ARG(QNAME),
                        INTEGER_ARG(&SIZE),
                        REAL_ARRAY_ARG(MASK),
                        INTEGER_ARG(&SET),
                        INTEGER_ARG(&STATUS) 
                        TRAIL_ARG(LOCS) 
                        TRAIL_ARG(QNAME) );

   F77_FREE_CHARACTER( QNAME );
   F77_IMPORT_INTEGER( SET, *set );
   F77_IMPORT_INTEGER( STATUS, *status );
   F77_FREE_REAL( MASK );
}


/* ----------------------------------------------- */

F77_SUBROUTINE(kpg1_rgndf)( CHARACTER(PARAM), INTEGER(MAXSIZ), INTEGER(MINSIZ),
                            CHARACTER(TEXT), INTEGER(IGRP), INTEGER(SIZE), 
                            INTEGER(STATUS) TRAIL(PARAM) TRAIL(TEXT) );

void kpg1Rgndf( const char *param, int maxsiz, int minsiz, const char *text, 
                Grp **grp, int *size, int *status ){

   DECLARE_CHARACTER_DYN(PARAM);  
   DECLARE_INTEGER(MAXSIZ);
   DECLARE_INTEGER(MINSIZ);
   DECLARE_CHARACTER_DYN(TEXT);  
   DECLARE_INTEGER(IGRP);
   DECLARE_INTEGER(SIZE);
   DECLARE_INTEGER(STATUS);

   F77_CREATE_CHARACTER( PARAM, strlen( param ) );
   F77_EXPORT_CHARACTER( param, PARAM, PARAM_length );

   F77_EXPORT_INTEGER( maxsiz, MAXSIZ );
   F77_EXPORT_INTEGER( minsiz, MINSIZ );

   F77_CREATE_CHARACTER( TEXT, strlen( text ) );
   F77_EXPORT_CHARACTER( text, TEXT, TEXT_length );

   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(kpg1_rgndf)( CHARACTER_ARG(PARAM),
                         INTEGER_ARG(&MAXSIZ),
                         INTEGER_ARG(&MINSIZ),
                         CHARACTER_ARG(TEXT),
                         INTEGER_ARG(&IGRP),
                         INTEGER_ARG(&SIZE),
                         INTEGER_ARG(&STATUS) 
                         TRAIL_ARG(PARAM) 
                         TRAIL_ARG(TEXT) );


   F77_FREE_CHARACTER( PARAM );
   F77_FREE_CHARACTER( TEXT );
   F77_IMPORT_INTEGER( SIZE, *size );
   F77_IMPORT_INTEGER( STATUS, *status );

   *grp = grpF2C( IGRP, status );

}


/* ----------------------------------------------- */

F77_SUBROUTINE(kpg1_wgndf)( CHARACTER(PARAM), INTEGER(IGRP0), INTEGER(MAXSIZ), 
                            INTEGER(MINSIZ), CHARACTER(TEXT), INTEGER(IGRP), 
                            INTEGER(SIZE), INTEGER(STATUS) TRAIL(PARAM) 
                            TRAIL(TEXT) );

void kpg1Wgndf( const char *param, Grp *grp0, int maxsiz, int minsiz,  
                const char *text, Grp **grp, int *size, int *status ){

   DECLARE_CHARACTER_DYN(PARAM);  
   DECLARE_INTEGER(IGRP0);
   DECLARE_INTEGER(MAXSIZ);
   DECLARE_INTEGER(MINSIZ);
   DECLARE_CHARACTER_DYN(TEXT);  
   DECLARE_INTEGER(IGRP);
   DECLARE_INTEGER(SIZE);
   DECLARE_INTEGER(STATUS);

   F77_EXPORT_INTEGER( grpC2F( grp0, status ), IGRP0 );
 
   F77_CREATE_CHARACTER( PARAM, strlen( param ) );
   F77_EXPORT_CHARACTER( param, PARAM, PARAM_length );

   F77_EXPORT_INTEGER( maxsiz, MAXSIZ );
   F77_EXPORT_INTEGER( minsiz, MINSIZ );

   F77_CREATE_CHARACTER( TEXT, strlen( text ) );
   F77_EXPORT_CHARACTER( text, TEXT, TEXT_length );

   F77_EXPORT_INTEGER( *status, STATUS );

   F77_CALL(kpg1_wgndf)( CHARACTER_ARG(PARAM),
                         INTEGER_ARG(&IGRP0),
                         INTEGER_ARG(&MAXSIZ),
                         INTEGER_ARG(&MINSIZ),
                         CHARACTER_ARG(TEXT),
                         INTEGER_ARG(&IGRP),
                         INTEGER_ARG(&SIZE),
                         INTEGER_ARG(&STATUS) 
                         TRAIL_ARG(PARAM) 
                         TRAIL_ARG(TEXT) );


   F77_FREE_CHARACTER( PARAM );
   F77_FREE_CHARACTER( TEXT );
   F77_IMPORT_INTEGER( SIZE, *size );
   F77_IMPORT_INTEGER( STATUS, *status );

   *grp = grpF2C( IGRP, status );

}



