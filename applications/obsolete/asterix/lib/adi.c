/*
 *    Description :
 *
 *     The ADI data system provides class oriented data storage to client
 *     software.
 *
 *    Method :
 *
 *     ADI communicates with clients using object tokens which are usually
 *     declared as integers in the client language. A token indexes a table
 *     of object handles. These store,
 *
 *       a) The object class id
 *       b) The location of the object's instance data
 *       c) A reference count
 *       d) Dependency information
 *       e) Various flags
 *
 *     Because the token used to reference an object provides two levels of
 *     indirection, ADI can reallocate the object data space with affecting
 *     client software.
 *
 *
 *    Implementation notes :
 *
 *     - ADI Object Structure
 *
 *     - ADI Object Allocation
 *
 *       Object allocation is performed using basic blocks. A BB is a
 *       block of memory with a header, which stores the data required
 *       for a number of objects of the same type. BB construction is
 *       controlled by a BB allocator, one of which exists for each
 *       type.
 *
 *       Have a 'last deleted' object for fast reallocation. ok for all
 *       objects bigger than ADIobj.
 *
 *     - Memory Transfers
 *
 *       Controlled using the MTA object. An MTA describes a conceptual
 *       block of data, which need not be contiguous in process memory.
 *       The MTA has 2 sets of dimensional information. The 'ddims' field
 *       describes the process memory space which bounds the subject data.
 *       The 'udims' field describes the data used within this volume,
 *       with 'uorig' describing the N-dimensional origin of this sub-
 *       volume. The MTA data structure contains other information, the
 *       base type and size in bytes of each data element.
 *
 *       A complication arises when referring to character strings. The
 *       natural mode for the C programmer to supply an array of character
 *       strings is as an array of char *. In Fortran however, and for C
 *       when strings are the output form, a 2-d array of characters is
 *       more natural. Thus, the convention is that when an MTA has an
 *       element size of _CSM then the data item refers to char **,
 *       otherwise simply char *. The second complication is that ADI
 *       arrays of strings are stored in neither of these forms, but as
 *       an array of ADIsegment's.
 *
 *       ** slices **
 *
 *     - Arrays
 *
 *     - Method combination forms
 *
 *     - Generic Functions
 *
 *    To test :
 *
 *     Check C <-> Fortran logical translation
 *     Character array copying ok?
 *
 *    To be done :
 *
 *     remove vector behaviour from destructors
 *     construct call in cls_alloc rather than wdata?
 *     parser grammar should be loadable
 *     instantiate member constants in cls_alloc
 *     dodgy putting kernel objects as data members - can't erase
 *     support C and Fortran array indexing
 *     checks to prevent illegal conversion <-> pointer type
 *     exit handlers
 *     vector ops, eg. cput0i( id, "a,b,c", 23, status )
 *     Memory leaks
 *     Public hash table interface
 *     Incorporate expression parsing code
 *     Data packing and stream representation of ADI data
 *     Iterators and locking
 *     Special non-recursive list destructor
 *     Remove malloc/free from adix_mark/rlse
 *     Methods
 *     Lock access when object is mapped
 *     Contexts   - provide hierarchy of scopes, and ability to destroy
 *                  all objects above a high water mark
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "asterix.h"

#include "aditypes.h"
#include "adikrnl.h"
#include "adimem.h"
#include "adilist.h"
#include "adistrng.h"
#include "adiparse.h"
#include "adicface.h"
#include "aditable.h"
#include "adifsys.h"
#include "adisyms.h"
#include "adierror.h"
#include "adiarray.h"

/*  Forward definitions
 *
 */
void adix_delgen( ADIobj id, int nval, ADIstatus status );
void adix_delhan( ADIobj id, int nval, ADIstatus status );
void adix_delmapctrl( ADIobj id, int nval, ADIstatus status );
void adix_delmco( ADIobj id, int nval, ADIstatus status );
void adix_delobj( ADIobj id, int nval, ADIstatus status );
ADIobj adix_stdmcf( ADIobj, int, ADIobj [], ADIstatus status );
ADIobj adix_exemth( ADIobj generic, ADIobj method,
		    int narg, ADIobj args[], ADIstatus status );
ADIobj adix_locgen( ADIobj name, int narg, ADIstatus status );

/* Declare kernel data types
 *
 */
_DEF_STATIC_CDEF("_SymbolBinding",sbind,128,NULL,NULL);
_DEF_STATIC_CDEF("_MappingControl",mapctrl,8,adix_delmapctrl,NULL);
_DEF_STATIC_CDEF("_Method",mthd,48,NULL,NULL);
_DEF_STATIC_CDEF("_MethodCombinationForm",mco,8,adix_delmco,NULL);
_DEF_STATIC_CDEF("_GenericFunction",gnrc,24,adix_delgen,NULL);
_DEF_STATIC_CDEF("_ExternalProcedure",eprc,48,NULL,NULL);
_DEF_STATIC_CDEF("_SuperclassRecord",pdef,48,NULL,NULL);
_DEF_STATIC_CDEF("_MemberRecord",mdef,48,NULL,NULL);
_DEF_STATIC_CDEF("_MemoryTransfer",mta,8,NULL,NULL);
_DEF_STATIC_CDEF("_ClassDeclaration",cdef,16,NULL,NULL);
_DEF_STATIC_CDEF("_ObjectHandle",han,512,adix_delhan,NULL);
_DEF_STATIC_CDEF("_ObjectReference",obj,512,adix_delobj,NULL);


static int              ADI_G_ntyp = 0; /* Class code counter */

ADIobj       UT_ALLOC_b;
ADIobj       UT_ALLOC_ub;
ADIobj       UT_ALLOC_w;
ADIobj       UT_ALLOC_uw;
ADIobj       UT_ALLOC_i;
ADIobj       UT_ALLOC_r;
ADIobj       UT_ALLOC_d;
ADIobj       UT_ALLOC_l;
ADIobj       UT_ALLOC_c;
ADIobj       UT_ALLOC_p;
ADIobj       UT_ALLOC_struc;

ADIobj		ADI_G_grplist = ADI__nullid;

ADIlogical      ADI_G_init = ADI__false;
ADIlogical      ADI_G_init_failed = ADI__false;

static ADIobj       ADI_G_commonstrings = ADI__nullid;

typedef
  struct ADIctxTag *ADIctxPtr;

typedef
  struct ADIctxTag {
    ADIobj              onexitlist;
    ADIctxPtr           last;
    }
  ADIctx;

/* We maintain a linked list of class definition structures. ADI_G_firstcdef
 * holds the address of the first class definition structure. Subsequent
 * links are stored in the class structures.
 */
static ADIclassDef    *ADI_G_firstcdef = NULL;
static ADIclassDef    **ADI_G_cdeflink = &ADI_G_firstcdef;

/* ...and for method combination forms
 */
static ADIobj         ADI_G_firstmco = ADI__nullid;
static ADIobj         *ADI_G_mcolink = &ADI_G_firstmco;

/* ...and for generic functions
 */
static ADIobj         ADI_G_firstgnrc = ADI__nullid;
static ADIobj         *ADI_G_gnrclink = &ADI_G_firstgnrc;

/* ...and for method functions
 */
static ADIobj         ADI_G_firstmthd = ADI__nullid;
static ADIobj         *ADI_G_mthdlink = &ADI_G_firstmthd;


/* Add a cell to an object list such as the above. Macro works on the list
 * insertion point and the new object.
 */
#define _LST_APPEND(_list,_id) \
     {ADIobj ncell = lstx_cell( _id,ADI__nullid, status );\
      *_list = ncell; _list = &_CDR(ncell); }

/*
 * The context list
 */
static ADIctx    ADI_G_basectx = {ADI__nullid,NULL};
static ADIctxPtr ADI_G_curctx = &ADI_G_basectx;



/*
 * Standard frequently used names
 */
ADIobj DnameAfter       = ADI__nullid;
ADIobj DnameAround      = ADI__nullid;
ADIobj DnameBefore      = ADI__nullid;
ADIobj DnamePrimary     = ADI__nullid;
ADIobj DnameNewLink     = ADI__nullid;
ADIobj DnameSetLink     = ADI__nullid;
ADIobj DnameUnLink      = ADI__nullid;

ADIobj ADIcvFalse	= ADI__nullid;
ADIobj ADIcvTrue	= ADI__nullid;
ADIobj ADIcvNulCons	= ADI__nullid;
ADIobj ADIcvStdOut	= ADI__nullid;

static ADIobj   ADI_G_stdmcf = ADI__nullid;


/*
 * Adjust name length variable to ignore trailing white space
 */
void adix_ntrunc( char *name, int *len )
  {
  char *nptr = name + (*len) - 1;

  while( (*len) && isspace(*nptr) ) {
    (*len)--; nptr--;
    }
  }

void adix_acc2tok( char *tok, ADIacmode mode )
  {
  char 		*aname;

  switch( mode ) {
    case ADI__read:
      aname = "READ";
      break;
    case ADI__write:
      aname = "WRITE";
      break;
    case ADI__update:
      aname = "UPDATE";
      break;
    }

  adic_setetc( tok, aname, 99 );
  }


/*
 *  Allocate a block of objects. Note that the object constructor is not
 *  called here, so unless other action is taken by the caller invalid
 *  data can be created if (for example) the data object is primitive and
 *  contains pointers. If however the type is primitive, and a data
 *  initialiser has been supplied, then that will be copied.
 */
ADIobj adix_cls_nalloc( ADIclassDef *cdef, int ndim, int dims[], ADIstatus status )
  {
  ADIblockCtrlPtr	actrl = &cdef->alloc;
  ADIobj                data;           /* Raw data block id */
  int                   nelm = 1;       /* Total number of elements needed */
  ADIobj                rval = ADI__nullid;

  _chk_stat_ret(ADI__nullid);           /* Check status on entry */

  if ( ! actrl->size ) {                /* Zero size? Must be abstract */
    adic_setetc( "CLS", cdef->name, _CSM );
    adic_setecs( ADI__ILLOP, "Pure abstract class ^CLS cannot be instantiated", status );
    }

  else {

/* Array of items? */
    if ( ndim ) {

/* Count number of elements */
      nelm = ADIaryCountNelm( ndim, dims );

/* Allocate the data segment */
      data = adix_bb_nalloc( actrl, nelm, status );

/* Create array descriptor and wrap it in a handle */
      rval = ADIaryNew( ndim, dims, data, ADI__nullid, status );
      }
    else {

/* Allocate single element */
      rval = adix_bb_nalloc( actrl, 1, status );
      }

/* Wrap in handle if non-kernel */
    if ( ! cdef->kernel )
      rval = adix_newhan( rval, ADI__false, status );
    }

/* Allocation went ok? */
  if ( _ok(status) ) {

/* Primitive and initialiser there? */
    if ( cdef->prim && cdef->pdata ) {
      int	  i,j;
      char	  *dat = (char *) _DTDAT(rval);

      for( i=nelm; i; i-- ) {
	char *pdata = cdef->pdata;

	for( j=actrl->size; j; j-- )
	  *dat++ = *pdata++;
	}

/*    _han_set(rval) = ADI__true;     *//* Initialised objects are "set" */
      }

/* Class instance? */
    else if ( ! (cdef->kernel || cdef->prim ) ) {
      int       i;
      ADIobj    *optr = (ADIobj *) _DTDAT(rval);

/* Member initialisations to be performed? */
      if ( cdef->meminit ) {
	ADIobj 	curmem = cdef->members;

	for( i=0; i<cdef->nslot*nelm; i++, optr++ ) {
	  ADIobj cmem = _mdef_defcls(curmem);

	  if ( _valid_q(cmem) )
	    *optr = adix_cls_alloc( _cdef_data(cmem), status );
	  else
	    *optr = ADI__nullid;

	  curmem = _mdef_next(curmem);
	  }
	}

      else if ( cdef->nslot )
	for( i=cdef->nslot*nelm; i; i-- )
	  *optr++ = ADI__nullid;

/* Class instance is always set */
      _han_set(rval) = ADI__true;
      }
    }

  return rval;                          /* Return new object */
  }

ADIobj adix_cls_alloc( ADIclassDef *cdef, ADIstatus status )
  {
  return adix_cls_nalloc( cdef, 0, NULL, status );
  }

void adix_erase( ADIobj *id, int nval, ADIstatus status )
  {
  ADIclassDefPtr        tdef;           /* Class definition data */

  if ( !_ok(status) )           	/* Check status on entry */
    return;

  if ( _valid_q(*id) )                  /* Valid handle? */
    {
    tdef = _ID_TYPE(*id);               /* Locate class definition block */

    if ( tdef->destruc )                /* Destructor defined? */
      (*tdef->destruc)( *id, nval, status );

    else if ( ! tdef->prim )            /* Class instance? */
      {
      ADIobj    *optr = _class_data(*id);
      int       i;

      for( i=0; i<tdef->nslot; i++, optr++ )
	if ( _valid_q(*optr) )
	  adix_erase( optr, 1, status );
      }

    if ( *status == ADI__NOTDEL ) 	/* Didn't delete data? */
      *status = SAI__OK;
    else
      adix_bb_free( id, nval, status ); /* Free objects */
    }
  }


void adix_delobj( ADIobj id, int nval, ADIstatus status )
  {
  int           i;
  KT_CTYPE_obj  *optr = _obj_data(id);

  for( i=0; i<nval; i++, optr++ )
    if ( _valid_q(*optr) )
      adix_erase( optr, 1, status );
  }


void adix_delmapctrl( ADIobj id, int nval, ADIstatus status )
  {
  int           i;
  KT_CTYPE_mapctrl  *mptr = _mapctrl_data(id);

  for( i=0; i<nval; i++,mptr++ ) {
    if ( mptr->dynamic )
      adix_mem_free( mptr->dptr, mptr->nbyte, status );
    }
  }


void adix_delhan( ADIobj id, int nval, ADIstatus status )
  {
  int           i;
  KT_CTYPE_han  *hptr = _han_data(id);

  for( i=0; i<nval; i++, hptr++ ) {
    hptr->ref--;                        /* Decrement reference count */

    if ( hptr->ref )                  	/* Outstanding references? */
      *status = ADI__NOTDEL;
    else {
      if ( hptr->slice ) {		/* Is this a slice? */
	if ( _ary_q(hptr->id) )		/* Vector slice? */
	  adix_erase( &hptr->id, 1,	/* Delete array block ignoring data */
		      status );
	}
      else
	adix_erase( &hptr->id, 1,	/* Not sliced data */
			status );

      if ( _valid_q(hptr->pl) )         /* Property list defined? */
	adix_erase( &hptr->pl, 1, status );
      }
    }
  }


ADIobj adix_newhan( ADIobj id, ADIlogical slice, ADIstatus status )
  {
  ADIobjHanPtr  hdata;                  /* Pointer to handle data */
  ADIobj newh = ADI__nullid;

/* Allocate new handle */
  newh = adix_cls_alloc( &KT_DEFN_han, status );

  if ( _ok(status) ) {                  /* Allocated ok? */
    hdata = _han_data(newh);            /* Locate data block */

    hdata->id = id;                     /* Store object reference */

    hdata->pl = ADI__nullid;            /* No properties */
    hdata->pid = ADI__nullid;		/* No parent defined */
    hdata->lock = ADI__nullid;		/* No object locks */
    hdata->ref = 1;                     /* Initialise the reference count */

    hdata->markdel = ADI__false;        /* Not marked for delete */
    hdata->readonly = ADI__false;       /* Dynamic object by default */
    hdata->dataset = ADI__false;        /* Data not set yet */
    hdata->slice = slice;          	/* Object is a slice? */
    }

  return newh;                          /* Return new handle */
  }



char *adix_dtdat( ADIobj id )
  {
  if ( _han_q(id) ) {
    return _DTDAT(_han_id(id));
    }
  else if ( _ary_q(id) ) {
    ADIarrayPtr      aptr = _ary_data(id);

    return _ID_DATA(aptr->data);
    }
  else
    return _ID_DATA(id);
  }


ADIclassDefPtr adix_dtdef( ADIobj id )
  {
  ADIarrayPtr           ad;
  ADIobjHanPtr          hd;
  ADIclassDefPtr        allc;
  ADIclassDef		*bcls = _ID_BLOCK(id)->cdef;

  if ( bcls == &KT_DEFN_han ) {
    hd = _han_data(id);
    allc = _DTDEF(hd->id);
    }
  else if ( bcls == &KT_DEFN_ary ) {
    ad = _ary_data(id);
    allc = _DTDEF(ad->data);
    }
  else
    allc = _ID_TYPE(id);

  return allc;
  }


ADIobj adix_new_cdef( char *name, int nlen, ADIlogical checkonly,
		      ADIclassCode *t, ADIstatus status )
  {
  ADIclassDefPtr        tdef;           /* New definition */
  ADIobj                typid;          /* Object identifier */

  if ( !_ok(status) )                   /* Check status on entry */
    return ADI__nullid;

  _GET_NAME(name,nlen);               	/* Import string */

/* New class definition structure */
  typid = adix_cls_alloc( &KT_DEFN_cdef, status );

  if ( checkonly )                      /* Type number externally defined ? */
    ADI_G_ntyp = _MAX(ADI_G_ntyp,*t);
  else {
    ADI_G_ntyp++;                         /* Increment type counter */
    *t = ADI_G_ntyp;
    }

  tdef = _cdef_data(typid);             /* Allocate memory for definition */

  tdef->name = strx_dupl( name, nlen ); /* Duplicate name */

  if ( _valid_q(ADI_G_commonstrings) )  /* Insert name in common table? */
    tdef->aname =
      adix_cmnC( tdef->name, status );

  tdef->destruc = NULL;                 /* No destructor by default */
  tdef->prnt = NULL;
  tdef->mcon = NULL;

  tdef->link = NULL;                    /* Add to linked list */
  *ADI_G_cdeflink = tdef;
  ADI_G_cdeflink = &tdef->link;

  tdef->nslot = 0;                      /* Starting values */
  tdef->members = ADI__nullid;
  tdef->superclasses = ADI__nullid;
  tdef->dslist = ADI__nullid;
  tdef->defmem = DEF_MEMBER_FLAG_VALUE;

  tdef->pdata = NULL;

  return typid;
  }


/* Create a new external procedure object
 */
ADIobj adix_neweprc( ADIlogical is_c, ADICB func, ADIstatus status )
  {
  ADIobj        newid = ADI__nullid;    /* The new object */

  if ( func ) {                         /* Function is defined? */

/* New external procedure object */
    newid = adix_cls_alloc( &KT_DEFN_eprc, status );

    if ( _ok(status) ) {                /* Fill in data slots */
      _eprc_prc(newid) = func;
      _eprc_c(newid) = is_c;
      }
    }

  return newid;                         /* Return the new object */
  }


/*
 * Define a primitive class
 *
 */
