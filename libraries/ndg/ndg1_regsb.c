#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "f77.h"
#include "sae_par.h"

#define GRP__NOID 0

extern F77_SUBROUTINE(grp_grpsz)( INTEGER(igrp), INTEGER(size),
                                  INTEGER(status) );
extern F77_SUBROUTINE(grp_infoc)( INTEGER(igrp), INTEGER(index),
                                  CHARACTER(item), CHARACTER(value),
                                  INTEGER(status) TRAIL(item) TRAIL(value) );
extern F77_SUBROUTINE(grp_grpex)( CHARACTER(grpexp), INTEGER(igrp1), 
                                  INTEGER(igrp2), INTEGER(size),
                                  INTEGER(added), LOGICAL(flag),
                                  INTEGER(status) TRAIL(grpexp) );
extern F77_SUBROUTINE(err_rep)( CHARACTER(param), CHARACTER(mess),
                                INTEGER(STATUS) TRAIL(param) TRAIL(mess) );

static void Error( const char *, int * );
static char *GetName( int, int, int * );
static void PutName( int, char *, int * );

F77_SUBROUTINE(ndg1_regsb)( CHARACTER(RE), INTEGER(IGRP0),
                            INTEGER(IGRP), INTEGER(SIZE), INTEGER(STATUS) 
                            TRAIL(RE) ){
/*
*  Name:
*     NDG1_RESGB

*  Purpose:

*  Description:

*  Parameters:

*  Authors:
*     DSB: David Berry (STARLINK)
*     {enter_new_authors_here}

*  History:
*     25-AUG-2000 (DSB):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*/

   GENPTR_CHARACTER(RE)
   GENPTR_INTEGER(IGRP0)
   GENPTR_INTEGER(IGRP)
   GENPTR_INTEGER(SIZE)
   GENPTR_INTEGER(STATUS)

#define BUFLEN 256

   char buf[BUFLEN];
   char *name = NULL;
   char infile_name[255];
   char outfile_name[255];
   char *script;
   char *mess;
   int i;
   int istat;
   int size;
   FILE *fd = NULL;

/* Check the global status. */
   if( *STATUS != SAI__OK ) return;

/* Get two unique temporary file names. */
   if( !tmpnam( infile_name ) || !tmpnam( outfile_name ) ){
      *STATUS = SAI__ERROR;
      Error( "Unable to create a temporary file name using \"tmpnam\".", 
              STATUS );
      return;
   } 

/* Open the file for writing. */
   if( fd ) fclose( fd );
   fd = fopen( infile_name, "w" );
   if( !fd ){
      *STATUS = SAI__ERROR;
      Error( "Unable to create a temporary file using \"fopen\".", STATUS );
      return;
   } 

/* Get the number of names to store in the file. */
   F77_CALL(grp_grpsz)( INTEGER_ARG(IGRP0), INTEGER_ARG(&size),
                        INTEGER_ARG(STATUS) );

/* Write the supplied names to the text file. */
   for( i = 1; i <= size && *STATUS == SAI__OK; i++ ){
      name = GetName( *IGRP0, i, STATUS );
      fprintf( fd, "%s\n", name );
   }

/* Close the file. */
   fclose( fd );

/* Execute the sed command, writing the results to the output file. */
   if( *STATUS == SAI__OK ){

/* Allocate memory for the sed command string. */
      script = (char *) malloc( (size_t) ( strlen( "sed -e '" )
                                           + RE_length
                                           + strlen( "' " )  
                                           + strlen( infile_name ) 
                                           + strlen( " >& " )  
                                           + strlen( outfile_name ) 
                                           + 1 ) );
      if( !script ) {
         *STATUS = SAI__ERROR;
         Error( "NDG_REGSB: Failed to allocate memory for sed command.", 
                 STATUS );

/* If OK, construct the sed command string. */
      } else {
         strcpy( script, "sed -e '" );
         strncpy( script + 8, RE, RE_length );
         strcpy( script + 8 + RE_length, "' " );
         strcpy( script + 10 + RE_length, infile_name );
         strcpy( script + strlen( script ), " >& " );
         strcpy( script + strlen( script ), outfile_name );

/* Execute the command, sending standard out and error to the output file. */
         istat = system( script );

/* Remove the input file. */
         remove( infile_name ); 

/* Deallocate the command string. */
         free( script );

/* Set STATUS and report an error if anything went wrong in the system
   call. */
         if( istat ) {
            mess = (char *) malloc( (size_t) ( strlen( "Supplied sed expression \"" )
                                           + RE_length
                                           + strlen( "\" could not be used." )  
                                           + 1 ) );
            if( !mess ) {
               *STATUS = SAI__ERROR;
               Error( "NDG_REGSB: Failed to allocate memory for error message.", 
                      STATUS );
            } else {
               *STATUS = SAI__ERROR;

               strcpy( mess, "Supplied sed expression \"" );
               strncpy( mess + 25, RE, RE_length );
               strcpy( mess + 25 + RE_length, "\" could not be used.");
               Error( mess, STATUS );

/* Attempt to copy error messages from the output file to the error system. */
               fd = fopen( outfile_name, "r" );
               if( fd ) {
                  while( fgets( buf, BUFLEN, fd ) ){
                     if( strlen( buf ) ) {
                        Error( buf, STATUS );
                     }
                  }
                  fclose( fd );
               }

               free( mess );
            }
           
            remove( outfile_name ); 
            return;
         }

/* Attempt to open the output file. */
         fd = fopen( outfile_name, "r" );

/* If succesful, extract each line of the file and append it to the
   returned group. Check for NDFs which did not match the supplied RE.
   These will be unchanged in the output file. Replace these lines with
   blanks to avoid the input NDF being used as the output NDF. */
         if( fd ) {
            i = 1;
            while( fgets( buf, BUFLEN, fd ) ){
               if( strlen( buf ) ) {
                  name = GetName( *IGRP0, i, STATUS );
                  if( strncmp( name, buf, strlen( name ) ) ){
                     PutName( *IGRP, buf, STATUS );
                  }
               }
               i++;
            }

/* Close and remove the output file. */
            fclose( fd );
            remove( outfile_name ); 

/* Get the number of names now in the returned group. */
            F77_CALL(grp_grpsz)( INTEGER_ARG(IGRP), INTEGER_ARG(SIZE),
                                 INTEGER_ARG(STATUS) );
         }         
      }
   }
}

