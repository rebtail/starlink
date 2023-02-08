#!/usr/bin/env python

'''
*+
*  Name:
*     tilepaste

*  Purpose:
*     Update the JSA tile collection to include data from one or more
*     NDFs.

*  Language:
*     python (2.7 or 3.*)

*  Description:
*     This script identifies the JSA tiles that overlap each of the
*     supplied NDFs, cuts each NDF up into corresponding sections and
*     coadds the NDF data into the existing tile (or creates the tile if
*     it does not already exist). The data in each final tile is the
*     weighted mean of the original tile data and the overlapping data
*     from the supplied NDFs. The reciprocal of the varianes are used as
*     weights.
*
*     For best results, the supplied NDFs should be gridded on the JSA
*     all-sky pixel grid (see parameter JSA). This avoids the need for
*     any resampling of the data. But data that is not so gridded can be
*     supplied, in which case it will be resamples onto the JSA pixel
*     grid before being coadded into the existing tiles.
*
*     The environment variable JSA_TILE_DIR should be defined prior to
*     using this command, and should hold the path to the directory in
*     which the NDFs containing the accumulated co-added data for each
*     tile are stored. Tiles for a specified instrument will be stored
*     within a sub-directory of this directory (see parameter INSTRUMENT).
*     If JSA_TILE_DIR is undefined, the current directory is used.

*  Usage:
*     tilepaste in instrument jsa [retain] [msg_filter] [ilevel] [glevel] [logfile]

*  ADAM Parameters:
*     GLEVEL = LITERAL (Read)
*        Controls the level of information to write to a text log file.
*        Allowed values are as for "ILEVEL". The log file to create is
*        specified via parameter "LOGFILE. In adition, the glevel value
*        can be changed by assigning a new integer value (one of
*        starutil.NONE, starutil.CRITICAL, starutil.PROGRESS,
*        starutil.ATASK or starutil.DEBUG) to the module variable
*        starutil.glevel. ["ATASK"]
*     ILEVEL = LITERAL (Read)
*        Controls the level of information displayed on the screen by the
*        script. It can take any of the following values (note, these values
*        are purposefully different to the SUN/104 values to avoid confusion
*        in their effects):
*
*        - "NONE": No screen output is created
*
*        - "CRITICAL": Only critical messages are displayed such as warnings.
*
*        - "PROGRESS": Extra messages indicating script progress are also
*        displayed.
*
*        - "ATASK": Extra messages are also displayed describing each atask
*        invocation. Lines starting with ">>>" indicate the command name
*        and parameter values, and subsequent lines hold the screen output
*        generated by the command.
*
*        - "DEBUG": Extra messages are also displayed containing unspecified
*        debugging information. In addition scatter plots showing how each Q
*        and U image compares to the mean Q and U image are displayed at this
*        ILEVEL.
*
*        In adition, the glevel value can be changed by assigning a new
*        integer value (one of starutil.NONE, starutil.CRITICAL,
*        starutil.PROGRESS, starutil.ATASK or starutil.DEBUG) to the module
*        variable starutil.glevel. ["PROGRESS"]
*     IN = NDF (Read)
*        A group of NDFs to be pasted into the JSA tile collection. They
*        must all have defined Variance components. Any that do not will
*        be reported and then ignored. They must all be form the same
*        JCMT instrument.
*     INSTRUMENT = LITERAL (Read)
*        Indicates the tiling scheme to be used (different instruments have
*        different tiling schemes). The default value is determined from
*        the FITS headers in the first supplied NDF. The user is prompted
*        only if a supported instrument cannot be determined from the FITS
*        headers. The following instrument names are recognised (unambiguous
*        abbreviations may be supplied): "SCUBA-2(450)", "SCUBA-2(850)",
*        "ACSIS", "DAS". NDFs containing co-added data for
*        the selected instrument reside within a corresponding sub-directory
*        of the directory specified by environment variable JSA_TILE_DIR.
*        These sub-directories are called "scuba2-450", "scuba2-850", "acsis"
*        and "das". []
*     JSA = _LOGICAL (Read)
*        TRUE if the supplied input NDFs are gridded on the JSA all-sky
*        pixel grid associated with the specified JCMT instrument. If
*        this is not the case, the NDFs are first resampled to this grid.
*        The dynamic default is True if the WCS for the first NDF uses an
*        HPX (HEALPix) projection, and False otherwise. []
*     LOGFILE = LITERAL (Read)
*        The name of the log file to create if GLEVEL is not NONE. The
*        default is "<command>.log", where <command> is the name of the
*        executing script (minus any trailing ".py" suffix), and will be
*        created in the current directory. Any file with the same name is
*        over-written. The script can change the logfile if necessary by
*        assign the new log file path to the module variable
*        "starutil.logfile". Any old log file will be closed befopre the
*        new one is opened. []
*     MSG_FILTER = LITERAL (Read)
*        Controls the default level of information reported by Starlink
*        atasks invoked within the executing script. This default can be
*        over-ridden by including a value for the msg_filter parameter
*        within the command string passed to the "invoke" function. The
*        accepted values are the list defined in SUN/104 ("None", "Quiet",
*        "Normal", "Verbose", etc). ["Normal"]
*     RETAIN = _LOGICAL (Read)
*        Should the temporary directory containing the intermediate files
*        created by this script be retained? If not, it will be deleted
*        before the script exits. If retained, a message will be
*        displayed at the end specifying the path to the directory. [FALSE]

*  Copyright:
*     Copyright (C) 2013 Science & Technology Facilities Council.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either Version 2 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
*     02110-1301, USA.

*  Authors:
*     DSB: David S. Berry (JAC, Hawaii)
*     {enter_new_authors_here}

*  History:
*     16-JUL-2013 (DSB):
*        Original version

*-
'''

