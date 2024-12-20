/**CFile****************************************************************

  FileName    [satMem.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Memory management.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satMem.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "satMem.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sat_MmFixed_t_
{
    // information about individual entries
    int           nEntrySize;    // the size of one entry
    int           nEntriesAlloc; // the total number of entries allocated
    int           nEntriesUsed;  // the number of entries in use
    int           nEntriesMax;   // the max number of entries in use
    char *        pEntriesFree;  // the linked list of free entries

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks 
    int           nChunks;       // the current number of memory chunks 
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};

struct Sat_MmFlex_t_
{
    // information about individual entries
    int           nEntriesUsed;  // the number of entries allocated
    char *        pCurrent;      // the current pointer to free memory
    char *        pEnd;          // the first entry outside the free memory

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks 
    int           nChunks;       // the current number of memory chunks 
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};

struct Sat_MmStep_t_
{
    int               nMems;     // the number of fixed memory managers employed
    Sat_MmFixed_t **  pMems;     // memory managers: 2^1 words, 2^2 words, etc
    int               nMapSize;  // the size of the memory array
    Sat_MmFixed_t **  pMap;      // maps the number of bytes into its memory manager
    // additional memory chunks
    int           nChunksAlloc;  // the maximum number of memory chunks 
    int           nChunks;       // the current number of memory chunks 
    char **       pChunks;       // the allocated memory
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates memory pieces of fixed size.]

  Description [The size of the chunk is computed as the minimum of
  1024 entries and 64K. Can only work with entry size at least 4 byte long.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_MmFixed_t * Sat_MmFixedStart( int nEntrySize )
{
    Sat_MmFixed_t * p;

    p = ABC_ALLOC( Sat_MmFixed_t, 1 );
    memset( p, 0, sizeof(Sat_MmFixed_t) );

    p->nEntrySize    = nEntrySize;
    p->nEntriesAlloc = 0;
    p->nEntriesUsed  = 0;
    p->pEntriesFree  = NULL;

    if ( nEntrySize * (1 << 10) < (1<<16) )
        p->nChunkSize = (1 << 10);
    else
        p->nChunkSize = (1<<16) / nEntrySize;
    if ( p->nChunkSize < 8 )
        p->nChunkSize = 8;

    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );

    p->nMemoryUsed   = 0;
    p->nMemoryAlloc  = 0;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmFixedStop( Sat_MmFixed_t * p, int fVerbose )
{
    int i;
    if ( p == NULL )
        return;
    if ( fVerbose )
    {
        printf( "Fixed memory manager: Entry = %5d. Chunk = %5d. Chunks used = %5d.\n",
            p->nEntrySize, p->nChunkSize, p->nChunks );
        printf( "   Entries used = %8d. Entries peak = %8d. Memory used = %8d. Memory alloc = %8d.\n",
            p->nEntriesUsed, p->nEntriesMax, p->nEntrySize * p->nEntriesUsed, p->nMemoryAlloc );
    }
    for ( i = 0; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    ABC_FREE( p->pChunks );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sat_MmFixedEntryFetch( Sat_MmFixed_t * p )
{
    char * pTemp;
    int i;

    // check if there are still free entries
    if ( p->nEntriesUsed == p->nEntriesAlloc )
    { // need to allocate more entries
        assert( p->pEntriesFree == NULL );
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc ); 
        }
        p->pEntriesFree = ABC_ALLOC( char, p->nEntrySize * p->nChunkSize );
        p->nMemoryAlloc += p->nEntrySize * p->nChunkSize;
        // transform these entries into a linked list
        pTemp = p->pEntriesFree;
        for ( i = 1; i < p->nChunkSize; i++ )
        {
            *((char **)pTemp) = pTemp + p->nEntrySize;
            pTemp += p->nEntrySize;
        }
        // set the last link
        *((char **)pTemp) = NULL;
        // add the chunk to the chunk storage
        p->pChunks[ p->nChunks++ ] = p->pEntriesFree;
        // add to the number of entries allocated
        p->nEntriesAlloc += p->nChunkSize;
    }
    // incrememt the counter of used entries
    p->nEntriesUsed++;
    if ( p->nEntriesMax < p->nEntriesUsed )
        p->nEntriesMax = p->nEntriesUsed;
    // return the first entry in the free entry list
    pTemp = p->pEntriesFree;
    p->pEntriesFree = *((char **)pTemp);
    return pTemp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmFixedEntryRecycle( Sat_MmFixed_t * p, char * pEntry )
{
    // decrement the counter of used entries
    p->nEntriesUsed--;
    // add the entry to the linked list of free entries
    *((char **)pEntry) = p->pEntriesFree;
    p->pEntriesFree = pEntry;
}

/**Function*************************************************************

  Synopsis    []

  Description [Relocates all the memory except the first chunk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmFixedRestart( Sat_MmFixed_t * p )
{
    int i;
    char * pTemp;
    if ( p->nChunks == 0 )
        return;
    assert( p->nChunks > 0 );

    // deallocate all chunks except the first one
    for ( i = 1; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    p->nChunks = 1;
    // transform these entries into a linked list
    pTemp = p->pChunks[0];
    for ( i = 1; i < p->nChunkSize; i++ )
    {
        *((char **)pTemp) = pTemp + p->nEntrySize;
        pTemp += p->nEntrySize;
    }
    // set the last link
    *((char **)pTemp) = NULL;
    // set the free entry list
    p->pEntriesFree  = p->pChunks[0];
    // set the correct statistics
    p->nMemoryAlloc  = p->nEntrySize * p->nChunkSize;
    p->nMemoryUsed   = 0;
    p->nEntriesAlloc = p->nChunkSize;
    p->nEntriesUsed  = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_MmFixedReadMemUsage( Sat_MmFixed_t * p )
{
    return p->nMemoryAlloc;
}



/**Function*************************************************************

  Synopsis    [Allocates entries of flexible size.]

  Description [Can only work with entry size at least 4 byte long.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_MmFlex_t * Sat_MmFlexStart()
{
    Sat_MmFlex_t * p;

    p = ABC_ALLOC( Sat_MmFlex_t, 1 );
    memset( p, 0, sizeof(Sat_MmFlex_t) );

    p->nEntriesUsed  = 0;
    p->pCurrent      = NULL;
    p->pEnd          = NULL;

    p->nChunkSize    = (1 << 16);
    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );

    p->nMemoryUsed   = 0;
    p->nMemoryAlloc  = 0;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmFlexStop( Sat_MmFlex_t * p, int fVerbose )
{
    int i;
    if ( p == NULL )
        return;
    if ( fVerbose )
    {
        printf( "Flexible memory manager: Chunk size = %d. Chunks used = %d.\n",
            p->nChunkSize, p->nChunks );
        printf( "   Entries used = %d. Memory used = %d. Memory alloc = %d.\n",
            p->nEntriesUsed, p->nMemoryUsed, p->nMemoryAlloc );
    }
    for ( i = 0; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    ABC_FREE( p->pChunks );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sat_MmFlexEntryFetch( Sat_MmFlex_t * p, int nBytes )
{
    char * pTemp;
    // check if there are still free entries
    if ( p->pCurrent == NULL || p->pCurrent + nBytes > p->pEnd )
    { // need to allocate more entries
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc ); 
        }
        if ( nBytes > p->nChunkSize )
        {
            // resize the chunk size if more memory is requested than it can give
            // (ideally, this should never happen)
            p->nChunkSize = 2 * nBytes;
        }
        p->pCurrent = ABC_ALLOC( char, p->nChunkSize );
        p->pEnd     = p->pCurrent + p->nChunkSize;
        p->nMemoryAlloc += p->nChunkSize;
        // add the chunk to the chunk storage
        p->pChunks[ p->nChunks++ ] = p->pCurrent;
    }
    assert( p->pCurrent + nBytes <= p->pEnd );
    // increment the counter of used entries
    p->nEntriesUsed++;
    // keep track of the memory used
    p->nMemoryUsed += nBytes;
    // return the next entry
    pTemp = p->pCurrent;
    p->pCurrent += nBytes;
    return pTemp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_MmFlexReadMemUsage( Sat_MmFlex_t * p )
{
    return p->nMemoryAlloc;
}





/**Function*************************************************************

  Synopsis    [Starts the hierarchical memory manager.]

  Description [This manager can allocate entries of any size.
  Iternally they are mapped into the entries with the number of bytes
  equal to the power of 2. The smallest entry size is 8 bytes. The
  next one is 16 bytes etc. So, if the user requests 6 bytes, he gets 
  8 byte entry. If we asks for 25 bytes, he gets 32 byte entry etc.
  The input parameters "nSteps" says how many fixed memory managers
  are employed internally. Calling this procedure with nSteps equal
  to 10 results in 10 hierarchically arranged internal memory managers, 
  which can allocate up to 4096 (1Kb) entries. Requests for larger 
  entries are handed over to malloc() and then ABC_FREE()ed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_MmStep_t * Sat_MmStepStart( int nSteps )
{
    Sat_MmStep_t * p;
    int i, k;
    p = ABC_ALLOC( Sat_MmStep_t, 1 );
    p->nMems = nSteps;
    // start the fixed memory managers
    p->pMems = ABC_ALLOC( Sat_MmFixed_t *, p->nMems );
    for ( i = 0; i < p->nMems; i++ )
        p->pMems[i] = Sat_MmFixedStart( (8<<i) );
    // set up the mapping of the required memory size into the corresponding manager
    p->nMapSize = (4<<p->nMems);
    p->pMap = ABC_ALLOC( Sat_MmFixed_t *, p->nMapSize+1 );
    p->pMap[0] = NULL;
    for ( k = 1; k <= 4; k++ )
        p->pMap[k] = p->pMems[0];
    for ( i = 0; i < p->nMems; i++ )
        for ( k = (4<<i)+1; k <= (8<<i); k++ )
            p->pMap[k] = p->pMems[i];
//for ( i = 1; i < 100; i ++ )
//printf( "%10d: size = %10d\n", i, p->pMap[i]->nEntrySize );
    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmStepStop( Sat_MmStep_t * p, int fVerbose )
{
    int i;
    if ( p->nChunksAlloc )
    {
        for ( i = 0; i < p->nChunks; i++ )
            ABC_FREE( p->pChunks[i] );
        ABC_FREE( p->pChunks );
    }
    for ( i = 0; i < p->nMems; i++ )
        Sat_MmFixedStop( p->pMems[i], fVerbose );
    ABC_FREE( p->pMems );
    ABC_FREE( p->pMap );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Stops the memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmStepRestart( Sat_MmStep_t * p )
{
    int i;
    if ( p->nChunksAlloc )
    {
        for ( i = 0; i < p->nChunks; i++ )
            ABC_FREE( p->pChunks[i] );
        p->nChunks = 0;
    }
    for ( i = 0; i < p->nMems; i++ )
        Sat_MmFixedRestart( p->pMems[i] );
}

/**Function*************************************************************

  Synopsis    [Creates the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sat_MmStepEntryFetch( Sat_MmStep_t * p, int nBytes )
{
    if ( nBytes == 0 )
        return NULL;
    if ( nBytes > p->nMapSize )
    {
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc ); 
        }
        p->pChunks[ p->nChunks++ ] = ABC_ALLOC( char, nBytes );
        return p->pChunks[p->nChunks-1];
    }
    return Sat_MmFixedEntryFetch( p->pMap[nBytes] );
}


/**Function*************************************************************

  Synopsis    [Recycles the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_MmStepEntryRecycle( Sat_MmStep_t * p, char * pEntry, int nBytes )
{
    if ( nBytes == 0 )
        return;
    if ( nBytes > p->nMapSize )
    {
//        ABC_FREE( pEntry );
        return;
    }
    Sat_MmFixedEntryRecycle( p->pMap[nBytes], pEntry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_MmStepReadMemUsage( Sat_MmStep_t * p )
{
    int i, nMemTotal = 0;
    for ( i = 0; i < p->nMems; i++ )
        nMemTotal += p->pMems[i]->nMemoryAlloc;
    return nMemTotal;
}
ABC_NAMESPACE_IMPL_END