void adix_def_pclass( char *name, ADIclassCode t,
		      size_t size, ADIobj *tid, ADIstatus status )
  {
  ADIclassDefPtr        tdef;           /* New class definition */

  if ( !_ok(status) )                   /* Check status on entry */
    return;

  *tid = adix_new_cdef( name, _CSM,     /* Allocate new class storage */
	(t!=0), &t, status );
  tdef = _cdef_data(*tid);

  adix_bb_init( &tdef->alloc, t, size,  /* Initialise basic block control */
		ADI__EXHANTAB, *tid, status );

  tdef->prim = ADI__true;
  }


void adix_def_pclass_data( ADIclassDefPtr tdef, char *data, ADIstatus status )
  {
  _chk_stat;
  tdef->pdata = data;
  }


void ADIdefClassMakeDlist( ADIclassDefPtr tdef, ADIstatus status )
  {
  ADIobj	curp;
  ADIobj	*ipoint = &tdef->dslist;

  _chk_stat;

/* Superclasses persent? */
  if ( _valid_q(tdef->superclasses) ) {

/* Put class name at head of list */
    *ipoint = lstx_cell( tdef->aname, ADI__nullid, status );
    ipoint = & _CDR(*ipoint);

/* Each each direct superclass name in turn */
    curp = tdef->superclasses;
    while ( _valid_q(curp) ) {
      *ipoint = lstx_cell( _pdef_name(curp), ADI__nullid, status );
      ipoint = & _CDR(*ipoint);

      curp = _pdef_next(curp);
      }
    }
  }


void ADIdefClassConvertNames( ADIclassDefPtr tdef, ADIstatus status )
  {
  ADIobj        curmem;

  _chk_stat;

  for( curmem = tdef->members; _valid_q(curmem); curmem = _mdef_next(curmem) )
    _mdef_aname(curmem) = adix_cmn( _mdef_name(curmem),
			     _mdef_nlen(curmem), status );
  }


/* A bunch of functions which help to create the class precedence list */

ADIobj adix_cons_pairs_aux( ADIobj lst, ADIstatus status )
  {
  ADIobj        rval;

  _chk_stat_ret(ADI__nullid);

  if ( _null_q(_CDDR(lst)) ) {
    rval = lstx_cell( lst, ADI__nullid, status );
    }
  else {
    rval = lstx_cell( lstx_new2( _CAR(lst), _CADR(lst), status ),
		      adix_cons_pairs_aux( _CDR(lst), status ),
		      status );
    }

  return rval;
  }


ADIobj adix_cons_pairs( ADIobj lst, ADIstatus status )
  {
  _chk_stat_ret(ADI__nullid);

  return adix_mapcar1( adix_cons_pairs_aux,
		       lstx_append, lst, status );
  }


ADIlogical adix_filt_classes_mtest( ADIobj x, ADIobj y )
  {
  return (x == _CADR(y)) ? ADI__true : ADI__false;
  }


ADIobj adix_filt_classes( ADIobj classes, ADIobj ppairs,
			  ADIstatus status )
  {
  ADIobj        cls;
  ADIobj        curp = classes;
  ADIobj        rval = ADI__nullid;

  _chk_stat_ret(ADI__nullid);

  while ( _valid_q(curp) && _ok(status) )
    {
    cls = _CAR(curp);

    if ( ! adix_member( cls, ppairs, adix_filt_classes_mtest, status ) )
      lstx_push( cls, &rval, status );

    curp = _CDR(curp);                  /* Next test class */
    }

  return rval;
  }


ADIobj adix_filt_cands( ADIobj candidates, ADIobj plist,
		      ADIobj dsupers, ADIstatus status )
  {
  ADIobj rval = ADI__nullid;

  _chk_stat_ret(ADI__nullid);

/* List is of length one? */
  if ( _null_q(_CDR(candidates)) )
    rval = _CAR(candidates);

  else
    {
    ADIobj cursub = plist;              /* Loop over possible subclasses */
    ADIobj sub;
    while ( _valid_q(cursub) && _null_q(rval))
      {
      ADIobj curcan = candidates;       /* Loop over candidates */
      ADIobj can;

      sub = _CAR(cursub);

      while ( _valid_q(curcan) )
	{
	can = _CAR(curcan);

	if ( adix_member( can,
			  adix_assoc( sub, dsupers, status ),
			  NULL, status ) )
	  {
	  rval = can;
	  break;
	  }

	curcan = _CDR(curcan);
	}

      cursub = _CDR(cursub);
      }
    }

  return rval;
  }


ADIlogical adix_filt_pairs_test( ADIobj x, ADIobj args )
  {
  return (args == _CAR(x)) ? ADI__true : ADI__false;
  }


ADIobj adix_filt_pairs( ADIobj ppairs, ADIobj winner, ADIstatus status )
  {
  _chk_stat_ret(ADI__nullid);

  return adix_removeif( adix_filt_pairs_test, winner, ppairs, status );
  }


/*
 * Establish the precedence order of a set of classes. The classes to
 * be ordered form the first argument. The second is the list of all
 * direct superclass relationships, ie. for each class a list of that
 * classes direct superclasses.
 */
ADIobj adix_estab_ord( ADIobj classes, ADIobj dsupers, ADIstatus status )
  {
  ADIobj        ppairs;
  ADIobj        preclst = ADI__nullid;  /* Precedence list */
  ADIobj        curcls = classes;       /* Current class list */

  _chk_stat_ret(ADI__nullid);

  ppairs = adix_cons_pairs( dsupers,    /* Make precedence pairs */
			    status );

  while ( _valid_q(curcls) &&           /* While more classes to process */
	  _ok(status) )
    {
    ADIobj      cands;
    ADIobj      winner;

    cands = adix_filt_classes( curcls,  /* Next lot of candidates */
		     ppairs, status );

    winner = adix_filt_cands( cands,
	 preclst, dsupers, status );

    ppairs = adix_filt_pairs( ppairs,
			 winner, status );

    curcls = adix_removeif( NULL, winner,
			curcls, status );

    lstx_push( winner, &preclst, status );
    }

  return lstx_revrsi( preclst, status );/* Return the precedence list */
  }


void adix_delgen( ADIobj id, int nval, ADIstatus status )
  {
  KT_CTYPE_gnrc         *gdata = _gnrc_data(id);
  int                   i;

  for( i=0; i<nval; i++, gdata++ )
    {
    adix_erase( &_gnrc_name(id), 1, status );
    adix_erase( &_gnrc_args(id), 1, status );
    adix_erase( &_gnrc_cdisp(id), 1, status );
    adix_erase( &_gnrc_fdisp(id), 1, status );
    }
  }

void adix_defgdp( ADIobj genid, ADIobj disp, ADIstatus status )
  {
  _chk_stat;

  if ( _eprc_c(disp) )
    _gnrc_cdisp(genid) = disp;
  else
    _gnrc_fdisp(genid) = disp;
  }

ADIobj adix_defgen_i( ADIobj name, int narg, ADIobj args, ADIobj mcomb,
		      ADIstatus status )
  {
  ADIobj	newid;			/* The new object */

  _chk_stat_ret(ADI__nullid);

  newid = adix_cls_alloc( &KT_DEFN_gnrc, status );

  if ( _ok(status) ) {               	/* Set generic record fields */
    _gnrc_name(newid) = name;
    _gnrc_narg(newid) = narg;
    _gnrc_args(newid) = args;
    _gnrc_mcomb(newid) = mcomb;
    _gnrc_cdisp(newid) = ADI__nullid;
    _gnrc_fdisp(newid) = ADI__nullid;
    _gnrc_mlist(newid) = ADI__nullid;

    _LST_APPEND(ADI_G_gnrclink,newid);  /* Append to system list */
    }

  return newid;
  }


void adix_defgen( char *spec, int slen, char *options, int olen,
		  ADIobj rtn, ADIobj *id, ADIstatus status )
  {
  ADIobj        args = ADI__nullid;     /* Generic argument list */
  ADIobj        *ainsert = &args;       /* Arg list insertion point */
  ADIobj        aname;                  /* An argument name */
  ADItokenType  ctok;                   /* Current token in parse stream */
  ADIobj        gname;                  /* New generic name */
  ADIobj        mcomb = ADI_G_stdmcf;   /* Method combination form */
  int           narg = 0;               /* Number of generic arguments */
  ADIobj        newid;                  /* The new generic function */
  ADIobj        pstream;                /* Parse stream */

  _chk_stat;                            /* Check status on entry */

  _GET_NAME(spec,slen);               	/* Import strings used in this rtn */
  _GET_NAME(options,olen);

/* Put specification into parse stream */
  pstream = ADIstrmExtendC( ADIstrmNew( status ), spec, slen, status );

  ctok = ADInextToken( pstream, status );

  if ( ctok == TOK__SYM ) {             /* Must be generic name */

/* Get identifier for generic name in common table */
    gname = prsx_symname( pstream, status );

    ctok = ADInextToken( pstream, status );

    if ( ctok == TOK__LPAREN ) {
      ctok = ADInextToken( pstream, status );

/* While more generic arguments */
      while ( (ctok==TOK__SYM) && _ok(status) ) {

/* Locate argument name in table */
	aname = prsx_symname( pstream, status );
	narg++;

	_LST_APPEND(ainsert,aname);     /* Insert into argument list */

	ctok = ADInextToken( pstream, status );

/*     Comma delimits argument names */
	if ( ctok == TOK__COMMA )
	  ctok = ADInextToken( pstream, status );
	else if ( ctok != TOK__RPAREN ) {
	  adic_setecs( ADI__INVARG, "Syntax error in generic argument list", status );
	  }
	}
      }
    }
  else
    adic_setecs( ADI__INVARG, "Generic name expected", status );

/* Free the parsing stream */
  adic_erase( &pstream, status );

  if ( _ok(status) ) {                  /* Parsing went ok? */
    newid = adix_defgen_i( gname, narg,	/* Allocate space for new generic */
		 args, mcomb, status );

    if ( _valid_q(rtn) )                /* Routine supplied? */
      adix_defgdp( newid, rtn, status );/* Install it */

    if ( id )                           /* User wants the identifier? */
      *id = newid;                      /* Return identifier */
    }
  }


void adix_defmth( char *spec, int slen,
		  ADIobj rtn, ADIobj *id, ADIstatus status )
  {
  ADIobj        args = ADI__nullid;     /* Generic argument list */
  ADIobj        *ainsert = &args;       /* Arg list insertion point */
  ADIobj        aname;                  /* An argument name */
  ADItokenType  ctok;                   /* Current token in parse stream */
  ADIobj        gname;                  /* New generic name */
  ADIobj        mform = DnamePrimary; /* Method form */
  int           narg = 0;               /* Number of generic arguments */
  ADIobj        newid;                  /* The new generic function */
  ADIobj     	pstream;                /* Parse stream */

  _chk_stat;                            /* Check status on entry */

  _GET_NAME(spec,slen);               	/* Import strings used in this rtn */

/* Put specification into parse stream */
  pstream = ADIstrmExtendC( ADIstrmNew( status ), spec, slen, status );

  ctok = ADInextToken( pstream, status );

/* Check for prefix character denoting non-primary method forms */
  if ( ctok == TOK__PLUS )
    mform = DnameAfter;
  else if ( ctok == TOK__MINUS )
    mform = DnameBefore;
  else if ( ctok == TOK__AT )
    mform = DnameAround;
  if ( mform != DnamePrimary )
    ctok = ADInextToken( pstream, status );

  if ( ctok == TOK__SYM ) {             /* Must be method name */
    gname = prsx_symname( pstream,     /* Get common string identifier */
			  status );

    ctok = ADInextToken( pstream, status );

    if ( ctok == TOK__LPAREN ) {
      ctok = ADInextToken( pstream, status );

      while ( (ctok==TOK__SYM) &&       /* While more arguments */
	       _ok(status) ) {
	aname = prsx_symname( pstream, /* Get common string identifier */
			      status );
	narg++;

	_LST_APPEND(ainsert,aname);     /* Insert into argument list */

	ctok = ADInextToken( pstream, status );

/*     Comma delimits argument names */
	if ( ctok == TOK__COMMA )
	  ctok = ADInextToken( pstream, status );
	else if ( ctok != TOK__RPAREN )
	  adic_setecs( ADI__INVARG,
		       "Syntax error in method argument list", status );
	}
      }
    }
  else
    adic_setecs( ADI__INVARG, "Method name expected", status );

/* Release the parse stream */
  adic_erase( &pstream, status );

  if ( _ok(status) ) {                  /* Parsing went ok? */
    ADIobj	gnid;			/* Generic function identifier */

    gnid = adix_locgen( gname, narg,	/* Look for generic function */
			status );
    if ( _null_q(gnid) )
      gnid = adix_defgen_i( gname, narg,/* Allocate space for new generic */
		 ADI__nullid,		/* with null argument names and */
		 ADI_G_stdmcf, status );/* standard method combination */

    newid = adix_cls_alloc( &KT_DEFN_mthd, status );

    _mthd_name(newid) = gname;          /* Set method record fields */
    _mthd_args(newid) = args;
    _mthd_form(newid) = mform;
    _mthd_exec(newid) = rtn;

    _gnrc_mlist(gnid) =			/* Add method to generic's list */
	lstx_cell( newid,
	  _gnrc_mlist(gnid), status );

    _LST_APPEND(ADI_G_mthdlink,newid);  /* Append to system list */

    if ( id )                           /* User wants identifier? */
      *id = newid;                      /* Return identifier */
    }
  }


ADIobj ADIdefClassLocMember( ADIobj memlist, char *name, int nlen,
			     ADIstatus status )
  {
  ADIobj	curm = memlist;
  ADIobj	found = ADI__false;

  while ( _valid_q(curm) && !found ) {
    KT_CTYPE_mdef     *mdata = _mdef_data(curm);

    if ( ! strx_cmp2c( mdata->name, mdata->nlen, name, nlen ) )
      found = ADI__true;
    else
      curm = mdata->next;
    }

  return found ? curm : ADI__nullid;
  }


ADIobj ADIdefClassCopyMember( ADIobj pmem, ADIobj *members,
			      ADIobj **ipoint, ADIstatus status )
  {
  ADIobj        newid;                  /* The new member definition */
  ADImemberDef	*pdata = _mdef_data(pmem);

  _chk_stat_ret(ADI__nullid);           /* Check status on entry */

/* Allocate storage for new member */
  newid = adix_cls_alloc( &KT_DEFN_mdef, status );

/* Store name and length in characters */
  _mdef_name(newid) = strx_dupl( pdata->name, pdata->nlen );
  _mdef_nlen(newid) = pdata->nlen;
  _mdef_aname(newid) = pdata->aname;
  adix_refadj( pdata->aname, 1, status );
  _mdef_defcls(newid) = pdata->defcls;

/* Inherit any constant data */
  _mdef_cdata(newid) = pdata->cdata;
  if ( _valid_q(pdata->cdata) )
    adix_refadj( pdata->cdata, 1, status );

  _mdef_next(newid) = ADI__nullid;

/* Insert new object into list and update insertion point */
  **ipoint = newid;
  *ipoint = &_mdef_next(newid);

/* Return new object */
  return newid;
  }


ADIobj ADIdefClassNewMember( ADIobj pstr, ADIobj *members,
			     ADIobj **ipoint, ADIstatus status )
  {
  ADIobj      	emem = ADI__nullid;	/* Existing member definition */
  ADIobj	memclsid = ADI__nullid; /* Member class definition block */
  ADIobj	memname = ADI__nullid;	/* Name of new member */
  ADIobj	memtype = ADI__nullid;	/* Name of new member type */
  char		*name;
  int		nlen;
  ADIobj        newid;                  /* The new member definition */

  _chk_stat_ret(ADI__nullid);           /* Check status on entry */

/* Class member initialisation can only happen once the types required */
/* to represent the common string table are in place */
  if ( _valid_q(ADI_G_commonstrings ) ) {

    ADIstring	*sptr;

/* Locate the string on the parse stream in the common string table */
    memname = prsx_symname( pstr, status );

/* Get the next token. If it too is TOK__SYM then the first symbol is */
/* interpreted as a class name, and the second name is the member name */
    if ( ADInextToken( pstr, status ) == TOK__SYM ) {
      memtype = memname;

/* Locate the member class definition */
      memclsid = ADIkrnlFindClsI( memtype, status );

/* Validate class name */
    if ( _null_q(memclsid) ) {
      strx_tok( "MCLASS", memtype );
      adic_setecs( ADI__INVARG, "Unknown class name /^MCLASS/ in member specification", status );
      }

/* Parse the member name */
      memname = prsx_symname( pstr, status );

/* Advance to token after member name */
      ADInextToken( pstr, status );
      }

/*   Locate data for name string. Check not already present */
    sptr = _seg_data(memname);
    name = sptr->data;
    nlen = sptr->len;
    }

  else {
    name = _strm_data(pstr)->ctok.dat; nlen = _strm_data(pstr)->ctok.nc;
    }

/* Look for member name in existing list */
  emem = ADIdefClassLocMember( *members, name, nlen, status );

/* The member name does not already exist */
  if ( _null_q(emem) ) {

/* Allocate storage for new member */
    newid = adix_cls_alloc( &KT_DEFN_mdef, status );

/* Store name and length in characters */
    _mdef_name(newid) = strx_dupl( name, nlen );
    _mdef_nlen(newid) = nlen;
    _mdef_aname(newid) = memname;
    _mdef_defcls(newid) = memclsid;
    _mdef_cdata(newid) = ADI__nullid;
    _mdef_next(newid) = ADI__nullid;

/* Insert into list */
    **ipoint = newid;
    *ipoint = &_mdef_next(newid);
    }

/* Member name already exists. We allow the user to override the
/* initialisation type of the new member, but the new type but be the
/* same type or derived from the existing member type */
  else {
    if ( memclsid != _mdef_defcls(emem) &&
	_valid_q(memclsid) && _valid_q(_mdef_defcls(emem)) ) {
      KT_CTYPE_cdef	*c1 = _cdef_data(memclsid);
      KT_CTYPE_cdef	*c2 = _cdef_data(_mdef_defcls(emem));

      if ( ! adix_chkder( c1, c2, status ) ) {
	adic_setetc( "MEM", name, nlen );
	adic_setetc( "SMEM", c2->name, 99 );
	adic_setecs( ADI__INVARG,
"The initialisation class of member ^MEM must be derived from ^SMEM", status );
	}
      }

/* Overrides are ok? */
    if ( _ok(status) ) {
      _mdef_defcls(emem) = memclsid;
      newid = emem;
      }
    }

/* Advance to token after member name */
  if ( _null_q(ADI_G_commonstrings) )
    ADInextToken( pstr, status );

  return newid;
  }