import os
import shutil
import starutil
from starutil import invoke
from starutil import NDG
from starutil import Parameter
from starutil import ParSys
from starutil import msg_out

#  Assume for the moment that we will not be retaining temporary files.
retain = 0

#  A function to clean up before exiting. Delete all temporary NDFs etc,
#  unless the script's RETAIN parameter indicates that they are to be
#  retained. Also delete the script's temporary ADAM directory.
def cleanup():
   global retain
   ParSys.cleanup()
   if retain:
      msg_out( "Retaining temporary files in {0}".format(NDG.tempdir))
   else:
      NDG.cleanup()


#  Catch any exception so that we can always clean up, even if control-C
#  is pressed.
try:

#  Declare the script parameters. Their positions in this list define
#  their expected position on the script command line. They can also be
#  specified by keyword on the command line. No validation of default
#  values or values supplied on the command line is performed until the
#  parameter value is first accessed within the script, at which time the
#  user is prompted for a value if necessary. The parameters "MSG_FILTER",
#  "ILEVEL", "GLEVEL" and "LOGFILE" are added automatically by the ParSys
#  constructor.
   params = []

   params.append(starutil.ParNDG("IN", "The input NDFs",
                                 starutil.get_task_par("DATA_ARRAY","GLOBAL",
                                                       default=Parameter.UNSET)))

   params.append(starutil.ParChoice("INSTRUMENT",
                                    ["SCUBA-2(450)", "SCUBA-2(850)", "ACSIS",
                                    "DAS"],
                                    "The JCMT instrument", "SCUBA-2(850)"))

   params.append(starutil.Par0L("JSA", "Are the input NDFs on the JSA "
                                "all-sky pixel grid?", True, noprompt=True ) )

   params.append(starutil.Par0L("RETAIN", "Retain temporary files?", False,
                                 noprompt=True))

#  Initialise the parameters to hold any values supplied on the command
#  line.
   parsys = ParSys( params )

#  It's a good idea to get parameter values early if possible, in case
#  the user goes off for a coffee whilst the script is running and does not
#  see a later parameter propmpt or error...

#  Get the input NDFs. They should be supplied as the first item on
#  the command line, in the form of a Starlink "group expression" (i.e.
#  the same way they are supplied to other SMURF commands such as makemap).
#  Quote the string so that it can be used as command line argument when
#  running an atask from the shell.
   indata = parsys["IN"].value

#  See if we can determine a good default for INSTRUMENT by looking at
#  the FITS headers of the first input NDF.
   try:
      cval = starutil.get_fits_header( indata[0], "INSTRUME" ).strip()
      if cval == "SCUBA-2":
         cval = starutil.get_fits_header( indata[0], "FILTER" ).strip()

         if cval == "450":
            deflt = "SCUBA-2(450)"

         elif cval == "850":
            deflt = "SCUBA-2(850)"

         else:
            deflt = None

      else:
         cval = starutil.get_fits_header( indata[0], "BACKEND" )

         if cval == "ACSIS":
            deflt = "ACSIS"

         elif cval == "DAS":
            deflt = "DAS"

         else:
            deflt = None

   except:
      deflt = None

   if deflt is not None:
      parsys["INSTRUMENT"].default = deflt
      parsys["INSTRUMENT"].noprompt = True

#  Get the JCMT instrument. Quote the string so that it can be used as
#  a command line argument when running an atask from the shell.
   instrument = starutil.shell_quote( parsys["INSTRUMENT"].value )
   msg_out( "Updating tiles for {0} data".format(instrument) )

#  See if temp files are to be retained.
   retain = parsys["RETAIN"].value

#  Set up the dynamic default for parameter "JSA". This is True if the
#  dump of the WCS FrameSet in the first supplied NDF contains the string
#  "HPX".
   prj = invoke("$KAPPA_DIR/wcsattrib ndf={0} mode=get name=projection".format(indata[0]) )
   parsys["JSA"].default = True if prj.strip() == "HEALPix" else False

#  See if input NDFs are on the JSA all-sky pixel grid.
   jsa = parsys["JSA"].value
   if not jsa:
      msg_out( "The supplied NDFs will first be resampled onto the JSA "
               "all-sky pixel grid" )

