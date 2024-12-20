/**CFile****************************************************************

  FileName    [hash.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hash map.]

  Synopsis    [External declarations.]

  Author      [Aaron P. Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 16, 2005.]

  Revision    [$Id: vec.h,v 1.00 2005/06/20 00:00:00 ahurst Exp $]

***********************************************************************/
 
#ifndef ABC__misc__hash__hash_h
#define ABC__misc__hash__hash_h


#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif
////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/util/abc_global.h"

#include "hashInt.h"
#include "hashFlt.h"
#include "hashPtr.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

int Hash_DefaultHashFunc(int key, int nBins) {
  return Abc_AbsInt( ( (key+11)*(key)*7+3 ) % nBins );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_END

#endif