void ADIdefClassNewMemberData( ADIobj pstr, ADIobj member,
			       ADIstatus status )
  {
  ADImemberDef	*memb = _mdef_data(member);

  _chk_stat;			/* Check status on entry */

/* If data already exists, the scrub it */
  if ( _valid_q(memb->cdata) )
    adic_erase( &memb->cdata, status );

/* Skip over the assignment token */
  ADInextToken( pstr, status );

/* Read a constant value from the stream */
  memb->cdata = prsx_cvalue( pstr, status );

/* Mark it as readonly */
  _han_readonly(memb->cdata) = ADI__true;
  }


void ADIparseClassMembers( ADIobj pstream,
			   ADIobj *members, ADIstatus status )
  {
  ADIlogical	defmem = ADI__false;
  ADIobj	*mnext = members;
  ADIlogical	more = ADI__true;

  _chk_init;			/* Check status on entry */

/* Locate member list insertion point */
  while ( _valid_q(*mnext) )
    mnext = &_mdef_next(*mnext);

/* While more class members to parse */
  while ( more && _ok(status) ) {

    ADIobj	newm;		/* The new member */

/* Add member to list */
    newm = ADIdefClassNewMember( pstream, members, &mnext, status );

/* This is the default member? */
    if ( ADIcurrentToken(pstream) == TOK__MUL ) {
      if ( ! defmem ) {
	defmem = ADI__true;
	_mdef_nlen(newm) = - _mdef_nlen(newm);
	ADInextToken( pstream, status );
	}
      else
	adic_setecs( ADI__INVARG, "Default member already defined", status );
      }

/* Definition supplies initial data? */
    if ( ADIcurrentToken(pstream) == TOK__ASSIGN )
      ADIdefClassNewMemberData( pstream, newm, status );

/* Comma delimits member names, otherwise at end of list. Shouldn't need */
/* TOK__END soak as parsing either from constant string or from EOL free */
/* stream?? */
    if ( ADIifMatchToken( pstream, TOK__COMMA, status ) ) {
      while ( ADIcurrentToken( pstream ) == TOK__END )
	ADInextToken( pstream, status );
      }
    else
      more = ADI__false;
    }
  }


void ADIparseClassSupers( ADIobj pstream, ADIobj *supers,
			  ADIobj *members, ADIstatus status )
  {
  ADItokenType	ctok;
  ADIobj	curp;			/* Loop over superclasses */
  ADIobj	*mnext = members;	/* Member list insertion point */
  ADIlogical	more = ADI__true;	/* More classes in list? */
  ADIobj	sname;			/* Superclass name string */
  ADIobj	*snext = supers;	/* Superclass list insertion point */
  ADIobj	stid;			/* Superclass definition object */
  ADIobj	newpar;			/* New superclass record */

  _chk_init;			/* Check status on entry */

/* Trap empty stream */
  more = (ADIcurrentToken(pstream) == TOK__SYM);

/* While more superclass names to parse */
  while ( more && _ok(status) ) {

/* Locate superclass name in common table */
    sname = prsx_symname( pstream, status );

/* Locate superclass by name */
    stid = ADIkrnlFindClsI( sname, status );

/* Error if not found */
    if ( _null_q(stid) ) {
      strx_tok( "SCLASS", sname );
      adic_setecs( ADI__INVARG, "Unknown class name /^SCLASS/ in superclass specification", status );
      }
    else {

/* Allocate storage for new superclass */
      newpar = adix_cls_alloc( &KT_DEFN_pdef, status );
      *snext = newpar;

/* Store attributes */
      _pdef_name(newpar) = sname;
      _pdef_clsid(newpar) = stid;
      _pdef_next(newpar) = ADI__nullid;

/* Get next token */
      ctok = ADInextToken( pstream, status );

/* Comma delimits superclass names, otherwise end */
      if ( ctok == TOK__COMMA ) {
	ctok = ADInextToken( pstream, status );
	while ( ctok == TOK__END )
	  ctok = ADInextToken( pstream, status );
	if ( ctok != TOK__SYM )
	  adic_setecs( ADI__INVARG,
		       "Syntax error in superclass specification", status );
	}
      else
	more = ADI__false;

/* Update insertion point */
      snext = &_pdef_next(newpar);
      }

/* End of loop over superclass names */
    }

/* Loop over superclasses, copying members to new class */
  curp = *supers;
  while ( _ok(status) && _valid_q(curp) ) {
    ADIobj  pmem;
    ADIclassDefPtr ptdef = _cdef_data(_pdef_clsid(curp));

/* Loop over all slots of the superclass */
    for( pmem = ptdef->members; _valid_q(pmem); pmem = _mdef_next(pmem) )
      (void) ADIdefClassCopyMember( pmem, members, &mnext, status );

/* Next superclass in list */
    curp = _pdef_next(curp);
    }
  }


ADIobj ADIdefClass_i( int narg, ADIobj args[], ADIstatus status )
  {
  ADIlogical		anyinit = ADI__false;
  ADIobj		cid;		/* The new definition structure */
  ADIobj		curm;		/* Loop over members */
  ADIobj		name = args[0];
  ADIobj		supers = args[1];
  ADIobj		members = args[2];
  size_t		size;
  ADIclassCode          t;              /* The new class code */
  ADIclassDefPtr        tdef;           /* New class definition */

  _chk_stat_ret(ADI__nullid);

/* Allocate new class definition record */
  cid = adix_new_cdef( _str_dat(name), _str_len(name), 0, &t, status );
  tdef = _cdef_data(cid);

/* Mark as a structured data type */
  tdef->prim = ADI__false;

/* Store superclass and member lists */
  tdef->members = members;
  tdef->superclasses = supers;

/* Count members, and determine whether any members have class initialisation */
  curm = members;
  tdef->nslot = 0;
  while ( _valid_q(curm) ) {
    ADImemberDef	*memb = _mdef_data(curm);

    tdef->nslot++;
    if ( memb->nlen < 0 ) {
      tdef->defmem = tdef->nslot;
      memb->nlen = - memb->nlen;
      }

    if ( _valid_q(memb->defcls) || _valid_q(memb->cdata) )
      anyinit = ADI__true;

    curm = memb->next;
    }

/* Store flag describing method initialisation */
  tdef->meminit = anyinit;

/* Number of bytes required per instance of the new class */
  size = tdef->nslot * sizeof(ADIobj);

/* Initialise basic block control for this class */
  adix_bb_init( &tdef->alloc, t, size, ADI__EXHANTAB, cid, status );

/* Construct direct super class list if table built. Also done manually */
  if ( _valid_q(ADI_G_commonstrings) )
    ADIdefClassMakeDlist( tdef, status );

/* Set function return value */
  return cid;
  }


void ADIdefClass_e( char *name, int nlen, char *parents, int plen,
		    char *members, int mlen, ADIobj *tid, ADIstatus status )
  {
  ADIobj		args[3] = {ADI__nullid,ADI__nullid,ADI__nullid};
					/* Arguments for internal routine */
					/* These are name, supers, members */
  ADIobj		cid;		/* New class identifier */
  ADIobj             pstream = ADI__nullid;        /* Parse stream */

  _chk_stat;

/* Create ADI string for name */
  adic_newv0c_n( name, nlen, args, status );

  _GET_STRING(parents,plen);            /* Import strings used in this rtn */
  _GET_STRING(members,mlen);

/* Parse parent specification if the string isn't empty. */
  if ( parents && (plen>0) ) {

/* Put parents specification into parse stream */
    pstream = ADIstrmExtendC( ADIstrmNew( status ), parents, plen, status );

    ADInextToken( pstream, status );

    ADIparseClassSupers( pstream, args+1, args+2, status );
    }

/* Parse member specification if the string isn't empty */
  if ( members && (mlen>0) ) {
    if ( _null_q(pstream) )
      pstream = ADIstrmNew( status );
    else
      ADIclearStream( pstream, status );

/* Put parents specification into parse stream */
    ADIstrmExtendC( pstream, members, mlen, status );

    ADInextToken( pstream, status );

    ADIparseClassMembers( pstream, args+2, status );
    }

/* Erase the parse stream if defined */
  if ( _valid_q(pstream) )
    adic_erase( &pstream, status );

/* Make new class definition */
  cid = ADIdefClass_i( 3, args, status );

/* Set return value if required */
  if ( tid )
    *tid = cid;
  }


void adix_delcls( KT_CTYPE_cdef **cvar, ADIstatus status )
  {
  ADIobj        cur;
  KT_CTYPE_cdef *tdef = *cvar;

  if ( tdef->link )                     /* Remove most recent first */
    adix_delcls( &tdef->link, status );

  if ( _valid_q(tdef->members) ) {      /* Members list */
    for( cur = tdef->members; _valid_q(cur); cur = _mdef_next(cur) )
      adix_erase( &_mdef_aname(cur),
		  1, status );
    }

  if ( _valid_q(tdef->superclasses) ) { /* Parents list */
    for( cur = tdef->superclasses; _valid_q(cur); cur = _pdef_next(cur) )
      adix_erase( &_pdef_name(cur),
		  1, status );
    }

  strx_free( tdef->name, status );      /* The class name */

  *cvar = NULL;
  }


void adix_def_destruc( ADIobj clsid, void (*rtn)( ADIobj, int, ADIstatus),
		       ADIstatus status )
  {
  _chk_stat;                            /* Check status on entry */

  _cdef_dest(clsid) = rtn;              /* Store destructor pointer */
  }


void adix_def_mcon( ADIobj clsid, void (*rtn)(ADIobj,ADImtaPtr,ADIstatus),
		    ADIstatus status )
  {
  _chk_stat;                            /* Check status on entry */

  _cdef_mcon(clsid) = rtn;              /* Store MTA constructor pointer */
  }

void adix_def_prnt( ADIobj clsid, void (*rtn)( ADIobj, ADIobj, ADIstatus),
		    ADIstatus status )
  {
  _chk_stat;                            /* Check status on entry */

  _cdef_prnt(clsid) = rtn;              /* Store printer pointer */
  }