#  Report the tile directory.
   tiledir = os.getenv( 'JSA_TILE_DIR' )
   if tiledir:
      msg_out( "Tiles will be written to {0}".format(tiledir) )
   else:
      msg_out( "Environment variable JSA_TILE_DIR is not set!" )
      msg_out( "Tiles will be written to the current directory ({0})".format(os.getcwd()) )

#  Loop round each supplied NDF. "indata" is an instance of the starutil.NDG
#  class, but "ndf" is a simple Python string.
   for ndf in indata:
      msg_out( " " )
      msg_out( "Copying data from {0}".format(ndf) )

#  Check it has a defined Variance component.
      invoke("$KAPPA_DIR/ndftrace ndf={0}".format(ndf) )
      if not starutil.get_task_par( "variance", "ndftrace" ):
          msg_out( "No Variance component found in {0} - it will be ignored".format(ndf) )
          continue

#  Indicate we have not yet selected the NDF that is aligned with the JSA
#  all-sky pixel grid.
      aligned = None

#  Get a list of the tiles that overlap the current NDF, and loop round
#  each one.
      invoke("$SMURF_DIR/jsatilelist in={0} instrument={1}".format(ndf,instrument) )
      for itile in starutil.get_task_par( "tiles", "jsatilelist" ):

#  Get the information about the tile. Ensure the directory and tile exist.
         invoke("$SMURF_DIR/jsatileinfo itile={0} instrument={1} "
                "create=yes".format(itile,instrument) )

#  Get the path to the tile's master NDF.
         tilendf = starutil.get_task_par( "tilendf", "jsatileinfo" )

#  Get a flag indicating if the tile's master NDF existed before the
#  above invocation of "jsatileinfo".
         existed = starutil.get_task_par( "exists", "jsatileinfo" )

#  Get the 2D spatial pixel index bounds of the master tile.
         tlbnd = starutil.get_task_par( "lbnd", "jsatileinfo" )
         tubnd = starutil.get_task_par( "ubnd", "jsatileinfo" )

#  If the NDFs are not gridded using the JSA all-sky grid appropriate to
#  the specified instrument, then we need to resample them onto that grid
#  before coadding the new and old data. We only need do this for the
#  first tile for each input NDF, since all tiles are aligned on the same
#  pixel grid.
         if aligned is None:
            if not jsa:
               aligned = NDG( 1 )[ 0 ]
               invoke("$KAPPA_DIR/wcsalign in={0} ref={1} out={2} lbnd=! "
                      "method=bilin".format(ndf,tilendf,aligned) )
            else:
               aligned = ndf

#  Get the pixel index bounds of the aligned NDF.
            invoke("$KAPPA_DIR/ndftrace ndf={0}".format(aligned) )
            nlbnd = starutil.get_task_par( "lbound", "ndftrace" )
            nubnd = starutil.get_task_par( "ubound", "ndftrace" )

#  Get the 2D spatial pixel index bounds of the overlap of the current tile
#  and the aligned NDF.
         olbnd = [ 1, 1 ]
         oubnd = [ 0, 0 ]
         for i in (0,1):
            if tlbnd[i] > nlbnd[i]:
               olbnd[i] = tlbnd[i]
            else:
               olbnd[i] = nlbnd[i]

            if tubnd[i] < nubnd[i]:
               oubnd[i] = tubnd[i]
            else:
               oubnd[i] = nubnd[i]

            if oubnd[i] < olbnd[i]:
               raise starutil.StarUtilError( "Expected {0} to overlap tile {1}"
                              " but it appears not to (internal programming "
                              "error).".format(ndf,itile) )

#  If the master tile exists, form the weighted mean of the existing master
#  tile and the overlap area of the aligned NDF.
         if existed:
            sec = "{0}({1}:{2},{3}:{4},)".format(aligned,olbnd[0],oubnd[0],olbnd[1],oubnd[1])
            inndf = NDG( [ tilendf, sec ] )
            outndf = NDG( 1 )
            invoke("$CCDPACK_DIR/makemos in={0} out={1} method=mean".format(inndf,outndf) )

#  Replace the old master tile with the new one.
            oldfile = "{0}.sdf".format(outndf[0])
            shutil.move( oldfile, tilendf )
            msg_out("   Updating {0}".format(tilendf) )

#  If the master tile does not yet exist, just copy a section of the aligned
#  NDF that has pixel bounds the same as the tile.
         else:
            sec = starutil.shell_quote("{0}({1}:{2},{3}:{4},)".format(aligned,tlbnd[0],tubnd[0],tlbnd[1],tubnd[1]))
            invoke("$KAPPA_DIR/ndfcopy in={0} out={1}".format(sec,tilendf) )
            msg_out("   Creating {0}".format(tilendf) )

#  Remove temporary files.
   cleanup()

#  If an StarUtilError of any kind occurred, display the message but hide the
#  python traceback. To see the trace back, uncomment "raise" instead.
except starutil.StarUtilError as err:
#  raise
   print( err )
   cleanup()

# This is to trap control-C etc, so that we can clean up temp files.
except:
   cleanup()
   raise

