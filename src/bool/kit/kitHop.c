/**CFile****************************************************************

  FileName    [kitHop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures involving AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitHop.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"
#include "aig/hop/hop.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_GraphToGiaInternal( Gia_Man_t * pMan, Kit_Graph_t * pGraph, int fHash )
{
    Kit_Node_t * pNode = NULL;
    int i, pAnd0, pAnd1;
    // check for constant function
    if ( Kit_GraphIsConst(pGraph) )
        return Abc_LitNotCond( 1, Kit_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Kit_GraphIsVar(pGraph) )
        return Abc_LitNotCond( Kit_GraphVar(pGraph)->iFunc, Kit_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Kit_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Abc_LitNotCond( Kit_GraphNode(pGraph, pNode->eEdge0.Node)->iFunc, pNode->eEdge0.fCompl ); 
        pAnd1 = Abc_LitNotCond( Kit_GraphNode(pGraph, pNode->eEdge1.Node)->iFunc, pNode->eEdge1.fCompl ); 
        if ( fHash )
            pNode->iFunc = Gia_ManHashAnd( pMan, pAnd0, pAnd1 );
        else
            pNode->iFunc = Gia_ManAppendAnd2( pMan, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Abc_LitNotCond( pNode->iFunc, Kit_GraphIsComplement(pGraph) );
}
int Kit_GraphToGia( Gia_Man_t * pMan, Kit_Graph_t * pGraph, Vec_Int_t * vLeaves, int fHash )
{
    Kit_Node_t * pNode = NULL;
    int i;
    // collect the fanins
    Kit_GraphForEachLeaf( pGraph, pNode, i )
        pNode->iFunc = vLeaves ? Vec_IntEntry(vLeaves, i) : Gia_Obj2Lit(pMan, Gia_ManPi(pMan, i));
    // perform strashing
    return Kit_GraphToGiaInternal( pMan, pGraph, fHash );
}
int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash )
{
    int iLit;
    Kit_Graph_t * pGraph;
    // transform truth table into the decomposition tree
    if ( vMemory == NULL )
    {
        vMemory = Vec_IntAlloc( 0 );
        pGraph = Kit_TruthToGraph( pTruth, nVars, vMemory );
        Vec_IntFree( vMemory );
    }
    else
        pGraph = Kit_TruthToGraph( pTruth, nVars, vMemory );
    if ( pGraph == NULL )
    {
        printf( "Kit_TruthToGia(): Converting truth table to AIG has failed for function:\n" );
        Kit_DsdPrintFromTruth( pTruth, nVars ); printf( "\n" );
    }
    // derive the AIG for the decomposition tree
    iLit = Kit_GraphToGia( pMan, pGraph, vLeaves, fHash );
    Kit_GraphFree( pGraph );
    return iLit;
}
int Kit_TruthToGia2( Gia_Man_t * pMan, unsigned * pTruth0, unsigned * pTruth1, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash )
{
    int iLit;
    Kit_Graph_t * pGraph;
    // transform truth table into the decomposition tree
    if ( vMemory == NULL )
    {
        vMemory = Vec_IntAlloc( 0 );
        pGraph = Kit_TruthToGraph2( pTruth0, pTruth1, nVars, vMemory );
        Vec_IntFree( vMemory );
    }
    else
        pGraph = Kit_TruthToGraph2( pTruth0, pTruth1, nVars, vMemory );
    if ( pGraph == NULL )
    {
        printf( "Kit_TruthToGia2(): Converting truth table to AIG has failed for function:\n" );
        Kit_DsdPrintFromTruth( pTruth0, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( pTruth1, nVars ); printf( "\n" );
    }
    // derive the AIG for the decomposition tree
    iLit = Kit_GraphToGia( pMan, pGraph, vLeaves, fHash );
    Kit_GraphFree( pGraph );
    return iLit;
}

/**Function*************************************************************

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_IsopNodeNum( unsigned * pTruth0, unsigned * pTruth1, int nVars, Vec_Int_t * vMemory )
{
    Kit_Graph_t * pGraph;
    int nNodes;
    // transform truth table into the decomposition tree
    if ( vMemory == NULL )
    {
        vMemory = Vec_IntAlloc( 0 );
        pGraph = Kit_TruthToGraph2( pTruth0, pTruth1, nVars, vMemory );
        Vec_IntFree( vMemory );
    }
    else
        pGraph = Kit_TruthToGraph2( pTruth0, pTruth1, nVars, vMemory );
    if ( pGraph == NULL )
    {
        printf( "Kit_TruthToGia2(): Converting truth table to AIG has failed for function:\n" );
        Kit_DsdPrintFromTruth( pTruth0, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( pTruth1, nVars ); printf( "\n" );
    }
    // derive the AIG for the decomposition tree
    nNodes = Kit_GraphNodeNum( pGraph );
    Kit_GraphFree( pGraph );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_IsopResubInt( Kit_Graph_t * pGraph, Vec_Int_t * vRes )
{
    int nVars = Kit_GraphLeaveNum(pGraph);
    assert( nVars >= 0 && nVars <= pGraph->nSize );
    if ( Kit_GraphIsConst(pGraph) )
        Vec_IntPush( vRes, Kit_GraphIsConst1(pGraph) );
    else if ( Kit_GraphIsVar(pGraph) )
        Vec_IntPush( vRes, 4 + Abc_Var2Lit(Kit_GraphVarInt(pGraph), Kit_GraphIsComplement(pGraph)) );
    else
    {
        Kit_Node_t * pNode = NULL; int i;
        Kit_GraphForEachNode( pGraph, pNode, i )
        {
            Kit_Node_t * pFan0 = Kit_GraphNodeFanin0( pGraph, pNode );
            Kit_Node_t * pFan1 = Kit_GraphNodeFanin1( pGraph, pNode );
            int iLit0 = Abc_Var2Lit( Kit_GraphNodeInt(pGraph, pFan0), pNode->eEdge0.fCompl );
            int iLit1 = Abc_Var2Lit( Kit_GraphNodeInt(pGraph, pFan1), pNode->eEdge1.fCompl );
            if ( iLit0 > iLit1 )
                iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1;
            Vec_IntPushTwo( vRes, 4 + iLit0, 4 + iLit1 );
        }
        assert( pNode == Kit_GraphNode(pGraph, pGraph->eRoot.Node) );
        Vec_IntPush( vRes, 4 + Abc_Var2Lit(Kit_GraphNodeInt(pGraph, pNode), Kit_GraphIsComplement(pGraph)) );
    }
}
Vec_Int_t * Kit_IsopResub( unsigned * pTruth0, unsigned * pTruth1, int nVars, Vec_Int_t * vMemory )
{
    Vec_Int_t * vRes = NULL;
    Kit_Graph_t * pGraph;
    int nNodes;
    // transform truth table into the decomposition tree
    if ( vMemory == NULL )
    {
        vMemory = Vec_IntAlloc( 0 );
        pGraph = Kit_TruthToGraph2( pTruth0, pTruth1, nVars, vMemory );
        Vec_IntFree( vMemory );
    }
    else
        pGraph = Kit_TruthToGraph2( pTruth0, pTruth1, nVars, vMemory );
    if ( pGraph == NULL )
    {
        printf( "Kit_TruthToGia2(): Converting truth table to AIG has failed for function:\n" );
        Kit_DsdPrintFromTruth( pTruth0, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( pTruth1, nVars ); printf( "\n" );
    }
    // derive the AIG for the decomposition tree
    nNodes = Kit_GraphNodeNum( pGraph );
    vRes = Vec_IntAlloc( 2*nNodes + 1 );
    Kit_IsopResubInt( pGraph, vRes );
    assert( Vec_IntSize(vRes) == 2*nNodes + 1 );
    Kit_GraphFree( pGraph );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Kit_GraphToHopInternal( Hop_Man_t * pMan, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode = NULL;
    Hop_Obj_t * pAnd0, * pAnd1;
    int i;
    // check for constant function
    if ( Kit_GraphIsConst(pGraph) )
        return Hop_NotCond( Hop_ManConst1(pMan), Kit_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Kit_GraphIsVar(pGraph) )
        return Hop_NotCond( (Hop_Obj_t *)Kit_GraphVar(pGraph)->pFunc, Kit_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Kit_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Hop_NotCond( (Hop_Obj_t *)Kit_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        pAnd1 = Hop_NotCond( (Hop_Obj_t *)Kit_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        pNode->pFunc = Hop_And( pMan, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Hop_NotCond( (Hop_Obj_t *)pNode->pFunc, Kit_GraphIsComplement(pGraph) );
}
Hop_Obj_t * Kit_GraphToHop( Hop_Man_t * pMan, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode = NULL;
    int i;
    // collect the fanins
    Kit_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = Hop_IthVar( pMan, i );
    // perform strashing
    return Kit_GraphToHopInternal( pMan, pGraph );
}
Hop_Obj_t * Kit_TruthToHop( Hop_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory )
{
    Hop_Obj_t * pObj;
    Kit_Graph_t * pGraph;
    // transform truth table into the decomposition tree
    if ( vMemory == NULL )
    {
        vMemory = Vec_IntAlloc( 0 );
        pGraph = Kit_TruthToGraph( pTruth, nVars, vMemory );
        Vec_IntFree( vMemory );
    }
    else
        pGraph = Kit_TruthToGraph( pTruth, nVars, vMemory );
    if ( pGraph == NULL )
    {
        printf( "Kit_TruthToHop(): Converting truth table to AIG has failed for function:\n" );
        Kit_DsdPrintFromTruth( pTruth, nVars ); printf( "\n" );
    }
    // derive the AIG for the decomposition tree
    pObj = Kit_GraphToHop( pMan, pGraph );
    Kit_GraphFree( pGraph );
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Kit_CoverToHop( Hop_Man_t * pMan, Vec_Int_t * vCover, int nVars, Vec_Int_t * vMemory )
{
    Kit_Graph_t * pGraph;
    Hop_Obj_t * pFunc;
    // perform factoring
    Vec_IntClear( vMemory );
    pGraph = Kit_SopFactor( vCover, 0, nVars, vMemory );
    // convert graph to the AIG
    pFunc = Kit_GraphToHop( pMan, pGraph );
    Kit_GraphFree( pGraph );
    return pFunc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