void adix_prnt_b( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[5];
  int   nc;

  nc = sprintf( buf, "%d", (int) *((ADIbyte *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_ub( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[5];
  int   nc;

  nc = sprintf( buf, "%d", (int) *((ADIubyte *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_w( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[7];
  int   nc;

  nc = sprintf( buf, "%d", (int) *((ADIword *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_uw( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[7];
  int   nc;

  nc = sprintf( buf, "%d", (int) *((ADIuword *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_i( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[11];
  int   nc;

  nc = sprintf( buf, "%ld", *((ADIinteger *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_r( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[20];
  int   nc;

  nc = sprintf( buf, "%f", *((ADIreal *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_d( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[30];
  int   nc;

  nc = sprintf( buf, "%f", *((ADIdouble *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_l( ADIobj stream, ADIobj id, ADIstatus status )
  {
  static char *tstr = "True";
  static char *fstr = "False";

  UT_CTYPE_l    val = *((ADIlogical *) _DTDAT(id));

  ADIstrmPutStr( stream, val ? tstr : fstr, _CSM, status );
  }

void adix_prnt_c( ADIobj stream, ADIobj id, ADIstatus status )
  {
  int   len = _str_len(id);
  char *dat = _str_dat(id);

  ADIstrmPutCh( stream, '"', status );
  ADIstrmPutStr( stream, dat, len, status );
  ADIstrmPutCh( stream, '"', status );
  }

void adix_prnt_p( ADIobj stream, ADIobj id, ADIstatus status )
  {
  char	buf[20];
  int   nc;

  nc = sprintf( buf, "%p", *((UT_CTYPE_p *) _DTDAT(id)) );

  ADIstrmPutStr( stream, buf, nc, status );
  }

void adix_prnt_struc( ADIobj stream, ADIobj id, ADIstatus status )
  {
  ADIobj  sid = *_struc_data(id);
  ADIsegmentPtr sptr;

  _chk_stat;

  ADIstrmPutCh( stream, '{', status );

  while ( _valid_q(sid) ) {
    sptr = _seg_data(_CAAR(sid));
    ADIstrmPutStr( stream, sptr->data, sptr->len, status );
    ADIstrmPutCh( stream, '=', status );
    adix_print( stream, _CDAR(sid), ADI__true, status );
    sid = _CDR(sid);
    if ( _valid_q(sid) ) {
      ADIstrmPutCh( stream, ',', status );
      ADIstrmPutCh( stream, ' ', status );
      }
    }

  ADIstrmPutCh( stream, '}', status );
  }


void adix_exit()
  {
  static                                /* List of stuff to be deleted */
    ADIobj *dlist[] =
    {
    &ADI_G_replist,                     /* Representation list */
    &ADI_G_firstmthd,                   /* Methods list */
    &ADI_G_firstgnrc,                   /* Generics list */
    &ADI_G_firstmco,                    /* Method combinations list */
    &DnameAfter, &DnameAround,          /* Method forms */
    &DnameBefore, &DnamePrimary,
    &DnameSetLink, &DnameUnLink,        /* Data model operations */
    NULL
    };

  ADIobj        **dobj = dlist;
  ADIctxPtr     lastctx;
  ADIstatype    status = SAI__OK;

  while ( *dobj )                       /* While more to delete */
    {
    if ( _valid_q(**dobj) )             /* Referenced object is defined? */
      adix_erase( *dobj, 1, &status );

    dobj++;                             /* Next element of dlist */
    }

/* Unwind contexts. The last context is not dynamically allocated and
   so is never removed. */
  do {
    lastctx = ADI_G_curctx;
    adix_rlse();
    }
  while ( ADI_G_curctx != lastctx );

/* Scrub the type definitions list */
  adix_delcls( &ADI_G_firstcdef, &status );

/* Remove the common string table */
  adix_erase( &ADI_G_commonstrings, 1, &status );

/* Remove the dynamically allocated data */
  adix_mem_end( &status );

  ADI_G_init = ADI__false;              /* Mark as uninitialised */
  }


static
  int recurse_check = 0;

void adi_init( ADIstatus status )
  {
  static
    struct
      {
      char              *name;
      ADIclassCode      clas;
      size_t            size;
      ADIobj            *avar;
      void              (*prnt)(ADIobj,ADIobj,ADIstatus);
      }
    ttable[] =
      {
      {"BYTE",   UT_CODE_b, sizeof(UT_CTYPE_b), &UT_ALLOC_b ,adix_prnt_b},
      {"UBYTE",  UT_CODE_ub,sizeof(UT_CTYPE_ub),&UT_ALLOC_ub,adix_prnt_ub},
      {"WORD",   UT_CODE_w, sizeof(UT_CTYPE_w), &UT_ALLOC_w ,adix_prnt_w},
      {"UWORD",  UT_CODE_uw,sizeof(UT_CTYPE_uw),&UT_ALLOC_uw,adix_prnt_uw},
      {"INTEGER",UT_CODE_i, sizeof(UT_CTYPE_i), &UT_ALLOC_i ,adix_prnt_i},
      {"REAL",   UT_CODE_r, sizeof(UT_CTYPE_r), &UT_ALLOC_r ,adix_prnt_r},
      {"DOUBLE", UT_CODE_d, sizeof(UT_CTYPE_d), &UT_ALLOC_d ,adix_prnt_d},
      {"LOGICAL",UT_CODE_l, sizeof(UT_CTYPE_l), &UT_ALLOC_l ,adix_prnt_l},
      {"CHAR",   UT_CODE_c, sizeof(ADIsegment), &UT_ALLOC_c ,adix_prnt_c},
      {"POINTER",UT_CODE_p, sizeof(UT_CTYPE_p), &UT_ALLOC_p ,adix_prnt_p},
      {"STRUC",	 UT_CODE_struc, sizeof(UT_CTYPE_struc),&UT_ALLOC_struc, adix_prnt_struc}
      };

  static
    ADIobj	obj_defd = ADI__nullid;
  static
    ADIobj	struc_defd = ADI__nullid;
  static
    ADIsegment  c_defd = {NULL,0};
  static
    ADIpointer  p_defd = NULL;

  int           i;

  _ERR_IN("adi_init");                  /* Mark for error reporting */

  if ( !_ok(status) )                   /* Check status on entry */
    return;

  recurse_check++;

  if ( recurse_check > 1 ) {            /* Prevent recursive operation */
    adic_setecs( ADI__FATAL, "Illegal recursion, probable programming error or corruption", status );
    }
  else if ( ! ADI_G_init &&             /* If not already initialised */
	    ! ADI_G_init_failed )       /* and not already tried */
    {
    adix_mem_begin();                   /* Initialise allocation system */

    for( i=0; i<11; i++ ) {
      adix_def_pclass( ttable[i].name,
		       ttable[i].clas,
		       ttable[i].size,
		       ttable[i].avar,
		       status );

      adix_def_prnt( *ttable[i].avar,
	   ttable[i].prnt, status );
      }

/* Install primitive data initialisation for strings and structures */
    adix_def_pclass_data( &ADI_G_tdef_obj, (char *) &obj_defd, status );
    adix_def_pclass_data( _cdef_data(UT_ALLOC_c), (char *) &c_defd, status );
    adix_def_pclass_data( _cdef_data(UT_ALLOC_p), (char *) &p_defd, status );
    adix_def_pclass_data( _cdef_data(UT_ALLOC_struc), (char *) &struc_defd, status );

/* Mark as initialised - must happen before any objects created or ADI
   will enter a recursive loop (which is trapped, but is fatal) */
    if ( _ok(status) ) {
      ADI_G_init = ADI__true;

/*  Establish the ADI exit handler. Frees all dynamic memory */
#ifdef use_on_exit
      on_exit( adix_exit, NULL );
#else
      atexit( adix_exit );
#endif
      }
    else
      ADI_G_init_failed = ADI__true;

/* Load various sub-systems */
    strx_init( status );
    lstx_init( status );
    tblx_init( status );

/* Create common string table to hold property names, class member names
   and any other common strings */
    ADI_G_commonstrings = tblx_new( 203, 0, status );

/* Install member names of _List and _HashTable classes in the newly created
   common string table */
    if ( _ok(status) ) {
      ADIclassDefPtr    tdef = ADI_G_firstcdef;

      while ( tdef ) {                  /* Add class names to table */
	tdef->aname = adix_cmnC( tdef->name, status );
	tdef = tdef->link;
	}

      ADIdefClassConvertNames( _cdef_data(UT_ALLOC_list), status );
      ADIdefClassConvertNames( _cdef_data(UT_ALLOC_tbl), status );
      ADIdefClassMakeDlist( _cdef_data(UT_ALLOC_list), status );
      ADIdefClassMakeDlist( _cdef_data(UT_ALLOC_tbl), status );
      }

/* Create constant objects */
    adic_newv0l( ADI__false, &ADIcvFalse, status );
    _han_readonly(ADIcvFalse) = ADI__true;
    adic_newv0l( ADI__true, &ADIcvTrue, status );
    _han_readonly(ADIcvTrue) = ADI__true;
    ADIcvNulCons = lstx_cell( ADI__nullid, ADI__nullid, status );
    _han_readonly(ADIcvNulCons) = ADI__true;

/* Install sub-systems which depend on common string table etc */
    prsx_init( status );

/* Create the constant stream object pointing to standard output */
    ADIcvStdOut = ADIstrmExtendFile( ADIstrmNew( status ), stdout, status );
/*    _han_readonly(ADIcvStdOut) = ADI__true; */

/* Install "Standard" method combination */
    adic_defmcf( "Standard", adix_stdmcf, &ADI_G_stdmcf, status );

/* Base class for object linking */
    adic_defcls( "ADIbase", "", "ADIlink", &DsysADIbase, status );

/* Install file system data extensions */
    ADIfsysInit( status );
    }

/* Restore recursion checker */
  recurse_check--;

/* Reset error naming */
  _ERR_OUT;
  }


#define _do_case(_t) \
    case _TM_code(_t): \
      for( optr = out, ival=nval; \
	   ival; ival--, iptr+=sizeof(_TM_ctype(_t)) ) \
	*optr++ = *((_TM_ctype(_t) *) iptr); \
      break;

#define _do_case_chk2(_t,_ct) \
    case _TM_code(_t): \
      for( optr = out, ival=nval; \
	   ival; ival--, iptr+=sizeof(_TM_ctype(_t)) ) {\
	_TM_ctype(_t) val = *((_TM_ctype(_t) *) iptr);\
	if ( val<((_TM_ctype(_t)) _TM_min(_ct)) || val>((_TM_ctype(_t)) _TM_max(_ct)) )\
	  {*optr++ = _TM_bad(_ct); (*nerr)++;}\
	else \
	  *optr++ = (_TM_ctype(_ct)) val;} \
      break;

#define _do_case_chk1(_t,_ct) \
    case _TM_code(_t): \
      for( optr = out, ival=nval; ival; ival--, iptr+=sizeof(_TM_ctype(_t)) ) {\
	_TM_ctype(_t) val = *((_TM_ctype(_t) *) iptr);\
	if ( val>((_TM_ctype(_t)) _TM_max(_ct)) )\
	  {*optr++ = _TM_bad(_ct); (*nerr)++;}\
	else \
	  *optr++ = (_TM_ctype(_t)) val;} \
      break;


/*
 * Caste to _BYTE
 */
void adix_caste_b( ADIclassCode type, int nval, char *in,
		   ADIbyte out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIbyte       *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case_chk1(ub,b)
    _do_case_chk2(w,b)
    _do_case_chk1(uw,b)
    _do_case_chk2(i,b)
    _do_case_chk2(r,b)
    _do_case_chk2(d,b)
    }
  }

/*
 * Caste to _UBYTE
 */
void adix_caste_ub( ADIclassCode type, int nval, char *in,
		    ADIubyte out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIubyte      *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case_chk2(b,ub)
    _do_case_chk1(w,ub)
    _do_case_chk1(uw,ub)
    _do_case_chk2(i,ub)
    _do_case_chk2(r,ub)
    _do_case_chk2(d,ub)
    }
  }

/*
 * Caste to _WORD
 */
void adix_caste_w( ADIclassCode type, int nval, char *in,
		   ADIword out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIword       *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case(b)
    _do_case(ub)
    _do_case_chk1(uw,w)
    _do_case_chk2(i,w)
    _do_case_chk2(r,w)
    _do_case_chk2(d,w)
    }
  }


/*
 * Caste to _UWORD
 */
void adix_caste_uw( ADIclassCode type, int nval, char *in,
		    ADIuword out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIuword      *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case_chk2(b,uw)
    _do_case(ub)
    _do_case_chk1(w,uw)
    _do_case_chk2(i,uw)
    _do_case_chk2(r,uw)
    _do_case_chk2(d,uw)
    }
  }


/*
 * Caste to _INTEGER
 */
void adix_caste_i( ADIclassCode type, int nval, char *in,
		   ADIinteger out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIinteger    *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case(b)
    _do_case(ub)
    _do_case(w)
    _do_case(uw)
    _do_case_chk2(r,i)
    _do_case_chk2(d,i)
    }
  }


/*
 * Caste to REAL
 */
void adix_caste_r( ADIclassCode type, int nval, char *in,
		   ADIreal out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIreal       *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case(b)
    _do_case(ub)
    _do_case(w)
    _do_case(uw)
    _do_case(i)
    _do_case_chk2(d,r)
    }
  }


/*
 * Caste to _DOUBLE
 */
void adix_caste_d( ADIclassCode type, int nval, char *in,
		   ADIdouble out[], int *nerr )
  {
  char          *iptr = in;    		/* Cursor over input data */
  int           ival;
  ADIdouble     *optr;                  /* Cursor over output data */

  switch( type ) {
    _do_case(ub)
    _do_case(b)
    _do_case(uw)
    _do_case(w)
    _do_case(i)
    _do_case(r)
    }
  }
#undef _do_case


/*
 * Caste to _CHAR
 */
void adix_caste_c( ADIlogical is_adi, ADIclassCode type, int nval, char *in,
		   char *out, int clen, int onulterm, int *nerr )
  {
  char          buf[30];                /* Big enough to hold character
					   converted basic types */
  int           ic;                     /* Loop over converted data */
  char          *iptr = in;    		/* Cursor over input items */
  int           ival = nval;            /* Loop over input values */
  ADIlogical    nterm = (clen==_CSM);   /* Null terminated input? */
  int           nc;                     /* Number of bytes written to buffer */
  int           oblen = clen;           /* Length of output buffer */
  char          *obuf;                  /* Output buffer */
  char          **odptr;                /* Cursor over null term'd strings */
  char          *optr;                  /* Cursor over block strings */
  ADIsegmentPtr osptr = NULL;           /* Cursor over ADI strings */

  if ( nterm )                          /* Null terminated strings? */
    odptr = (char **) out;
  else if ( is_adi )                    /* Block character strings? */
    osptr = (ADIsegmentPtr) out;
  else                                  /* ...otherwise ADI strings */
    optr = (char *) out;

#define _cop_pad(_s,_n) \
  if ( nterm ) {obuf=*odptr;} \
  else if ( is_adi ) {obuf=osptr->data;oblen=osptr->len;} \
  else obuf = optr; \
  if ( (oblen) && (_n > oblen) ) \
    {(*nerr)++;memset(obuf,' ',_n);}\
  else \
    {for(ic=0;ic<_n;ic++) obuf[ic] = _s[ic]; \
     if ( nterm ) obuf[_n]=0; \
     else if ( _n < oblen ) \
       {if (onulterm) obuf[_n]=0;else memset(obuf+_n,' ',oblen- _n);}}

#define _do_next if ( nterm ) odptr++; \
		 else { if ( is_adi ) osptr++; else optr+=clen;}

  switch( type ) {
    case _TM_code(ub):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(ub)) ) {
	nc = sprintf( buf, "%d", (int) *((_TM_ctype(ub) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(b):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(b)) ) {
	nc = sprintf( buf, "%d", (int) *((_TM_ctype(b) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(uw):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(uw)) ) {
	nc = sprintf( buf, "%d", (int) *((_TM_ctype(uw) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(w):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(w)) ) {
	nc = sprintf( buf, "%d", (int) *((_TM_ctype(w) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(i):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(i)) ) {
	nc = sprintf( buf, "%ld", *((_TM_ctype(i) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(r):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(r)) ) {
	nc = sprintf( buf, "%g", *((_TM_ctype(r) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(d):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(d)) ) {
	nc = sprintf( buf, "%g", *((_TM_ctype(d) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;

    case _TM_code(p):
      for( ; ival; ival--, iptr+=sizeof(_TM_ctype(p)) ) {
	nc = sprintf( buf, "%x", *((_TM_ctype(p) *) iptr) );
	_cop_pad(buf,nc);
	_do_next;
	}
      break;
    }

#undef _cop_pad
#undef _do_next
  }


/*
 * Caste from _CHAR
 */
void adix_caste_c2( ADIlogical is_adi, int clen, ADIclassCode type,
		    char *in, int nval, char *out, int *nerr )
  {
  char          buf[30];                /* Big enough to hold character
					   converted basic types */
  ADIlogical    ftype = ADI__false;     /* Floating point output? */
  int           ic;                     /* Loop over converted data */
  char          **idptr;                /* Cursor over null term'd strings */
  char          *iptr;                  /* Cursor over block strings */
  ADIsegmentPtr isptr = NULL;           /* Cursor over ADI strings */
  int           ival = nval;            /* Loop over input values */
  int           nscan;                  /* Number of items read by sscanf */
  ADIlogical    nterm = (clen==_CSM);   /* Null terinated input? */
  char          *optr = out;    	/* Output data pointer */

  if ( nterm )                          /* Null terminated strings? */
    idptr = (char **) in;
  else if ( is_adi )                    /* Block character strings? */
    isptr = (ADIsegmentPtr) in;
  else                                  /* ...otherwise ADI strings */
    iptr = (char *) in;

  if ( (type == UT_CODE_r) ||           /* Floating point output? */
       (type == UT_CODE_d) )
    ftype = ADI__true;

#define _do_scan \
	ADIdouble      out;\
	if ( nterm ) strncpy(buf,*idptr,30);\
	else if ( is_adi ) strncpy( buf, isptr->data, isptr->len ); \
	else { \
	memcpy( buf, iptr, clen );\
	buf[clen] = 0;}\
	if ( ftype ) \
	  for( ic=0; ic<clen; ic++ )\
	    if ( (buf[ic] == 'd') || (buf[ic]=='D') )\
	      buf[ic] = 'e';\
	nscan = sscanf( buf, "%lg", &out );\

#define _chk_ok_2(_t,_ct) \
  if ( nscan == 1 ) { \
  if ( out<((_TM_ctype(_t)) _TM_min(_ct)) || out>((_TM_ctype(_t)) _TM_max(_ct)) )\
    {*((_TM_ctype(_ct) *) optr) = _TM_bad(_ct); (*nerr)++;}\
  else \
    *((_TM_ctype(_ct) *) optr) = (_TM_ctype(_ct)) out;} \
  else \
    {*((_TM_ctype(_ct) *) optr) = _TM_bad(_ct); (*nerr)++;}

#define _chk_ok_1(_t) \
  if ( nscan == 1 ) \
    *((_TM_ctype(_t) *) optr) = (_TM_ctype(_t)) out; \
  else \
    {*((_TM_ctype(_t) *) optr) = _TM_bad(_t); (*nerr)++;}

#define _do_next if ( nterm ) idptr++; \
		 else { if ( is_adi ) isptr++; else iptr+=clen;}

  switch( type ) {
    case _TM_code(ub):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(ub)) ) {
	_do_scan;
	_chk_ok_2(d,ub);
	_do_next;
	}
      break;

    case _TM_code(b):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(b)) ) {
	_do_scan;
	_chk_ok_2(d,b);
	_do_next;
	}
      break;

    case _TM_code(uw):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(uw)) ) {
	_do_scan;
	_chk_ok_2(d,uw);
	_do_next;
	}
      break;

    case _TM_code(w):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(w)) ) {
	_do_scan;
	_chk_ok_2(d,w);
	_do_next;
	}
      break;

    case _TM_code(i):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(i)) ) {
	_do_scan;
	_chk_ok_2(d,i);
	_do_next;
	}
      break;

    case _TM_code(r):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(r)) ) {
	_do_scan;
	_chk_ok_2(d,r);
	_do_next;
	}
      break;

    case _TM_code(d):
      for( ; ival; ival--, optr+=sizeof(_TM_ctype(d)) ) {
	_do_scan;
	_chk_ok_1(d);
	_do_next;
	}
      break;
    }

#undef _do_scan
#undef _chk_ok_1
#undef _chk_ok_2
#undef _do_next
  }

/*
 *  Locate the allocator block for the named class
 */
ADIclassDef *ADIkrnlFindClsInt( char *cls, int clen, ADIstatus status )
  {
  ADIlogical            found = ADI__false;
  ADIclassDefPtr        tdef = ADI_G_firstcdef;       /* Cursor */

/* Entry status is ok? */
  if ( _ok(status) ) {

/* Loop over classes comparing class name string with that supplied */
    while ( tdef && ! found ) {
      if ( ! strncmp( cls, tdef->name, clen ) )
	found = ADI__true;
      else
	tdef = tdef->link;
      }
    }

/* Set function return value */
  return found ? tdef : NULL;
  }

ADIclassDef *ADIkrnlFindClsC( char *cls, int clen, ADIstatus status )
  {
  ADIclassDef	*tdef = NULL;           /* Default return value */

  if ( (*cls == '*') && (clen==1) )     /* The universal type */
    tdef = &KT_DEFN_obj;
  else
    tdef = ADIkrnlFindClsInt( cls, clen, status );

  return tdef;
  }

ADIobj ADIkrnlFindClsI( ADIobj name, ADIstatus status )
  {
  ADIclassDef	*tdef;

  tdef = ADIkrnlFindClsC( _str_dat(name), _str_len(name), status );

  return tdef ? tdef->selfid : ADI__nullid;
  }


/*
 * Allocate object(s) of user named class
 */
void adix_newn( ADIobj pid, char *name, int nlen, char *cls, int clen,
		int ndim, int dims[], ADIobj *id, ADIstatus status )
  {
  ADIclassDef		*tdef;		/* Class definition */

  _chk_stat;                            /* Standard checks */

  _GET_NAME(cls,clen);                	/* Import string */

/* Locate the class allocator */
  tdef = ADIkrnlFindClsC( cls, clen, status );

/* Invoke creator function with null data values */
  if ( tdef ) {
    adix_new_n( ADI__true, pid, name, nlen, ndim, dims, NULL,
		tdef, 0, id, status );
    }
  }


ADIobj adix_cmn_i( char *str, int len, ADIlogical dstatic, ADIstatus status )
  {
  ADIobj        dpair;                  /* name.value pair in table */
  ADIobj        name;

  _GET_STRING(str,len);                 /* Handle C string marker */

/* Find or insert in string table */
  dpair = tblx_sadd( &ADI_G_commonstrings, str, len, ADI__nullid, status );

/* Clone the identifier from the name of the dotted pair */
  name = adix_clone( _CAR(dpair), status );

/* Mark string as read-only */
  _han_readonly(name) = ADI__true;

/* Set return value */
  return _ok(status) ? name : ADI__nullid;
  }

/*
 * Common string, length specified, not static data
 */
ADIobj adix_cmn( char *str, int len, ADIstatus status )
  {
  return adix_cmn_i( str, len, ADI__false, status );
  }

/*
 * Common string, nul terminated, not static data
 */
ADIobj adix_cmnC( char *str, ADIstatus status )
  {
  return adix_cmn_i( str, strlen(str), ADI__false, status );
  }


/*  adix_pl_scan - Scan property for a property name
 *
 *  Description :
 *
 *    Returns address a pointer to a list element. If 'found' is true,
 *    the _CAR of the element is the property-value dotted pair, otherwise
 *    the address is the insertion point for such a property.
 */
void adix_pl_scan( ADIobj    *head,      /* Head of a NODE list */
		  ADIobj    str,        /* String to find */
		  ADIobj    **sptr,     /* & of pointer to prop dotted pair */
		  ADIlogical  *found,     /* Was a property cell found? */
		  ADIstatus status )
  {
  ADIobj        cstr;                   /* Current property name string */
  int           test=0;                 /* String comparison */

  if ( !_ok(status) )                   /* Check status */
    return;

  *found = ADI__false;                  /* Default return values */
  *sptr = head;

  while ( ((**sptr)!=ADI__nullid) &&    /* Loop while end of list not reached */
	  !(*found) &&                  /*   and property not found */
	  (test>=0) )                   /*   and not past property in alphabet */
    {
    cstr = _CAAR(**sptr);               /* Locate next property name */

    if ( cstr == str )                  /* Useful fast check for equality */
      *found = ADI__true;
    else
      {
      test = strx_cmp( str, cstr );     /* Compare property names. */

      if ( test == 0 )                  /* Terminate if match found */
	*found = ADI__true;
      else if ( test > 0 )              /* Next list element if property name */
	*sptr = &_CDR(**sptr);          /* precedes current one alph'ically */
      }
    }
  }


/*
 *  Locate insertion point for named item in a property list
 */
void adix_pl_find( ADIobj *plist, char *property, int plen,
		   ADIlogical create, ADIobj **value, ADIobj *parid,
		   ADIstatus status )
  {
  ADIobj        hnode;                  /* New element for table hash list */
  ADIobj        *lentry;                /* List insertion point */
  ADIlogical    there;                  /* String in list? */
  ADIobj        tstr;                   /* Property string descriptor */

  _chk_stat;                            /* Check status */

  *value = NULL;                        /* Default return value */

  tstr = adix_cmn( property, plen,      /* Find or insert in common table */
		   status );

  if ( parid )                          /* Set parent to point to name */
    *parid = tstr;

  adix_pl_scan( plist, tstr,            /* Look along list for string */
      &lentry, &there, status );

  if ( there ) {
    *value = &_CDAR(*lentry);
    }
  else if ( create )
    {
    hnode = lstx_cell( tstr,            /* The string.value dotted pair */
		 ADI__nullid, status );

    *lentry = lstx_cell( hnode,         /* The list cell */
	     *lentry, status );

    *value = &_CDR(hnode);
    }
  }


void adix_delprp( ADIobj id, char *pname, int plen, ADIstatus status )
  {
  ADIobj        *lentry;                /* List insertion point */
  ADIobj	*plist;			/* Property list address */
  ADIlogical    there;                  /* String in list? */
  ADIobj        old_dp;
  ADIobj        tstr;                   /* Temp string descriptor */

  _chk_han(id); _chk_stat;		/* Has to be a handled object */

  tstr = adix_cmn( pname, plen,		/* Get name in table */
		   status );

  plist = &_han_pl(id);			/* Locate the property list */

  adix_pl_scan( plist, tstr,            /* Look along list for string */
      &lentry, &there, status );

  adix_erase( &tstr, 1, status );       /* Free temporary string */

  if ( there )                          /* Present in list? */
    {
    old_dp = *lentry;                   /* The dotted pair to be deleted */

    *lentry = _CDR(*lentry);            /* By-pass old list element */

    adix_erase( &_CAR(old_dp), 1, status );/* Delete old property and value */
    _CDR(old_dp) = ADI__nullid;
    adix_erase( &old_dp, 1, status );
    }
  }

void adix_locprp( ADIobj id, char *pname, int plen, ADIobj *pid,
		  ADIstatus status )
  {
  ADIobj	*vaddr;			/* Address of property value */

  _chk_han(id); _chk_stat;		/* Has to be a handled object */

  adix_pl_find( &_han_pl(id), pname, plen, ADI__false,
		&vaddr, NULL, status );

  if ( vaddr )
    *pid = *vaddr;
  else {
    adic_setetc( "PROP", pname, plen );
    adic_setecs( ADI__NOPROP, "Property with name /^PROP/ not found", status );
    }
  }

void adix_nprp( ADIobj id, int *nprp, ADIstatus status )
  {
  ADIobj	plist;			/* The object property list */
  int		n = 0;			/* Number of properties */

  _chk_han(id); _chk_stat;		/* Has to be a handled object */

  plist = _han_pl(id);			/* Locate the property list */

  if ( _valid_q(plist) )		/* Check for null list */
    n = lstx_len( _han_pl(id), status );

  if ( _ok(status) )
    *nprp = n;
  }

void adix_indprp( ADIobj id, int index, ADIobj *pid, ADIstatus status )
  {
  ADIobj	plist;			/* The object property list */
  ADIobj	*pslot;			/* The property slot */

  _chk_han(id); _chk_stat;		/* Has to be a handled object */

  if ( index < 1 )
    adic_setecs( ADI__INVARG, "Property index must be greater than zero", status );

  else {
    plist = _han_pl(id);		/* Locate the property list */

    if ( _valid_q(plist) ) {		/* Not an empty list */
      pslot = lstx_nth( plist, index, status );

      if ( pslot )			/* Valid list item? */
	*pid = adix_clone( _CDR(*pslot), status );
      else
	adic_setecs( ADI__NOPROP, "Property index is too large", status );
      }
    else
      adic_setec( ADI__NOPROP, status );
    }
  }


/*
 * Structures
 */
void adix_delcmp( ADIobj id, char *cname, int clen, ADIstatus status )
  {
  ADIobj        *lentry;                /* List insertion point */
  ADIobj	*clist;			/* Component list address */
  ADIlogical    there;                  /* String in list? */
  ADIobj        old_dp;
  ADIobj        tstr;                   /* Temp string descriptor */

  if ( ! _struc_q(id) ) 		/* Check this is a structure */
    adic_setecs( ADI__ILLOP, "Object is not of type STRUC", status );

  _chk_stat;				/* Has to be a handled object */

  tstr = adix_cmn( cname, clen,		/* Get name in table */
		   status );

  clist = _struc_data(id);		/* Locate the component list */

  adix_pl_scan( clist, tstr,            /* Look along list for string */
      &lentry, &there, status );

  adix_erase( &tstr, 1, status );       /* Free temporary string */

  if ( there ) {                        /* Present in list? */
    old_dp = *lentry;                   /* The dotted pair to be deleted */

    *lentry = _CDR(*lentry);            /* By-pass old list element */

    adix_erase( &_CAR(old_dp), 1, status );/* Delete old component and value */
    _CDR(old_dp) = ADI__nullid;
    adix_erase( &old_dp, 1, status );
    }
  }

void adix_loccmp( ADIobj id, char *cname, int clen, ADIobj *cid,
		  ADIstatus status )
  {
  ADIobj	*vaddr;			/* Address of property value */

  if ( ! _struc_q(id) ) 		/* Check this is a structure */
    adic_setecs( ADI__ILLOP, "Object is not of type STRUC", status );

  _chk_stat;				/* Has to be a handled object */

/* Find object address */
  adix_pl_find( _struc_data(id), cname, clen, ADI__false,
		&vaddr, NULL, status );

  if ( vaddr )
    *cid = *vaddr;
  else {
    adic_setetc( "COMP", cname, clen );
    adic_setecs( ADI__NOCOMP, "Component with name /^COMP/ not found", status );
    }
  }

void adix_ncmp( ADIobj id, int *ncmp, ADIstatus status )
  {
  ADIobj	clist;			/* The structure component list */
  int		n = 0;			/* Number of properties */

  if ( ! _struc_q(id) ) 		/* Check this is a structure */
    adic_setecs( ADI__ILLOP, "Object is not of type STRUC", status );

  _chk_stat;				/* Has to be a handled object */

  clist = *_struc_data(id);		/* Locate the component list */

  if ( _valid_q(clist) )		/* Check for null list */
    n = lstx_len( clist, status );

  if ( _ok(status) )
    *ncmp = n;
  }

void adix_indcmp( ADIobj id, int index, ADIobj *cid, ADIstatus status )
  {
  ADIobj	clist;			/* The structure component list */
  ADIobj	*cslot;			/* The component slot */

  if ( ! _struc_q(id) ) 		/* Check this is a structure */
    adic_setecs( ADI__ILLOP, "Object is not of type STRUC", status );

  _chk_stat;				/* Has to be a handled object */

  if ( index < 1 )
    adic_setecs( ADI__INVARG, "Component index must be greater than zero", status );

  else {
    clist = *_struc_data(id);		/* Locate the component list */

    if ( _valid_q(clist) ) {		/* Not an empty list */
      cslot = lstx_nth( clist,
			index, status );

      if ( cslot )			/* Valid list item? */
	*cid = adix_clone( _CDR(*cslot),
			   status );
      else
	adic_setecs( ADI__NOCOMP, "Component index is too large", status );
      }
    else
      adic_setec( ADI__NOCOMP, status );
    }
  }



/* Context manipulation
 *
 */
void adix_mark( void )
  {
  ADIctxPtr     newc;

  newc = (ADIctxPtr) malloc(sizeof(ADIctx));

  newc->onexitlist = ADI__nullid;
  newc->last = ADI_G_curctx;

  ADI_G_curctx = newc;
  }

void adix_rlse( void )
  {
  ADIctxPtr     oldc = ADI_G_curctx;

  if ( ADI_G_curctx != &ADI_G_basectx )
    {
    if ( oldc->onexitlist !=            /* Exit handlers to execute? */
	 ADI__nullid )
      {
      }

    ADI_G_curctx = oldc->last;          /* Recover previous context */

    free( oldc );                       /* Free context memory */
    }
  }


/* Reference count manipulation
 *
 *  Internal:
 *
 *    adix_refcnt       - retrieve count
 *    adix_refadj       - add offset to reference count
 */
int adix_refcnt( ADIobj id, ADIstatus status )
  {
  int           cnt = 0;

  if ( _han_q(id) )
    cnt = _han_ref(id);
  else
    adic_setec( ADI__ILLKOP, status );

  return cnt;
  }


void adix_refadj( ADIobj id, int offset, ADIstatus status )
  {
  if ( _han_q(id) )
    _han_ref(id) += offset;
  else
    adic_setec( ADI__ILLKOP, status );
  }


ADIobj *adix_defmem( ADIobj *id, ADIstatus status )
  {
  ADIobj                *dmem = NULL;
  ADIclassDefPtr        tdef;

  if ( _ok(status) )
    {
    tdef = _DTDEF(*id);
    if ( tdef->prim )
      dmem = id;
    else if ( tdef->defmem == DEF_MEMBER_FLAG_VALUE )
      {
      adic_setetc( "CLS", tdef->name, 99 );
      adic_setecs( ADI__NOMEMB, "No default member defined for class ^CLS", status );
      }
    else
      dmem = _class_data(*id) + tdef->defmem;
    }

  return dmem;
  }


/*
 * Create a memory transfer object
 */
ADIobj adix_new_mta( ADIclassCode type, ADIstatus status )
  {
  ADImtaPtr  mdata;                  /* Pointer to transfer data */
  ADIobj newm = ADI__nullid;

  newm = adix_cls_alloc( &KT_DEFN_mta,  /* Allocate new MTA */
			status );

  if ( _ok(status) )                    /* Allocated ok? */
    {
    mdata = _mta_data(newm);            /* Locate data block */

    mdata->type = type;                 /* Store object code */
    }

  return newm;                          /* Return new MTA */
  }


void adix_mtacop_c( int in_is_adi, char *in, int ilen,
		    int nval,
		    int out_is_adi, char *out, int olen,
		    int onulterm, ADIstatus status )
  {
  char          *ibuf;
  int           ic;                     /* Loop over converted data */
  char          **idptr;                /* Cursor over null term'd strings */
  ADIlogical    interm = (ilen==_CSM);  /* Null terminated input? */
  char          *iptr;                  /* Cursor over input items */
  ADIsegmentPtr isptr = NULL;           /* Cursor over ADI strings */
  int           ival = nval;            /* Loop over input values */
  int           lilen = ilen;           /* Local input length */
  int           lolen = olen;           /* Local output length */
  char          *obuf;
  char          **odptr;                /* Cursor over null term'd strings */
  ADIlogical    onterm = (olen==_CSM);  /* Null terminated input? */
  char          *optr;                  /* Cursor over block strings */
  ADIsegmentPtr osptr = NULL;           /* Cursor over ADI strings */

  _chk_stat;                            /* Check status on entry */

  if ( interm )                         /* Null terminated input */
    idptr = (char **) in;
  else if ( in_is_adi )                 /* Block character input */
    isptr = (ADIsegmentPtr) in;
  else                                  /* ...otherwise ADI input */
    iptr = (char *) in;

  if ( onterm )                         /* Null terminated output */
    odptr = (char **) out;
  else if ( out_is_adi )                /* Block character output */
    osptr = (ADIsegmentPtr) out;
  else                                  /* ...otherwise ADI output */
    optr = (char *) out;

  for( ; ival; ival-- )                 /* Loop over input strings */
    {
    if ( interm ) {                     /* Find length of this input */
      ibuf = *idptr;
      lilen = strlen( ibuf );
      }
    else if ( in_is_adi ) {
      ibuf = isptr->data;
      lilen = isptr->len;
      }
    else
      ibuf = iptr;

    if ( out_is_adi ) {                 /* Allowed length for output */
      obuf = osptr->data;
      lolen = osptr->len;
      }
    else if ( onterm ) {
      obuf = *odptr;
      lolen = 999;
      }
    else
      obuf = optr;

    for( ic=0; ic<_MIN(lilen,lolen); ic++ )
      obuf[ic] = ibuf[ic];

    if ( lilen < lolen )                /* Null terminate or pad */
      {
      if ( onulterm )
	obuf[lilen] = 0;
      else
	memset( obuf + lilen, ' ', lolen - lilen );
      }

    if ( interm )                       /* Advance input pointer */
      idptr++;
    else if ( in_is_adi )
      isptr++;
    else
      iptr += ilen;

    if ( interm )                       /* Advance output pointer */
      odptr++;
    else if ( out_is_adi )
      osptr++;
    else
      optr += olen;
    }
  }

/*
 * This routine is only needed if foreign language interfaces are built
 */
#ifdef ADI_F77
void adix_mtacop_l( ADIlogical in_c, ADIlogical *in, int n,
		    ADIlogical out_c, ADIlogical *out, ADIstatus status )
  {
  int		i;
  ADIlogical	*iptr = in;
  ADIlogical	*optr = out;

  if ( in_c && ! out_c ) {
    for( i=0; i<n; i++ )
      *optr++ = (*iptr++) ? F77_TRUE : F77_FALSE;
    }
  else if ( out_c && ! in_c ) {
    for( i=0; i<n; i++ )
      *optr++ = ((*iptr++) == F77_FALSE) ? ADI__false : ADI__true;
    }
  else
    _CH_MOVE( in, out, n*sizeof(ADIlogical) );
  }
#endif

void adix_mtacop( ADImtaPtr ind, ADImtaPtr outd, ADIstatus status )
  {
  ADIlogical    contig = ind->contig;   /* Contiguous tranfer? */
  int           idim;                   /* Loop over dimensions */
  char		*idptr;			/* Ptr through i/p declared space */
  int		ioffset = 0;		/* Offset from i/p frame to origin */
  int		isec;			/* Loop over sections to be copied */
  int		isecskip = 1;		/* I/p values to skip per section */
  int		ncdim;			/* Number of contiguous dimensions */
  int		nerr = 0;		/* Number of conversion errors */
  int		nsec = 1;		/* Number of contiguous areas */
  char		*odptr;			/* Ptr through o/p declared space */
  int           onval = 1;              /* Values to move per iteration */
  int		ooffset = 0;		/* Offset from o/p frame to origin */
  int		osecskip = 1;		/* O/p values to skip per section */

/* Output is not truncated by default */
  outd->trunc = ADI__false;

/* Find number of contiguous dimensions if the input is non-contiguous */
  if ( ! contig ) {
    ncdim = 0;
    for( idim=0; idim<ind->ndim && ! ncdim; idim++ )
      if ( ind->udims[idim] != ind->ddims[idim] )
	ncdim = idim;
    }

/* Copying array values? If so, check that output is not truncated */
  if ( ind->ndim ) {
    for( idim=0; idim<ind->ndim; idim++ ) {
      if ( outd->ddims[idim] < ind->udims[idim] ) {
	outd->udims[idim] = outd->ddims[idim];
	outd->trunc = ADI__true;
	}
      else
	outd->udims[idim] = ind->udims[idim];

/* If the output declared data dimension is larger than the input data */
/* dimension to be copied then the output data space is not contiguous, */
/* unless this is the first dimension */
      if ( contig && idim ) {
	if ( ind->udims[idim] != outd->ddims[idim] ) {
	  if ( contig )
	    ncdim = idim;
	  contig = ADI__false;
	  }
	}

/* While contiguous calculate number of elements to be copied per section. */
/* Once non-contiguous, accumlate the number of sections */
      if ( contig ) {
	isecskip *= ind->ddims[idim];
	osecskip *= outd->ddims[idim];
	onval *= outd->udims[idim];
	}
      else
	nsec *= outd->udims[idim];
      }

/* Calculate offsets from start of each declared sub-space to the origin */
/* of the section to be copied */
    ioffset = ADIaryOffset( ind->ndim, ind->ddims, ind->uorig );
    ooffset = ADIaryOffset( outd->ndim, outd->ddims, outd->uorig );
    }

/* Initialise pointers to input and output data. These step through the */
/* declared data spaces, with offsets applied for the origins of the */
/* sections with respect to the declared dimensions */
  idptr = (char *) ind->data + ioffset * ind->size;
  odptr = (char *) outd->data + ooffset * outd->size;

/* Loop over contiguous sections to be copied.  */
  for( isec = 0; isec<nsec; isec++ ) {

/* Types are the same? */
    if ( ind->type == outd->type ) {

/* Special routine for character copying */
      if ( outd->type == UT_CODE_c )
	adix_mtacop_c( _valid_q(ind->id), idptr, ind->size, onval,
		       _valid_q(outd->id), odptr, outd->size,
		       outd->nulterm, status );

/* And another special one for logical copying. This is only required */
/* if foreign languages are built in */
#ifdef ADI_F77
      else if ( outd->type == UT_CODE_l )
	adix_mtacop_l( ind->nulterm, (ADIlogical *) idptr, onval,
		       outd->nulterm, (ADIlogical *) odptr, status );
#endif

/* Copy data directly */
      else
	_CH_MOVE( odptr, idptr, onval*outd->size );
    }

/* Types are different, conversion required */
    else {

/*   Converting to character type? */
      if ( outd->type == UT_CODE_c )
	adix_caste_c( _valid_q(outd->id), ind->type, onval, idptr,
		      odptr, outd->size, outd->nulterm, &nerr );

/*   Converting from character type? */
      else if ( ind->type == UT_CODE_c )
	adix_caste_c2( _valid_q(ind->id), ind->size, outd->type,
		       idptr, onval, odptr, &nerr );

/*   Otherwise switch on type */
      else {
	switch ( outd->type ) {
	  case UT_CODE_b:
	    adix_caste_b( ind->type, onval, idptr, (ADIbyte *) odptr, &nerr );
	    break;
	  case UT_CODE_ub:
	    adix_caste_ub( ind->type, onval, idptr, (ADIubyte *) odptr, &nerr );
	    break;
	  case UT_CODE_w:
	    adix_caste_w( ind->type, onval, idptr, (ADIword *) odptr, &nerr );
	    break;
	  case UT_CODE_uw:
	    adix_caste_uw( ind->type, onval, idptr, (ADIuword *) odptr, &nerr );
	    break;
	  case UT_CODE_i:
	    adix_caste_i( ind->type, onval, idptr, (ADIinteger *) odptr, &nerr );
	    break;
	  case UT_CODE_r:
	    adix_caste_r( ind->type, onval, idptr, (ADIreal *) odptr, &nerr );
	    break;
	  case UT_CODE_d:
	    adix_caste_d( ind->type, onval, idptr, (ADIdouble *) odptr, &nerr );
	    break;
	  default:
	    adic_setecs( ADI__ILLOP, "Data conversion not supported on type ^TYP", status );
	    break;
	  }
	}
      }

/* Increment pointers stepping through declared data spaces */
    idptr += isecskip * ind->size;
    odptr += osecskip * outd->size;
    }

  if ( _ok(status) && _valid_q(outd->id) )
    _han_set(outd->id) = ADI__true;

  if ( nerr ) {
    adic_seteti( "NERR", nerr );
    adic_setecs( ADI__CONER, "^NERR data conversion error(s) occurred", status );
    }
  }


void adix_mtaid( ADIobj id, ADImtaPtr mta, ADIstatus status )
  {
  ADIclassDefPtr        tdef;

  _chk_stat;

  if ( ! _han_q(id) ) {
    adic_setecs( ADI__ILLKOP, "Cannot construct MTA for kernel object", status );
    }
  else
    {
    ADIobj      hid = _han_id(id);
    tdef = _DTDEF(id);                  /* Locate class definition block */

    mta->type = tdef->alloc.clas;       /* Set the transfer type */
    mta->tdef = tdef;

    mta->contig = ADI__true;            /* Some defaults */
    mta->trunc = ADI__false;

    mta->id = id;                       /* Store object being transferred */

    if ( _ary_q(hid) ) {                /* Array object? */
      ADIarrayPtr       adata = _ary_data(hid);
      int               i;
      int		*bdims;
      ADIobj		bdata;

      mta->size = tdef->alloc.size;

      mta->ndim = adata->ndim;
      for( i=0; i<mta->ndim; i++ )  	/* The dimensions of this data */
	mta->udims[i] = adata->dims[i];

/* Find origin in the base array, its dimensions and its data address */
      ADIaryBaseInfo( adata, NULL, mta->uorig, &bdims, &bdata, status );

/* Store the base dimensions */
      for( i=0; i<mta->ndim; i++ )
	mta->ddims[i] = bdims[i];

/* Decide whether data is contiguous in memory. The condition for this */
/* to be the case is that all but the last used dimension must be equal */
/* in size to the declared dimension */
      for( i=0; i<(mta->ndim-1); i++ )
	if ( mta->udims[i] != mta->ddims[i] )
	  mta->contig = ADI__false;

/* Must point to original data so that origin and base dimensions can be */
/* applied when copying data to and from slices */
      mta->data = (void *) _DTDAT(bdata);
      }
    else
      {
      mta->data = _DTDAT(id);
      mta->size = tdef->alloc.size;

      mta->ndim = 0;                      /* Flag as scalar */

      mta->udims[0] = 1;
      mta->uorig[0] = 1;
      mta->ddims[0] = 1;
      }
    }
  }

void adix_findmem( ADIobj id, char *mem, int mlen,
		   ADIobj **mad, ADIobj *parid, ADIstatus status )
  {
  ADIobj                curmem;
  ADIlogical            found = ADI__false;
  int                   imem = 1;
  ADIclassDefPtr        tdef;

  _chk_stat;

  _GET_NAME(mem,mlen);                	/* Import member name */

  tdef = _DTDEF(id);
  curmem = tdef->members;

  while ( ! (_null_q(curmem) || found) )
    {
    if ( strncmp( _mdef_name(curmem), mem, mlen ) )
      {
      imem++;
      curmem = _mdef_next(curmem);
      }
    else
      {
      found = ADI__true;
      if ( parid )
	*parid = _mdef_aname(curmem);
      *mad = _class_data(id) + imem - 1;
      }
    }

  if ( ! found ) {                      /* No such member? */
    adic_setetc( "CLS", tdef->name, 99 );
    adic_setetc( "MEM", mem, mlen );
    adic_setecs( ADI__NOMEMB, "Class ^CLS has no member called ^MEM", status );
    }
  }


void adix_chkget( ADIobj *id, ADIobj **lid, ADIstatus status )
  {
/* Check GET operation ok on this object. Allow GETs on primitive data
 * defined by ADI, and on class data where a default member is defined
 */
  if ( ! _valid_q(*id) ) {              /* Valid ADI identifier? */
    adic_setec( ADI__IDINV, status );
    }
  else if ( _han_q(*id) )               /* Only scalar object allowed */
    {
    ADIclassDefPtr      tdef = _DTDEF(*id);

/* Data not set? */
    if ( ! _han_set(*id) ) {
      adic_setec( ADI__NOTSET, status );
      }

    else if ( tdef->prim ) {            /* Object is primitive? */
      ADIclassCode cc = tdef->alloc.clas;

      if ( cc < UT_CODE_b ||            /* Primitive but not an ADI built */
	   cc > UT_CODE_c ) {           /* in type */
	adic_setetc( "CLS", tdef->name, 99 );
	adic_setecs( ADI__ISPRIM, "Cannot GET data of type ^CLS", status );
	}
      else
	*lid = id;
      }
    else                                /* User supplied a class object */
      {
      *lid = adix_defmem( id, status );

      if ( _null_q(**lid) ) {
	adic_setecs( ADI__NOTSET, "Default data member has no value", status );
	}
      }
    }
  else {
    adic_setecs( ADI__ILLKOP, "Cannot GET data from kernel objects", status );
    }
  }


void adix_chkput( ADIobj *id, ADIobj **lid, ADIstatus status )
  {
/* Check PUT operation ok on this object. Allow PUTs on primitive data
 * defined by ADI, and on class data where a default member is defined
 */
  if ( ! _valid_q(*id) )                /* Valid ADI identifier? */
    {
    }
  else if ( _han_q(*id) ) {             /* Only scalar object allowed */
    ADIclassDefPtr      tdef = _DTDEF(*id);

    if ( _han_readonly(*id) ) {
      adic_setecs( ADI__RDONLY, "Illegal write operation attempted", status );
      }

    else if ( tdef->prim ) {            /* Object is primitive? */
      ADIclassCode cc = tdef->alloc.clas;

      if ( cc < UT_CODE_b ||            /* Primitive but not an ADI built */
	   cc > UT_CODE_c ) {           /* in type */
	adic_setetc( "CLS", tdef->name, 99 );
	adic_setecs( ADI__ILLKOP, "Cannot PUT to object of type ^CLS", status );
	}
      else
	*lid = id;
      }
    else                                /* User supplied a class object */
      *lid = adix_defmem( id, status );
    }
  else {
    adic_setetc( "CLS", _DTDEF(*id)->name, 99 );
    adic_setecs( ADI__ILLKOP, "Cannot PUT data from kernel objects", status );
    }
  }


/*
 * Locate data given name and access mode
 */
void adix_locdat( ADIobj *id, char *name, int nlen, int flgs,
		  ADIobj **did, ADIobj *parid, ADIstatus status )
  {
  int		mode = ADI__AC_VALUE;	/* Default values */
  char		*lname = name;
  int   	lnlen = nlen;
  ADIlogical	iscreate =		/* Create access requested? */
		    (flgs & DA__CREATE);

  if ( parid )				/* Default is no parent */
    *parid = ADI__nullid;

  if ( name ) {				/* Decide on mode */
    if ( *name == '.' ) {		/* Property name preceded by period */
      mode = ADI__AC_PROPERTY;
      lname++;
      if ( nlen > 0 )
	lnlen--;
      }
    else if ( *name )			/* Don't allow null strings */
      mode = ADI__AC_MEMBER;
    }

  if ( mode == ADI__AC_VALUE ) { 	/* Simple value */
    if ( iscreate )
      adix_chkput( id, did, status );   /* Check write operation ok */
    else if ( flgs & DA__SET )
      adix_chkget( id, did, status );   /* Check read operation ok */
    else
      *did = id;
    }

  else if ( mode == ADI__AC_MEMBER ) {	/* Named component */
    if ( _struc_q(*id) )		/* Input object is a structure? */
      adix_pl_find( _struc_data(*id),	/* Find component insertion point */
		    lname, lnlen,
		    iscreate,
		    did, parid, status );
    else
      adix_findmem( *id, lname, lnlen,  /* Find member insertion point */
		    did, parid, status );
    }

  else if ( mode == ADI__AC_PROPERTY )	/* Property access */
    adix_pl_find( &_han_pl(*id), lname, /* Find property insertion point */
		  lnlen, iscreate,
		  did, parid, status );

  if ( _ok(status) && (*did) ) {        /* Ok so far and address defined? */
    if ( _valid_q(**did) ) {
      ADIobj  hid = _han_id(**did);
      if ( flgs & DA__ARRAY )		/* Object is required to be an array */
	if ( ! _ary_q(hid) )
	  adic_setecs( ADI__INVARG,
		"Array object expected", status );
      }
    }
  }

/*
 * Does a component exist?
 */
ADIlogical adix_there( ADIobj id, char *name, int nlen, ADIstatus status )
   {
  ADIobj	*lid;

/* Find data insertion point */
  adix_locdat( &id, name, nlen, DA__DEFAULT, &lid, NULL, status );

  if ( _ok(status) ) {			/* Status good if component exists */
    if ( lid )
      return _valid_q(*lid) ? ADI__true : ADI__false;
    else
      return ADI__false;
    }
  else {
    adix_errcnl( status );
    return ADI__false;
    }
  }

/*
 * Locate a component
 */
ADIobj adix_find( ADIobj id, char *name, int nlen, ADIstatus status )
  {
  ADIobj	*lid;

/* Find data insertion point */
  adix_locdat( &id, name, nlen, DA__DEFAULT, &lid, NULL, status );

  if ( _ok(status) ) {
/* Bump up reference count unless it's a kernel object */
    if ( _han_q(*lid) )
      adix_refadj( *lid, 1, status );
    return *lid;
    }
  else {
    adix_errcnl( status );
    return ADI__nullid;
    }
  }

ADIobj adix_clone( ADIobj id, ADIstatus status )
  {
  adix_refadj( id, 1, status );     	/* Bump up reference count and */

  return id;
  }

void adix_slice( ADIobj id, char *name, int nlen, int ndim,
		 int diml[], int dimu[], ADIobj *sid, ADIstatus status )
  {
  ADIobj      	*lid;

/* Find the data address. Must be an array, and its data must be defined */
  adix_locdat( &id, name, nlen, DA__ARRAY|DA__SET, &lid, NULL, status );

  /* Check not accessed */

  if ( _ok(status) ) {			/* Everything ok? */
    ADIarrayPtr		ary;
    int			idim;

    ary = _ary_data(_han_id(*lid));	/* Locate the array block */

    if ( ndim > ary->ndim )		/* Check slice dimensionality */
      adic_setecs( ADI__INVARG, "Slice dimensionality exceeds that of object",
		   status );
    else {

/* Check slice bounds validity */
      for( idim=0; idim<ndim && _ok(status); idim++ ) {
	if ( (diml[idim] < 1) )
	  adic_setecs( ADI__INVARG, "Slice lower bound is less than one",
		   status );
	else if ( dimu[idim] > ary->dims[idim] )
	  adic_setecs( ADI__INVARG, "Slice upper bound is greater than object dimension",
		   status );
	else if ( diml[idim] > dimu[idim] )
	  adic_setecs( ADI__INVARG, "Slice lower bound is higher than upper bound",
		   status );
	}

      if ( _ok(status) ) {		/* Bounds are ok? */
	int	dims[ADI__MXDIM];
	ADIobj	fdid;
	ADIobj  newid;

/*     Construct slice dimensions from lower and upper bounds */
	for( idim=0; idim<ndim; idim++ )
	  dims[idim] = dimu[idim] - diml[idim] + 1;

/*     Locate the array cell specified by indices supplied */
	fdid = ADIaryCell( ary, diml, status );

/*     Construct new array block, and increment reference count on */
/*     parent object */
	newid = ADIaryNew( ndim, dims, fdid,
			 adix_clone(id,	status), status );

/*     Wrap the new array block in a handle */
	newid = adix_newhan( newid, ADI__true, status );

/*     Inherit state from parent */
	_han_set(newid) = _han_set(id);

	 *sid = newid;			/* Return new object */
	}
      }
    }
  }


void adix_cell( ADIobj id, char *name, int nlen, int ndim,
		int index[], ADIobj *cid, ADIstatus status )
  {
  ADIobj      	*lid;

/* Find data address */
  adix_locdat( &id, name, nlen, DA__ARRAY|DA__SET, &lid, NULL, status );

  /* Check not accessed */

  if ( _ok(status) ) {			/* Everything ok? */
    ADIarrayPtr		ary;
    int			idim;

    ary = _ary_data(_han_id(*lid));	/* Locate the array block */

    if ( ndim > ary->ndim )		/* Check slice dimensionality */
      adic_setecs( ADI__INVARG, "Index dimensionality exceeds that of object",
		   status );
    else {
      for( idim=0;			/* Check indices validity */
	   idim<ndim && _ok(status);
	   idim++ ) {
	if ( (index[idim] < 1) )
	  adic_setecs( ADI__INVARG, "Index value is less than one",
		   status );
	else if ( index[idim] > ary->dims[idim] )
	  adic_setecs( ADI__INVARG, "Index value is greater than object dimension",
		   status );
	}

      if ( _ok(status) ) {		/* Indices are ok? */
	ADIobj	fdid;
	ADIobj  newid;

/* Locate the array cell */
	fdid = ADIaryCell( ary,	index, status );

	newid = adix_newhan( fdid,	/* Construct new handle */
	      ADI__true, status );
	_han_set(newid) = _han_set(id);

	adix_refadj( id, 1, status );	/* Bump up ref count on parent */

	 *cid = newid;			/* Return new object */
	}
      }
    }
  }


void adix_shape( ADIobj id, char *name, int nlen, int mxndim, int dims[],
		 int *ndim, ADIstatus status )
  {
  ADIobj      	*lid;

/* Find data address */
  adix_locdat( &id, name, nlen, DA__DEFAULT, &lid, NULL, status );

  if ( _ok(status) ) {
    ADIobj      	hid = _han_id(*lid);
    int		idim;

    if ( _ary_q(hid) ) {              	/* Array object? */
      ADIarrayPtr       adata = _ary_data(hid);

      if ( adata->ndim <= mxndim ) {
	*ndim = adata->ndim;

	for( idim=0; idim<adata->ndim; idim++ )
	  dims[idim] = adata->dims[idim];

        for( ; mxndim; idim++ )
	  dims[idim] = 0;
        }
      else
        adic_setecs( ADI__EXCEED,
		   "Number of array dimensions exceeds buffer size",
		   status );
      }
    else {
      *ndim = 0;
      for( idim=0; idim<mxndim; idim++ )
        dims[idim] = 0;
      }
    }
  }

/* Get value of object, or object component
 */
void adix_get_n( int clang, ADIobj id, char *name, int nlen,
		 int ndim, int mxdims[], ADIclassCode vtype, int vsize,
		 void *value, int nactdims[], ADIstatus status )
  {
  int           idim;                   /* Loop over dimensions */
  ADImta        imta;                   /* MTA for the object */
  ADIobj        *lid;                   /* Object to be accessed */
  ADImta        omta = _DEF_1D_MTA;     /* Output value MTA */

/* Find data insertion point */
  adix_locdat( &id, name, nlen, DA__SET, &lid, NULL, status );

  if ( _ok(status) ) {
    adix_mtaid( *lid, &imta, status );  /* Set input channel */

    omta.type = vtype;                  /* Set output channel */
    omta.data = value;
    omta.size = vsize;
    omta.ndim = ndim;
    for( idim=0; idim<ndim; idim++ )
      omta.ddims[idim] = mxdims[idim];
    omta.nulterm = clang;

    adix_mtacop( &imta, &omta, status );/* Perform transfer */

    if ( nactdims )			/* User wants actuals back? */
      for( idim=0; idim<ndim; idim++ )
	nactdims[idim] = omta.udims[idim];/* Actual data used */
    }
  }


void adix_chkmode( char *mode, int mlen, ADIacmode *amode,
		   ADIstatus status )
  {
  _chk_stat;				/* Check status on entry */

  _GET_STRING(mode,mlen);		/* Import the string */

  if ( ! strx_cmpi2c( mode, mlen, "READ", _MIN(4,mlen) ) )
    *amode = ADI__read;
  else if ( ! strx_cmpi2c( mode, mlen, "WRITE", _MIN(5,mlen) ) )
    *amode = ADI__write;
  else if ( ! strx_cmpi2c( mode, mlen, "UPDATE", _MIN(6,mlen) ) )
    *amode = ADI__update;
  else {
    adic_setetc( "MODE", mode, mlen );
    adic_setecs( ADI__INVARG, "Invalid access mode", status );
    }
  }

/* Look for map control object with specified mapping type. Return insertion
 * point for new list element if not present, otherwise the the address of
 * of an ADIobj pointing to the list node containing the amp control object */
ADIobj adix_loc_mapctrl( ADIobj id, ADIclassCode mtype, void *ptr,
			 ADIobj **ipoint, ADIstatus status )
  {
  ADIobj	*laddr = &_han_lock(id);
  ADIobj	curo = *laddr;
  ADIobj	lobj = ADI__nullid;

  _chk_stat_ret(ADI__nullid);

/* The default insertion point for new locks */
  *ipoint = laddr;

  while ( _null_q(lobj) && _valid_q(curo) ) {

/* Get next lock object, and test whether it's a map control */
    lobj = _CAR(curo);
    if ( _mapctrl_q(lobj) ) {
      if ( ptr ) {
	if ( ptr != _mapctrl_dptr(lobj) )
	  lobj = ADI__nullid;
	}
      else if ( mtype != _mapctrl_type(lobj) )
	lobj = ADI__nullid;
      }
    else
      lobj = ADI__nullid;

    if ( _null_q(lobj) ) {
      *ipoint = &_CDR(curo);
      curo = **ipoint;
      }
    }

  if ( _null_q(lobj) )
    *ipoint = laddr;

  return lobj;
  }

ADIobj adix_add_mapctrl( ADIobj id, ADIacmode mode, ADIclassCode mtype,
			 size_t nbyte, ADIlogical dynamic,
			 ADIstatus status )
  {
  ADIobj	*ipoint;
  ADIobj	lobj;
  ADImapCtrl	*mctrl = NULL;
  ADIobj	newm;			/* New object */

/* Check inherited status on entry */
  _chk_stat_ret(ADI__nullid);

/* Look for existing map control with similar mapping type. Note that if
 * mode is not ADI__read, then any existing mapping control object on the
 * lock list causes an error */
  lobj = adix_loc_mapctrl( id, mtype, NULL, &ipoint, status );
  if ( _valid_q(lobj) && (mode != ADI__read) ) {
    mctrl = _mapctrl_data(lobj);

    adix_acc2tok( "ACC", mctrl->mode );
    adic_setecs( ADI__MAPPED, "Object is already mapped for ^ACC access", status );
    }

  if ( ! mctrl ) {
    newm = adix_cls_alloc( &KT_DEFN_mapctrl,  /* Allocate new map control */
			status );

    mctrl = _mapctrl_data(newm);
    lobj = ADI__nullid;
    }

/* No access clashes or allocation errors? */
  if ( _ok(status) ) {
    if ( _valid_q(lobj) )
      mctrl->nref++;
    else {
      mctrl->mode = mode;
      mctrl->nbyte = nbyte;
      mctrl->type = mtype;
      mctrl->nref = 1;
      mctrl->dynamic = dynamic;

      if ( dynamic )			/* Dynamic data is required? */
	mctrl->dptr = (void *) adix_mem_alloc( nbyte, status );

/* Append new lock to object's locking list */
      *ipoint = lstx_append( *ipoint,
			lstx_cell( newm, ADI__nullid, status ), status );
      }
    }
  else
    newm = ADI__nullid;

  return newm;				/* Set return value */
  }


/* Map value of object, or object component
 */
void adix_map_n( int clang, ADIobj id, char *name, int nlen,
		 char *mode, int mlen,
		 ADIclassCode vtype, int vsize,
		 void **vptr, ADIstatus status )
  {
  int           idim;                   /* Loop over dimensions */
  ADIacmode	imode;			/* Mapping mode */
  ADImta        imta;                   /* MTA for the object */
  ADIobj        *lid;                   /* Object to be accessed */
  int		damode;			/* Data access mode */
  ADIobj	mctrl;			/* Map control object */
  ADImta        omta = _DEF_1D_MTA;     /* Output value MTA */

/* Validate the mapping mode string */
  adix_chkmode( mode, mlen, &imode, status );

/* If writing, allow creation, otherwise insist on data being set */
  if ( imode == ADI__write )
    damode = DA__CREATE;
  else
    damode = DA__SET;

/* Find data insertion point */
  adix_locdat( &id, name, nlen,	damode, &lid, NULL, status );

  if ( _ok(status) ) {
    size_t	nbyte = 0;
    ADIlogical	dynamic = ADI__false;

    adix_mtaid( *lid, &imta, status );  /* Set input channel */

/* Need dynamic memory if different types or if mapped object is */
/* non-contiguous in memory */
    if ( (vtype!=imta.type) || ! imta.contig ) {
      dynamic = ADI__true;
      nbyte = vsize * ADIaryCountNelm( imta.ndim, imta.udims );
      }

/* Create the mapping control object */
    mctrl = adix_add_mapctrl( *lid, imode, vtype, nbyte, dynamic, status );

/* Perform data conversion if dynamic, otherwise just point to the input */
/* data object */
    if ( dynamic ) {
      omta.type = vtype;                  /* Set output channel */
      omta.data = _mapctrl_dptr(mctrl);
      omta.size = vsize;
      omta.ndim = imta.ndim;
      for( idim=0; idim<imta.ndim; idim++ )
	omta.ddims[idim] = imta.udims[idim];
      omta.nulterm = clang;

      adix_mtacop( &imta, &omta, status );/* Perform transfer */
      }
    else
      _mapctrl_dptr(mctrl) = (void *) imta.data;
    }

/* Set value of returned pointer */
  *vptr = _ok(status) ? _mapctrl_dptr(mctrl) : NULL;
  }

void adix_map_t( int clang, ADIobj id, char *name, int nlen,
		 char *cls, int clen, char *mode, int mlen,
		 void **vptr, ADIstatus status )
  {
  ADIclassDef		*tdef;		/* Class definition */

  _chk_stat;                            /* Standard checks */

  _GET_NAME(cls,clen);                	/* Import string */

/* Locate the allocator object */
  tdef = ADIkrnlFindClsC( cls, clen, status );

  if ( tdef )
    adix_map_n( clang, id, name, nlen, mode, mlen, tdef->alloc.clas,
		tdef->alloc.size, vptr, status );
  }

void adix_unmap_n( ADIobj id, char *name, int nlen,
		   void *vptr, ADIstatus status )
  {
  ADIobj	*ipoint;
  ADIobj        *lid;                   /* Object to be accessed */
  ADIobj	lobj;			/* The object lock */

/* Find data address */
  adix_locdat( &id, name, nlen, DA__DEFAULT, &lid, NULL, status );

  lobj = adix_loc_mapctrl( id, 0, vptr, &ipoint, status );

  if ( _valid_q(lobj) ) {
    ADImapCtrl	*mctrl = _mapctrl_data(lobj);

    if ( vptr )
      mctrl->nref--;
    else
      mctrl->nref = 0;

    if ( ! mctrl->nref ) {
      ADIobj	linkobj;

/* Do we have to convert the data back to the original object type? */
      if ( mctrl->dynamic ) {
	ADImta	imta;			/* MTA describing mapped data */
	ADImta	omta;			/* MTA describing mapping object */

	adix_mtaid( *lid, &omta, status );  /* Set output channel */

	imta = omta;
	imta.type = mctrl->type;
	imta.data = mctrl->dptr;

	adix_mtacop( &imta, &omta, status );/* Perform transfer */
	}

/* If the mapping mode was write, the data is now set */
      if ( mctrl->mode == ADI__write )
	_han_set(*lid) = ADI__true;

/* Keep a copy of the next lock object in the chain */
      linkobj = _CDR(*ipoint);
      _CDR(*ipoint) = ADI__nullid;

/* Erase the list element and the mapping lock object */
      adic_erase( ipoint, status );

/* Ensure list links are kept up to date */
      *ipoint = linkobj;
      }
    }
  }

/* Write data to a slot address. If the address is NULL, create a new object
 * otherwise write the data.
 *
 *   Method :
 *
 *    IF (object address holds null) THEN
 *      create object with dimensions from <mta>
 *    ENDIF
 *    IF (constructor defined and object data unset) THEN
 *      invoke constructor
 *      set dataset bit if <mta> data defined
 *    ENDIF
 *
 */
void adix_wdata( ADIobj *id, ADImtaPtr mta, ADIstatus status )
  {
  ADIclassDef	  *ddef = mta->tdef;
  ADIclassDef     *adef = ddef;
  ADImta          omta;                 /* MTA for the object */

/* If the data slot is empty create the data object using the class */
/* definition supplied, otherwise locate the class definition of the */
/* existing data */
  if ( _null_q(*id) )
    *id = adix_cls_nalloc( adef, mta->ndim, mta->ddims, status );
  else
    adef = _DTDEF(*id);

/* Constructor exists, and data not set? */
  if ( adef->mcon && ! _han_set(*id) ) {
    (*adef->mcon)( *id, mta, status );  /* Invoke the constructor */

    if ( _ok(status) && mta->data )     /* Set dataset bit if data and ok */
      _han_set(*id) = ADI__true;
    }

  else if ( mta->data ) {               /* Input data defined? */
    adix_mtaid( *id, &omta, status );   /* Set output channel */

    adix_mtacop( mta, &omta, status );  /* Perform transfer */

    if ( _ok(status) )                  /* Everything ok? */
      _han_set(*id) = ADI__true;
    }
  }


/* Object creation
 */
void adix_new_n( ADIlogical clang, ADIobj pid, char *name, int nlen,
		 int ndim, int dims[], void *value,
		 ADIclassDef *tdef, int vsize,
		 ADIobj *id, ADIstatus status )
  {
  int           idim;                   /* Loop over dimensions */
  ADImta        imta = _DEF_1D_MTA;     /* MTA for the object */
  ADIobj        *newid = NULL;    	/* The newly created object */
  ADIobj	parid = pid;		/* Parent object */

  if ( _valid_q(pid) )			/* If structured data */
    adix_locdat( &pid, name, nlen,	/* Find data insertion point */
		 DA__CREATE, &newid,
		 &parid, status );
  else {
    *id = ADI__nullid;
    newid = id;
    }

  imta.data = value;                    /* Set input channel */
  imta.size = vsize;
  imta.tdef = tdef;
  imta.type = tdef->alloc.clas;
  imta.ndim = ndim;
  for( idim=0; idim<ndim; idim++ )
    imta.ddims[idim] = imta.udims[idim] = dims[idim];
  imta.nulterm = clang;

/* Write the data */
  adix_wdata( newid, &imta, status );

/* Everything went ok? */
  if ( _ok(status) ) {

/* Set the parent object if defined */
    if ( _valid_q(parid) )
      _han_pid(*newid) = parid;

/* Return new object if wanted */
    if ( id )
      *id = *newid;
    }
  }


/* Put value of object, or object component
 */
void adix_put_n( int clang, ADIobj id, char *name, int nlen,
		 int ndim, int dims[], ADIclassDef *tdef,
		 int vsize, void *value, ADIstatus status )
  {
  int           idim;                   /* Loop over dimensions */
  ADImta        imta = _DEF_1D_MTA;     /* Input value MTA */
  ADIobj        *lid;                   /* Object to be accessed */
  ADIobj	parid;			/* Parent object */

/* Find the data insertion point */
  adix_locdat( &id, name, nlen, DA__CREATE, &lid, &parid, status );

  if ( _ok(status) ) {                  /* Located insertion point ok? */
    imta.data = value;                  /* Set input channel */
    imta.ndim = ndim;
    for( idim=0; idim<ndim; idim++ )
      imta.ddims[idim] = imta.udims[idim] = dims[idim];
    imta.tdef = tdef;
    imta.type = tdef->alloc.clas;
    imta.size = vsize;
    imta.nulterm = clang;

/* Write the data */
    adix_wdata( lid, &imta, status );

/* Evrything went ok? */
    if ( _ok(status) ) {

/* Parent object is defined? */
      if ( _valid_q(parid) )
	_han_pid(*lid) = parid;

/* Set dataset bit if input data supplied */
      if ( imta.data )
	_han_set(id) = ADI__true;
      }
    }
  }


/* Set value of object, or object component
 */
void adix_set_n( int clang, ADIobj id, char *name, int nlen,
		 int ndim, int dims[], ADIclassDef *tdef,
		 int vsize, void *value, ADIstatus status )
  {
  ADImta        imta = _DEF_1D_MTA;     /* Input value MTA */
  ADIlogical    ismemb = ADI__false;    /* Member access? */
  ADIobj        *lid;                   /* Object to be accessed */
  ADIobj	parid;			/* Parent object identifier */

/* Find data insertion point */
  adix_locdat( &id, name, nlen, DA__CREATE, &lid, &parid, status );

  if ( _ok(status) ) {                  /* Located insertion point ok? */
    int         idim;

    imta.data = value;                  /* Set input channel */
    imta.ndim = ndim;
    for( idim=0; idim<ndim; idim++ )
      imta.ddims[idim] = imta.udims[idim] = dims[idim];
    imta.tdef = tdef;
    imta.type = tdef->alloc.clas;
    imta.size = vsize;
    imta.nulterm = clang;

/* Write the data */
    adix_wdata( lid, &imta, status );

    if ( _ok(status) &&                 /* Set parent object */
	 _valid_q(parid) )
      _han_pid(*lid) = parid;
    }
  }


ADIobj adix_copy( ADIobj id, ADIstatus status )
  {
  ADIarrayPtr   adata;
  ADIobj        rval = id;
  ADIobj        temp;

  if ( _valid_q(id) )			/* Trap null objects */
   if ( _han_q(id) ) {                  /* Don't copy kernel objects */
    if ( _han_readonly(id) )            /* Object is readonly? */
      rval = adix_clone(id,status);
    else
      {
      ADIobj    hid = _han_id(id);      /* Object pointed to by handle */
      ADIclassDef    *tdef;

      tdef = _DTDEF(hid);               /* Locate object class definition */

      if ( _ary_q(hid) ) {              /* Array kernel object */
	adata = _ary_data(hid);

	temp = adix_cls_nalloc( tdef,         /* Create new object */
	  adata->ndim, adata->dims, status );
	}
      else if ( tdef->prim ) {          /* Primitive (ie. non-class) data? */
	temp = adix_cls_alloc( tdef,         /* Create new object */
	   status );

	if ( tdef->mcon ) {             /* Constructor defined? */
	  ADImta        imta;

	  adix_mtaid( id, &imta, status );

	  (*tdef->mcon)( temp, &imta, status );
	  }
	else
	  memcpy( _DTDAT(temp), _DTDAT(id), tdef->alloc.size );

	if ( _ok(status) ) {
	  _han_set(temp) = ADI__true;
	  rval = temp;
	  }
	}
      else {                            /* Class instance */
	int     i;
	ADIobj  *iobj,*oobj;

/* Create the new object */
	temp = adix_cls_alloc( tdef, status );

	iobj = _class_data(id);
	oobj = _class_data(temp);

	for( i=0; i<tdef->nslot; i++)
	  *oobj++ = adix_copy( *iobj++, status );

	if ( _ok(status) )
	  rval = temp;
	}
      }
    }

  return rval;
  }


void adix_print( ADIobj stream, ADIobj id, ADIlogical value_only,
		 ADIstatus status )
  {
  ADIobj                curmem;
  ADIobj                *iobj;
  ADIclassDef           *tdef;

  _chk_stat;

  if ( id == ADI__nullid )
    ADIstrmPutStr( stream, "<null>", _CSM, status );
  else if ( _han_q(id) ) {
    ADIobj      hid = _han_id(id);

    if ( ! value_only )
      ADIstrmPrintf( stream, "< {%d:%d->%d:%d}, nref=%d, ", status,
	    _ID_IBLK(id), _ID_SLOT(id),
	    _ID_IBLK(hid), _ID_SLOT(hid),
	    _han_ref(id) );

    if ( _krnl_q(hid) )
      adix_print( stream, hid, ADI__false, status );
    else {
      tdef = _DTDEF(hid);               /* Locate class definition block */

      if ( ! value_only )
	ADIstrmPutStr( stream, tdef->name, _CSM, status );
/*      if ( ! _han_set(id) )
	{
	if ( ! value_only )
	  printf( ", " );
	printf( "not set" );
	}
      else */ if ( tdef->prnt ) {
	if ( ! value_only )
	  ADIstrmPrintf( stream, ", ", status );
	(*tdef->prnt)( stream, id, status );
	}
      else if ( ! _prim_q(hid) ) {
	iobj = _class_data(id);
	for( curmem = tdef->members; _valid_q(curmem);
	     curmem = _mdef_next(curmem), iobj++ ) {
	  ADIstrmPrintf( stream, "\n  ", status );
	  adix_prnt_c( stream, _mdef_aname(curmem), status );
	  ADIstrmPrintf( stream, " = ", status );
	  adix_print( stream, *iobj, ADI__true, status );
	  }
	ADIstrmPrintf( stream, "\n", status );
	}
      }
    if ( _valid_q(_han_pl(id)) ) {
      ADIobj curp = _han_pl(id);
      ADIstrmPutStr( stream, ", props = {", _CSM, status );
      do {
	adix_prnt_c( stream, _CAAR(curp), status );
	ADIstrmPutCh( stream, '=', status );
	adix_print( stream, _CDAR(curp), ADI__true, status );
	curp = _CDR(curp);
	ADIstrmPutCh( stream, (curp==ADI__nullid) ? '}' : ',', status );
	}
      while ( _valid_q(curp) && _ok(status) );
      }

    if ( ! value_only )
      ADIstrmPutCh( stream, '>', status );
    }
  else if ( _cdef_q(id) ) {
    ADIclassDefPtr  tdef = _cdef_data(id);

    ADIstrmPrintf( stream, "< Class definition %s", status, tdef->name );
    if ( tdef->prim ) {
      ADIstrmPrintf( stream, ", primitive, size = %d bytes",
		     status, tdef->alloc.size );
      }
    else {
      ADIobj            cmem = tdef->members;
      ADIobj            cpar = tdef->superclasses;
      int               imem;

      if ( _null_q(cpar) )
	ADIstrmPutStr( stream, ", base class", _CSM, status );
      else {
	ADIstrmPutStr( stream, ", superclasses {", _CSM, status );
	for( ; _valid_q(cpar); cpar = _pdef_next(cpar) ) {
	  adix_print( stream, _pdef_name(cpar), ADI__true, status );
	  if ( _valid_q(cpar) )
	    ADIstrmPutCh( stream, ' ', status );
	  }
	ADIstrmPutCh( stream, '}', status );
	}
      ADIstrmPrintf( stream, ",\n", status );

      for( imem=0; _valid_q(cmem); cmem = _mdef_next(cmem), imem++ ) {
	ADIstrmPutStr( stream, "  ", _CSM, status );
	if ( _valid_q(_mdef_defcls(cmem) ))
	  ADIstrmPrintf( stream, "%s ", status,
		_cdef_data(_mdef_defcls(cmem))->name );
	adix_prnt_c( stream, _mdef_aname(cmem), status );
	if ( imem == tdef->defmem )
	  ADIstrmPutCh( stream, '*', status );
	ADIstrmPutCh( stream, ' ', status );
	if ( _valid_q(_mdef_cdata(cmem) )) {
	  ADIstrmPutStr( stream, "= ", _CSM, status );
	  adix_print( stream, _mdef_cdata(cmem), ADI__true, status );
	  }
	ADIstrmPrintf( stream, "\n", status );
	}
      }
    ADIstrmPutStr( stream, "  >", _CSM, status );
    }
  else if ( _ary_q(id) ) {
    ADIarrayPtr ary = _ary_data(id);
    int         i;

    if ( _krnl_q(ary->data) )
      ADIstrmPutStr( stream, "generic array", _CSM, status );
    else
      ADIstrmPutStr( stream, _DTDEF(ary->data)->name, _CSM, status );
    ADIstrmPutCh( stream, '[', status );
    for( i=0; i<ary->ndim; i++ )
      ADIstrmPrintf( stream, "%d%c", status,
		ary->dims[i], ((i+1)==ary->ndim) ? ']' : ',' );
    }
  else {
    tdef = _DTDEF(id);               /* Locate class definition block */

    if ( tdef->prnt )
      (*tdef->prnt)( stream, id, status );
    else
      ADIstrmPrintf( stream, "<%s %d:%d, nref=%d> ", status, tdef->name,
	    _ID_IBLK(id), _ID_SLOT(id), _han_ref(id) );

    value_only = ADI__true;
    }

  if ( ! value_only )
    ADIstrmPrintf( stream, "\n", status );
  }



/*
 *  C generic dispatch for methods of form void xx(ADIobj,ADIobj,status)
 *  Used for SetLink
 */
ADIobj adix_cdsp_voo( ADICB rtn, int narg, ADIobj args[], ADIstatus status )
  {
  (*((ADIooCB) rtn))( args[0], args[1], status );

  return ADI__nullid;
  }

ADIobj adix_cdsp_vo( ADICB rtn, int narg, ADIobj args[], ADIstatus status )
  {
  (*((ADIoCB) rtn))( args[0], status );

  return ADI__nullid;
  }

/*
 *  Fortran generic dispatch for methods of form void xx(ADIobj,ADIobj,status)
 *  Used for SetLink
 */
ADIobj adix_fdsp_voo( ADICB *rtn, int *narg, ADIobj args[], ADIstatus status )
  {
  (*((ADIfooCB) *rtn))( args+0, args+1, status );

  return ADI__nullid;
  }

ADIobj adix_fdsp_vo( ADICB *rtn, int *narg, ADIobj args[], ADIstatus status )
  {
  (*((ADIfoCB) *rtn))( args+1, status );

  return ADI__nullid;
  }



char *adix_name( ADIobj id, ADIstatus status )
  {
  char		*rval = NULL;

  if ( _han_q(id) ) {
    if ( _null_q(_han_pid(id)) )
      adic_setec( ADI__NONAME, status );
    else
      rval = _str_dat(_han_pid(id));
    }
  else
    adic_setec( ADI__NONAME, status );

  return rval;
  }

char *adix_qcls( ADIobj id, ADIstatus status )
  {
  ADIclassDefPtr        tdef;

  _chk_stat_ret(NULL);

  tdef = _DTDEF(id);                    /* Locate class definition */

  return tdef->name;                    /* Return name */
  }


void adix_cerase( ADIobj id, char *member, int mlen, ADIstatus status )
  {
  ADIobj        *mid;

  _chk_stat;

  adix_findmem( id, member, mlen, &mid, NULL, status );

  if ( _valid_q(*mid) )
    adix_erase( mid, 1, status );
  }


void adix_cputid( ADIobj id, char *name, int nlen,
		  ADIobj value, ADIstatus status )
  {
  ADIobj        *mid;
  ADIobj	parid;			/* Parent object identifier */

  _chk_stat;

  adix_locdat( &id, name, nlen,		/* Find data insertion point */
	       DA__CREATE, &mid,
	       &parid, status );

  if ( _null_q(*mid) )			/* Set object slot */
    *mid = value;
  else {
    adix_erase( mid, 1, status );
    *mid = value;
    }

  if ( _valid_q(parid) )   	        /* Set parent object */
    _han_pid(value) = parid;
  }

void adix_cputiid( ADIobj id, ADIobj name, ADIobj value, ADIstatus status )
  {
  ADIobj        *mid;
  ADIobj	parid;			/* Parent object identifier */
  ADIsegmentPtr sptr;

  _chk_stat;

  sptr = _seg_data(name);		/* Locate string data */

/* Find data insertion point */
  adix_locdat( &id, sptr->data, sptr->len, DA__CREATE,
	       &mid, &parid, status );

  if ( _null_q(*mid) )
    *mid = value;
  else {
    adix_erase( mid, 1, status );
    *mid = value;
    }

  if ( _valid_q(parid) )   	        /* Set parent object */
    _han_pid(value) = parid;
  }



/* Generic and method handling
 *
 *  adix_locmco         - find method combination given name
 *  adix_newmco         - fill out a new method combination structure
 *  adix_defmco         - high level method combination definition
 *  adix_delmco         - destroy method combination object
 *  adix_stdmcf         - "Standard" method combination
 */

ADIobj adix_locmco( ADIobj name, ADIstatus status )
  {
  ADIobj        curm = ADI_G_firstmco;
  ADIobj        mcid = ADI__nullid;
  ADIlogical    found = ADI__false;

  _chk_stat_ret(ADI__nullid);

  while ( ! (_null_q(curm) || found) )
    {
    mcid = _CAR(curm);

    if ( _mco_name(mcid) == name )
      found = ADI__true;
    else
      curm = _CDR(curm);
    }

  return found ? mcid : ADI__nullid;
  }


ADIobj adix_newmco( ADIobj name, ADIobj cexec, ADIstatus status )
  {
  ADIobj        newid;                  /* New mco object */

  newid = adix_cls_alloc( &KT_DEFN_mco, /* Allocate new array block */
			 status );

  if ( _ok(status) )                    /* Allocation went ok? */
    {
    _mco_name(newid) = name;            /* Fill fields in */
    _mco_cexec(newid) = cexec;

    _LST_APPEND(ADI_G_mcolink,newid);   /* Append to system list */
    }

  return newid;                         /* Return new block */
  }


void adix_delmco( ADIobj id, int nval, ADIstatus status )
  {
  KT_CTYPE_mco  *dptr = _mco_data(id);
  int           i;

  for( i=0; i<nval; i++, dptr++ )
    {
    adix_erase( &dptr->name, 1, status );
    adix_erase( &dptr->cexec, 1, status );
    }
  }


void adix_defmcf( char *name, int nlen,
		  ADIobj rtn, ADIobj *id, ADIstatus status )
  {
  ADIobj        aname;                  /* ADI string version of name */
  ADIobj        mcid = ADI__nullid;     /* The new object */

  _chk_stat;                            /* Check status on entry */

  _GET_NAME(name,nlen);               	/* Import string used in this rtn */

  if ( _null_q(rtn) ) {                 /* Check not null routine */
    adic_setecs( ADI__INVARG,
		 "Illegal null method combination executor", status );
    }
  else
    {
    aname = adix_cmn( name, nlen,       /* Introduce name to table */
			status );

    mcid = adix_locmco( aname,          /* Try and locate it */
			status );

    if ( _valid_q(mcid) ) {
      adic_setecs( ADI__EXISTS,
		   "Method combination form already exists", status );
      }
    else
      {
      mcid = adix_newmco( aname, rtn,   /* Introduce to system */
			   status );

      if ( _ok(status) && id )          /* Set return value */
	*id = mcid;
      }
    }
  }


KT_CTYPE_cdef *adix_loccls( ADIobj name, ADIstatus status )
  {
  KT_CTYPE_cdef *tdef = ADI_G_firstcdef;
  ADIlogical    found = ADI__false;

  _chk_stat_ret(NULL);

  while ( tdef && ! found )
    {
    if ( tdef->aname == name )
      found = ADI__true;
    else
      tdef = tdef->link;
    }

  return tdef;
  }

/*
 * Does the class c2 exist in the inheritance list of c1?
 */
ADIlogical adix_chkder( ADIclassDef *c1, ADIclassDef *c2, ADIstatus status )
  {
  ADIlogical    derived = ADI__true;    /* True by default */

  _chk_stat_ret(ADI__false);            /* Check status on entry */

  if ( c1 != c2 ) {                     /* A class is derived from itself */
    ADIobj curp = c1->superclasses;

    while ( _valid_q(curp) ) {          /* Loop over superclasses */
      ADIclassDef *ptdef;

      ptdef = _cdef_data(               /* Locate parent class definition */
		_pdef_clsid(curp));

      if ( c2 == ptdef )                /* Match found? */
	return ADI__true;
		else if ( adix_chkder( ptdef,     /* Or in parent classes of parent? */
		c2, status ) )
	return ADI__true;

      curp = _pdef_next(curp);		/* Next superclass */
      }

    derived = ADI__false;               /* Fall though -> not derived */
    }

  return derived;
  }

/*  Rank a list of methods in descending priority order given a bunch of
 *  arguments */
void adix_primth( int narg, int farg, int nmth,
                  ADIobj *mlist, ADIstatus status )
  {
  ADIobj	curp;
  int		iarg;
  ADIobj	newlist = ADI__nullid;	/* The users reordered list */
  int		nleft;			/* Number of methods remaining */
  ADIobj	*ipoint = &newlist;	/* List insertion point */

  _chk_stat;

/* If nthm = 0 was supplied, then do them all */
  if ( ! nmth ) {
    curp = *mlist;
    while ( _valid_q(curp) ) {
      nmth++;
      curp = _CDR(curp);
      }
    }

/* Check case of only one method */
  if ( nmth > 1 ) {

/* While more input methods and more arguments to process */
    for( nleft=nmth, iarg=farg; iarg<narg && (nleft>1) && _ok(status); iarg++ ) {

      int	imth;
      ADIobj	curp;
      ADIobj 	dslist = ADI__nullid;	/* Direct superclass list */
		ADIobj 	mclist = ADI__nullid;	/* Method arg class list */
      ADIobj	rlist; 			/* Ranked class list */

/* Gather a list of the classes which appear at this argument position for */
/* each of the remaining methods */
      curp = *mlist;
      imth = 1;
      while ( imth <= nmth ) {
	ADIobj 	adslist;	/* Method arg direct superclass list */
	ADIobj mthd = _CAR(curp);
	ADIobj cura = _mthd_args(mthd);
	ADIobj acls;
	int	jarg = iarg;

/* Skip to the iarg'th argument for this method */
	while ( jarg-- )
	  cura = _CDR(cura);

/* Locate class definition object */
	acls = ADIkrnlFindClsI( _CAR(cura), status );

/* Add the direct-superclass list to our list of such */
	adslist = _cdef_data(acls)->dslist;
	if ( _valid_q(adslist) )
	  lstx_addtoset( &dslist, _cdef_data(acls)->dslist, status );

/* Add to our set of list elements */
        lstx_addtoset( &mclist, _CAR(cura), status );

/* Next method */
        curp = _CDR(curp); imth++;
        }

/* Order the list of classes appearing in mclist into ascending priority */
      rlist = adix_estab_ord( mclist, dslist, status );

/* Now process the list of methods. Start at the beginning of the ranked
 * list and shuffle the remaining methods with this class to the head of the
 * the list. */
      curp = rlist;
      while ( _valid_q(curp) && (nleft>1) ) {

	int	nmoved = 0;
        ADIobj	curm = *mlist;
		  ADIobj  *cpoint = mlist;
	int	imth = 0;
        ADIlogical anyout = ADI__false;

        while ( imth < nleft ) {

          int	  jarg = iarg;
          ADIobj  mthd = _CAR(curm);
          ADIobj  cura = _mthd_args(mthd);
          ADIobj  *anext = &_CDR(curm);
          ADIobj  next = *anext;

/*    Skip to the iarg'th argument for this method */
          while ( jarg-- )
            cura = _CDR(cura);

/*    This method's argument matches the current one in the ranked list?
 *    Shove to the front of the list, unless there are no intervening out
 *    of order methods */
          if ( _CAR(curp) == _CAR(cura) ) {
	    if ( anyout ) {
              ADIobj old = *mlist;
				  *cpoint = next;
              *anext = old;
              *mlist = curm;
              }
            nmoved++;
            }
          else {
            anyout = ADI__true;
            cpoint = anext;
            }

          curm = next; imth++;
          }

/* All the high priority methods have been transferred to the head of the
 * the list. If more than one method, invoke this routine recursively to
 * order them. */
        if ( nmoved > 1 )
          adix_primth( narg, farg+1, nmoved, mlist, status );

/* Move all these methods to our output list. They go on the end! Advance
 * the output list insertion point */
		  *ipoint = *mlist;
        nleft -= nmoved;
        while ( nmoved-- )
          ipoint = & _CDR(*ipoint);
        }

      }

/* Tack any remaining method on the end */
    *ipoint = *mlist;

/* Set user list to reordered list */
    if ( _ok(status) )
      *mlist = newlist;
    }
  }

/*  Gather applicable methods for the given generic function, given
 *  arguments and the allowable method forms specified. Each method
 *  is processed as follows,
 *
 *  - for each method
 *    - Is the name the same as the generic?
 *      N: next method
 *    - Is the number of arguments the same as the generic?
 *      N: next method
 *    - Are we collecting this method form?
 *      N: next method
 *    - for each argument
 *      - is user arg the same class as the method arg, or a derived
 *        class of the method arg?
 *        N: next method
 *      - add to method list of each form
 *      end for each
 *    end for each
 *  - for each method form
 *    - add the method arg class to the list for this form, and the
 *      direct superclass list for this arg to the list of such for
 *      this form UNLESS the arg is already present in the former list
 *    end for each
 */
void adix_gthmth( ADIobj gen, int narg, ADIobj args[], int nmform,
		  ADIobj *mform[], ADIlogical mfopri[],
		  ADIobj mlist[], ADIstatus status )
  {
  ADIobj        acur, cur;              /* Cursors over lists */
  ADIlogical    found;                  /* Found method? */
  KT_CTYPE_gnrc *gdata;                 /* Generic function data block */
  int           i;
  int           iform;                  /* Form id of method */
  KT_CTYPE_mthd *mdata;                 /* Method data block */
  ADIobj        mth;                    /* Method object */
  int           nmth = 0;               /* Number of applicable methods */
  ADIlogical    ok;                     /* Validity test */

  _chk_stat;                            /* Check status on entry */

  gdata = _gnrc_data(gen);              /* Locate generic data */

  for( i=0; i<nmform; i++ )             /* Initialise lists */
    mlist[i] = ADI__nullid;

  cur = gdata->mlist;			/* This generic's method list */

  while ( _valid_q(cur) ) {
    mth = _CAR(cur);
	 mdata = _mthd_data(mth);            /* Locate method data */
    cur = _CDR(cur);                    /* Advance to next method */

    found = ADI__false;                 /* Suitable form? */
    for( iform=0; iform<nmform && ! found ; )
      if ( *(mform[iform]) == mdata->form )
	found = ADI__true;
      else
	iform++;
    if ( ! found ) continue;

    acur = mdata->args;                 /* Start of method args */

    ok = ADI__true;
    for( i=0; i<narg && ok; i++ )       /* Check arguments compatible */
      {
      ADIclassDefPtr    uargc;
      ADIclassDefPtr    margc;
      ADIobj            aclsnam;

      uargc = _DTDEF(args[i]);          /* Get class block of user arg */

		aclsnam = _CAR(acur);
      margc = adix_loccls( aclsnam, status );

      if ( ! adix_chkder(uargc,margc,   /* Method arg class must exist in the */
			      status) ) /* inheritance list of the user arg */
	ok = ADI__false;

      acur = _CDR(acur);
      }

    if ( ! ok )                         /* Arguments are incompatible */
      continue;

    mlist[iform] = lstx_cell( mth,      /* Method is applicable! */
	   mlist[iform], status );

    nmth++;
    }

  if ( nmth ) {                         /* Check for no methods */
    for ( iform=0;                      /* Loop over forms */
	  iform<nmform; iform++ )
		if ( _valid_q(mlist[iform]) ) {   /* Methods for this form? */

/*     Order the methods in descending order of priority */
        adix_primth( narg, 0, 0, mlist + iform, status );

/*     Callee wants list in ascending priority order? */
	if ( ! mfopri[iform] )
	  mlist[iform] =
	     lstx_revrsi( mlist[iform], status );
	}
    }
  else
    adic_setecs( ADI__NOMTH, "No methods matching signature ^SIG", status );
  }


/*  Implements "Standard" method combination. The method forms "Around",
 *  "Before" , "Primary" and "After" are all used.
 *
 */
ADIobj adix_stdmcf( ADIobj gen, int narg, ADIobj args[], ADIstatus status )
  {
  static                                /* Forms we want to gather */
    ADIobj *mforms[] =
    {
    &DnameAround, &DnameBefore, &DnamePrimary, &DnameAfter
    };

  static                                /* Highest priority first per form */
    ADIlogical mfopri[] =
    {
    ADI__true, ADI__true, ADI__true, ADI__false
    };

  ADIobj        curp;                   /* Cursor over method list */
  ADIlogical    finished = ADI__false;  /* Quit back to caller? */
  ADIobj        mlists[4];              /* Methods to be executed */
  ADIobj        mresult;                /* Method result */
  ADIobj        rval = ADI__nullid;     /* Return value */

  _chk_stat_ret(ADI__nullid);

/* Gather the methods */
  adix_gthmth( gen, narg, args, 4, mforms, mfopri, mlists, status );
  _chk_stat_ret(ADI__nullid);

  curp = mlists[0];                     /* First the Around methods */
  while ( _valid_q(curp) && ! finished && _ok(status) ) {
    mresult = adix_exemth( gen,         /* Invoke the method */
	       _CAR(curp),
	       narg, args, status );

    if ( *status == ADI__CALNXTMTH ) {  /* Invoke the next method? */
      curp = _CDR(curp);
      *status = SAI__OK;                /* Should erase result too? */
      }
    else {
      finished = ADI__true;
      rval = mresult;
      }
    }

  if ( ! finished )                     /* Around methods didn't force exit */
    {
    curp = mlists[1];                   /* Now the Before methods */

    while ( _valid_q(curp) && _ok(status) ) {
      mresult = adix_exemth( gen,       /* Invoke the method */
			 _CAR(curp), narg, args, status );

      if ( *status == ADI__CALNXTMTH )  /* Invoke the next method? */
	{
	adic_setecs( ADI__MTHERR,
		     "Illegal use of ADI_CALNXT/adic_calnxt", status );
	}
      else
	curp = _CDR(curp);
      }

    curp = mlists[2];                   /* Now the Primary methods */

    while ( _valid_q(curp) && ! finished && _ok(status) )
      {
      mresult = adix_exemth( gen,       /* Invoke the method */
	       _CAR(curp),
	       narg, args, status );

      if ( *status == ADI__CALNXTMTH )  /* Invoke the next method? */
	{
	*status = SAI__OK;              /* erase result?? */
	curp = _CDR(curp);
	}
      else {
	finished = ADI__true;
	rval = mresult;
	}
      }

    curp = mlists[3];                   /* Now the After methods */

    while ( _valid_q(curp) && _ok(status) ) {
      mresult = adix_exemth( gen,       /* Invoke the method */
	       _CAR(curp),
	       narg, args, status );

      if ( *status == ADI__CALNXTMTH )  /* Invoke the next method? */
	{
	adic_setecs( ADI__MTHERR,
		     "Illegal use of ADI_CALNXT/adic_calnxt", status );
	}
      else
	curp = _CDR(curp);
      }
    }

  return rval;                          /* Set the return value */
  }


ADIobj adix_locgen( ADIobj name, int narg, ADIstatus status )
  {
  ADIobj        curm = ADI_G_firstgnrc;
  ADIlogical    found = ADI__false;
  ADIobj        gnid = ADI__nullid;

  _chk_stat_ret(ADI__nullid);

  while ( ! (_null_q(curm) || found) )
    {
    gnid = _CAR(curm);

    if ( (_gnrc_name(gnid) == name) && (_gnrc_narg(gnid) == narg) )
      found = ADI__true;
    else
      curm = _CDR(curm);
    }

  return found ? gnid : ADI__nullid;
  }


/*  Execute a particular method for a given generic function. It has
 *  already been decided that the method is applicable by this point.
 */
ADIobj adix_exemth( ADIobj generic, ADIobj method,
		    int narg, ADIobj args[], ADIstatus status )
  {
  ADIobj        disp;                   /* Dispatch function */
  KT_CTYPE_mthd *mth = _mthd_data(method);
  ADIobj        res = ADI__nullid;      /* Result of method */

  _chk_stat_ret(ADI__nullid);           /* Check status on entry */

  if ( _eprc_c(mth->exec) ) {           /* Method executor is C? */
    disp = _gnrc_cdisp(generic);

    if ( _valid_q(disp) )               /* Generic has defined C dispatch? */
      res = ((ADIcGenericDispatchCB) _eprc_prc(disp) )( _eprc_prc(mth->exec), narg, args, status );
    else                                /* Use default arg passing */
      res = ((ADIcMethodCB) _eprc_prc(mth->exec) )( narg, args, status );
    }
  else {                                /* Otherwise Fortran */
    disp = _gnrc_fdisp(generic);

    if ( _valid_q(disp) )               /* Generic has defined Fortran dispatch? */
      res = ((ADIfGenericDispatchCB) _eprc_prc(disp) )( &_eprc_prc(mth->exec), &narg, args, status );
    else                                /* Use default arg passing */
      res = ((ADIfMethodCB) _eprc_prc(mth->exec) )( &narg, args, status );
    }

  return res;                           /* Return the result */
  }


ADIobj adix_execi( ADIobj func, int narg,
		   ADIobj args[], ADIstatus status )
  {
  ADIobj        gen;                    /* Generic function id */
  ADIobj        rval = ADI__nullid;     /* The returned value */

  _chk_stat_ret(ADI__nullid);           /* Check status on entry */

  gen = adix_locgen( func, narg,        /* Locate the generic function */
		     status );

  if ( _null_q(gen) ) {                 /* First potential error */
    adic_setec( ADI__NOMTH, status );
    }
  else {                                /* Invoke the method combination */
    ADIobj      mcf = _gnrc_mcomb(gen); /* Locate method combinator */

    (*((ADIcMethodCombinationCB) _eprc_prc(_mco_cexec(mcf))))( gen, narg, args, status );
    }

  return rval;                          /* Set return value */
  }


ADIobj adix_exec( char *func, int flen, int narg,
		  ADIobj args[], ADIstatus status )
  {
  ADIobj        fname;                  /* The function name in the table */

  _chk_stat_ret(ADI__nullid);           /* Check status on entry */

  _GET_NAME(func,flen);               	/* Import name */

  fname = adix_cmn( func, flen,         /* Locate in common string table */
		      status );

  return adix_execi( fname, narg, args, /* and execute the method */
		     status );
  }



void adix_id_flush( char *grp, int glen, ADIstatus status )
  {
  ADIobj	*lvalue;

  adix_pl_find( &ADI_G_grplist, grp, glen, ADI__false,
		&lvalue, NULL, status );

  if ( lvalue && _ok(status) ) {
    }
  else {
    adic_setetc( "GRP", grp, glen );
    adic_setecs( ADI__INVARG, "Invalid identifier group /^GRP/", status );
    }
  }

void adix_id_link( ADIobj id, char *grp, int glen, ADIstatus status )
  {
  ADIobj	*lvalue;

  adix_pl_find( &ADI_G_grplist, grp, glen, ADI__true,
		&lvalue, NULL, status );

  if ( lvalue && _ok(status) ) {
    }
  else {
    adic_setetc( "GRP", grp, glen );
    adic_setecs( ADI__INVARG, "Invalid identifier group /^GRP/", status );
    }
  }

void ADIkrnlAddCommonStrings( ADIcstrTableEntry stable[], ADIstatus status )
  {
  int		i;

  for( i=0; stable[i].string; i++ )
    *(stable[i].saddr) =
      adix_cmnC( stable[i].string, status );
  }