static void Error( const char *text, int *STATUS ) {
/*
*  Name:
*     Error

*  Purpose:
*     Report an error using EMS.

*  Description:
*     The supplied text is used as the text of the error message.
*     A blank parameter name is used.

*  Parameters:
*     text
*        The error message text. Only the first 80 characters are used.
*     STATUS
*        A pointer to the global status value. This should have been set
*        to a suitable error value before calling this function.

*  Notes:
*     - If a NULL pointer is supplied for "text", no error is reported.
*/

   DECLARE_CHARACTER(param,1);
   DECLARE_CHARACTER(mess,80);
   int j;

/* Check the supplied pointer. */
   if( text ) {

/* Set the parameter name to a blank string. */
      param[0] = ' ';

/* Copy the first "mess_length" characters of the supplied message into 
      "mess". */
      strncpy( mess, text, mess_length );

/* Pad any remaining bytes with spaces (and replace the terminating null
   character with a space). */
      for( j = strlen(mess); j < mess_length; j++ ) {
         mess[ j ] = ' ';
      }

/* Report the error. */
      F77_CALL(err_rep)( CHARACTER_ARG(param), CHARACTER_ARG(mess),
                         INTEGER_ARG(STATUS) TRAIL_ARG(param) 
                         TRAIL_ARG(mess) );
   }
}

static char *GetName( int igrp, int i, int *STATUS ) {
/*
*  Name:
*     GetName

*  Purpose:
*     Gets an element out of a GRP group.

*  Description:
*     This function returns a pointer to a null-terminated C string holding 
*     an element of a supplied GRP group.

*  Parameters:
*     igrp = int (Given)
*        The GRP identifier for the group.
*     i = int (Given)
*        The index of the element to return.
*     STATUS = *int (Given and Returned)
*        The inherited status.

*  Returned Value:
*     A pointer to a static string holding the element. This string should not 
*     be modified or freed by the caller.
*     
*/

   DECLARE_CHARACTER(item,4);
   DECLARE_CHARACTER(name,256);
   DECLARE_INTEGER(IGRP);
   DECLARE_INTEGER(I);
   static char buffer[256];
   char *ret;
   int j;

/* Check the inherited status. */
   if( *STATUS != SAI__OK ) return NULL;

/* Store a Fortran string with the value "NAME" for use with GRP_INFOC. */
   item[0] = 'N';
   item[1] = 'A';
   item[2] = 'M';
   item[3] = 'E';

/* Get the name from the group. */
   IGRP = igrp;
   I = i;

   F77_CALL(grp_infoc)( INTEGER_ARG(&IGRP), INTEGER_ARG(&I),
                        CHARACTER_ARG(item), CHARACTER_ARG(name),
                        INTEGER_ARG(STATUS) TRAIL_ARG(item)
                        TRAIL_ARG(name) );

/* Replace all trailing blank characters in the returned Fortran string with 
   null characters. */
   if( *STATUS == SAI__OK ) {
      strcpy( buffer, name );
      for( j = name_length - 1; j >= 0; j-- ) {
         if( isspace( (int) buffer[j] ) ) {
            buffer[j] = 0;
         } else {
            break;
         }
      }
      ret = buffer;
   } else {
      ret = NULL;
   }

/* Return the pointer. */
   return ret;
}


static void PutName( int igrp, char *value, int *STATUS ){
/*
*  Name:
*     PutName

*  Purpose:
*     Appends an element to the end of a GRP group.

*  Description:
*     This function 

*  Parameters:
*     igrp = int (Given)
*        The GRP identifier for the group.
*     value = char * (Given)
*        The text to store (null terminated).
*     STATUS = *int (Given and Returned)
*        The inherited status.
*/

   DECLARE_CHARACTER(text,200);
   DECLARE_INTEGER(IGRP1);
   DECLARE_INTEGER(IGRP2);
   DECLARE_INTEGER(SIZE);
   DECLARE_INTEGER(ADDED);
   DECLARE_LOGICAL(FLAG);
   int j;

/* Check the inherited status. */
   if( *STATUS != SAI__OK ) return;

/* Copy the supplied value into "text". */
   strcpy( text, value );

/* If the last character is a newline, replace it with a null. */
   if( text[ strlen( text ) - 1 ] == '\n' ) {
      text[ strlen( text ) - 1 ] = 0;
   } 

/* Pad any remaining bytes with spaces (and replace the terminating null
   character with a space). */
   for( j = strlen( text ); j < text_length; j++ ) {
      text[ j ] = ' ';
   }

   IGRP1 = GRP__NOID;
   IGRP2 = igrp;

   F77_CALL(grp_grpex)( CHARACTER_ARG(text), INTEGER_ARG(&IGRP1),
                        INTEGER_ARG(&IGRP2), INTEGER_ARG(&SIZE), 
                        INTEGER_ARG(&ADDED), LOGICAL_ARG(&FLAG), 
                        INTEGER_ARG(STATUS) TRAIL_ARG(text) );
}
