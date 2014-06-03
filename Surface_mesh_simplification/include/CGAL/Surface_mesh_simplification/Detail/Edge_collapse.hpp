// Copyright (c) 2006  GeometryFactory (France). All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Fernando Cacciola <fernando.cacciola@geometryfactory.com>
//
#ifndef CGAL_SURFACE_MESH_SIMPLIFICATION_DETAIL_EDGE_COLLAPSE_H
#define CGAL_SURFACE_MESH_SIMPLIFICATION_DETAIL_EDGE_COLLAPSE_H 1

#include <CGAL/Surface_mesh_simplification/Detail/Common.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_profile.h>
#include <CGAL/boost/graph/Euler_operations.h>

namespace CGAL {

namespace Surface_mesh_simplification
{

//
// Implementation of the vertex-pair collapse triangulated surface mesh simplification algorithm
//
template<class ECM_
        ,class Traits_
        ,class ShouldStop_
        ,class VertexIndexMap_
        ,class EdgeIndexMap_
        ,class EdgeIsBorderMap_
        ,class EdgeIsConstrainedMap_
        ,class GetCost_
        ,class GetPlacement_
        ,class VisitorT_
        >
class EdgeCollapse
{
public:

  typedef ECM_              ECM ;
  typedef Traits_           Traits;
  typedef ShouldStop_       ShouldStop ;
  typedef VertexIndexMap_   VertexIndexMap ;
  typedef EdgeIndexMap_     EdgeIndexMap ;
  typedef EdgeIsBorderMap_  EdgeIsBorderMap ;
  typedef EdgeIsConstrainedMap_ EdgeIsConstrainedMap;
  typedef GetCost_          GetCost ;
  typedef GetPlacement_     GetPlacement ;
  typedef VisitorT_         VisitorT ;
  
  typedef EdgeCollapse Self ;
  
  typedef Edge_profile<ECM> Profile ;
  
  typedef boost::graph_traits  <ECM>       GraphTraits ;
  typedef boost::graph_traits  <ECM const> ConstGraphTraits ;
  
  typedef typename GraphTraits::vertex_descriptor      vertex_descriptor ;
  typedef typename GraphTraits::vertex_iterator        vertex_iterator ;
  typedef typename GraphTraits::halfedge_descriptor    halfedge_descriptor ;
  typedef typename GraphTraits::halfedge_iterator      halfedge_iterator ;
  typedef typename CGAL::Halfedge_around_source_iterator<ECM> out_edge_iterator ;
  typedef typename CGAL::Halfedge_around_target_iterator<ECM> in_edge_iterator ;
  typedef typename GraphTraits::traversal_category     traversal_category ;
  typedef typename GraphTraits::edges_size_type        size_type ;
  
  typedef typename GraphTraits::edge_iterator edge_iterator ;
  typedef typename boost::property_map<ECM, CGAL::vertex_point_t>::type Vertex_point_pmap;
  typedef typename boost::property_traits<Vertex_point_pmap>::value_type Point;

  typedef typename GetCost     ::result_type Cost_type ;
  typedef typename GetPlacement::result_type Placement_type ;
  
  typedef typename Traits::Equal_3 Equal_3 ;
  
  typedef typename Traits::Vector_3 Vector ;
  typedef typename Traits::FT       FT ;

  struct Compare_id
  {
    Compare_id() : mAlgorithm(0) {}
    
    Compare_id( Self const* aAlgorithm ) : mAlgorithm(aAlgorithm) {}
    
    bool operator() ( halfedge_descriptor const& a, halfedge_descriptor const& b ) const 
    {
      return mAlgorithm->get_halfedge_id(a) < mAlgorithm->get_halfedge_id(b);
    }
    
    Self const* mAlgorithm ;
  } ;
  
  struct Compare_cost
  {
    Compare_cost() : mAlgorithm(0) {}
    
    Compare_cost( Self const* aAlgorithm ) : mAlgorithm(aAlgorithm) {}
    
    bool operator() ( halfedge_descriptor const& a, halfedge_descriptor const& b ) const
    { 
      // NOTE: A cost is an optional<> value.
      // Absent optionals are ordered first; that is, "none < T" and "T > none" for any defined T != none.
      // In consequence, edges with undefined costs will be promoted to the top of the priority queue and poped out first.
      return mAlgorithm->get_data(a).cost() < mAlgorithm->get_data(b).cost();
    }
    
    Self const* mAlgorithm ;
  } ;
  
  struct edge_id : boost::put_get_helper<size_type, edge_id>
  {
    typedef boost::readable_property_map_tag category;
    typedef size_type                        value_type;
    typedef size_type                        reference;
    typedef halfedge_descriptor                  key_type;
    
    edge_id() : mAlgorithm(0) {}
    
    edge_id( Self const* aAlgorithm ) : mAlgorithm(aAlgorithm) {}
    
    size_type operator[] ( halfedge_descriptor const& e ) const { return mAlgorithm->get_edge_id(e); }
    
    Self const* mAlgorithm ;
  } ;
  
  
  typedef Modifiable_priority_queue<halfedge_descriptor,Compare_cost,edge_id> PQ ;
  typedef typename PQ::handle pq_handle ;
  
  // An Edge_data is associated with EVERY _ edge in the mesh (collapsable or not).
  // It relates the edge with the PQ-handle needed to update the priority queue
  // It also relates the edge with a policy-based cache
  class Edge_data
  {
  public :
  
    Edge_data() : mPQHandle() {}
    
    Cost_type const& cost() const { return mCost ; }
    Cost_type      & cost()       { return mCost ; }
    
    pq_handle PQ_handle() const { return mPQHandle ;}
    
    bool is_in_PQ() const { return mPQHandle != PQ::null_handle() ; }
    
    void set_PQ_handle( pq_handle h ) { mPQHandle = h ; }
    
    void reset_PQ_handle() { mPQHandle = PQ::null_handle() ; }
    
  private:  
    
    Cost_type mCost ;
    pq_handle mPQHandle ;
  } ;
  typedef Edge_data* Edge_data_ptr ;
  typedef boost::scoped_array<Edge_data> Edge_data_array ;
  
  
public:

  EdgeCollapse( ECM&                        aSurface
              , ShouldStop           const& aShouldStop
              , Traits               const& aGeomTraits
              , VertexIndexMap       const& aVertex_index_map
              , EdgeIndexMap         const& aEdge_index_map
              , EdgeIsBorderMap      const& aEdge_is_border_map
              , EdgeIsConstrainedMap const& aEdge_is_constrained_map
              , GetCost              const& aGetCost
              , GetPlacement         const& aGetPlacement
              , VisitorT                    aVisitor
              ) ;

  int run() ;
  
private:
  
  void Collect();
  void Loop();
  bool Is_collapse_topologically_valid( Profile const& aProfile ) ;
  bool Is_tetrahedron( halfedge_descriptor const& h1 ) ;
  bool Is_open_triangle( halfedge_descriptor const& h1 ) ;
  bool Is_collapse_geometrically_valid( Profile const& aProfile, Placement_type aPlacement ) ;
  void Collapse( Profile const& aProfile, Placement_type aPlacement ) ;
  void Update_neighbors( vertex_descriptor const& aKeptV ) ;
  
  Profile create_profile ( halfedge_descriptor const& aEdge )
  { 
    return Profile(aEdge,mSurface,Vertex_index_map,Edge_index_map,Edge_is_border_map, m_has_border);
  }  
  
  size_type get_halfedge_id   ( halfedge_descriptor const& aEdge ) const { return Edge_index_map[aEdge]; }
  size_type get_edge_id ( halfedge_descriptor const& aEdge ) const { return get_halfedge_id(aEdge) / 2 ; }

  bool is_primary_edge ( halfedge_descriptor const& aEdge ) const { return ( get_halfedge_id(aEdge) % 2 ) == 0 ; }
  
  halfedge_descriptor primary_edge ( halfedge_descriptor const& aEdge ) 
  { 
    return is_primary_edge(aEdge) ? aEdge : opposite(aEdge,mSurface) ;
  }  
    
  bool is_border ( halfedge_descriptor const& aEdge ) const { return get(CGAL::halfedge_is_border, mSurface, aEdge) ; }    
  
  bool is_constrained( halfedge_descriptor const& aEdge ) const { return get(Edge_is_constrained_map,aEdge); }
  bool is_constrained( vertex_descriptor const& aVertex ) const;

  bool is_border_or_constrained( halfedge_descriptor const& aEdge ) const { return Edge_is_border_map[aEdge] ||
                                                                                     Edge_is_constrained_map[aEdge];}

  bool is_edge_a_border ( halfedge_descriptor const& aEdge ) const
  {
    return is_border(aEdge) || is_border(opposite(aEdge,mSurface)) ;
  }    
  
  bool is_border ( vertex_descriptor const& aV ) const ;
  
  bool is_border_or_constrained ( vertex_descriptor const& aV ) const ;

  bool are_shared_triangles_valid( Point const& p0, Point const& p1, Point const& p2, Point const& p3 ) const ;
  
  halfedge_descriptor find_connection ( vertex_descriptor const& v0, vertex_descriptor const& v1 ) const ;
  
  vertex_descriptor find_exterior_link_triangle_3rd_vertex ( halfedge_descriptor const& e, vertex_descriptor const& v0, vertex_descriptor const& v1 ) const ;
  
  Edge_data& get_data ( halfedge_descriptor const& aEdge ) const 
  { 
    CGAL_assertion( is_primary_edge(aEdge) ) ;
    return mEdgeDataArray[get_edge_id(aEdge)];
  }
  
  Point 
  const& 
  get_point ( vertex_descriptor const& aV ) const
  {
    return get(CGAL::vertex_point,mSurface,aV);
  }
  
  boost::tuple<vertex_descriptor,vertex_descriptor> get_vertices( halfedge_descriptor const& aEdge ) const
  {
    vertex_descriptor p,q ;
    p = boost::source(aEdge,mSurface);
    q = boost::target(aEdge,mSurface);
    return boost::make_tuple(p,q);
  }
  
  boost::tuple<vertex_descriptor,vertex_descriptor> get_vertices( halfedge_descriptor const& aEdge ) 
  {
    vertex_descriptor p,q ;
    p = boost::source(aEdge,mSurface);
    q = boost::target(aEdge,mSurface);
    return boost::make_tuple(p,q);
  }
  
  std::string vertex_to_string( vertex_descriptor const& v ) const
  {
    Point const& p = get_point(v);
    return boost::str( boost::format("[V%1%:%2%]") % get(Vertex_index_map,v) % xyz_to_string(p) ) ;
  }
    
  std::string edge_to_string ( halfedge_descriptor const& aEdge ) const
  {
    vertex_descriptor p,q ; boost::tie(p,q) = get_vertices(aEdge);
    return boost::str( boost::format("{E%1% %2%->%3%}%4%") % get(Edge_index_map,aEdge) % vertex_to_string(p) % vertex_to_string(q) % ( is_border(aEdge) ? " (BORDER)" : ( is_border(aEdge->opposite()) ? " (~BORDER)": "" ) ) ) ;
  }
  
  Cost_type get_cost ( Profile const& aProfile ) const
  {
    return Get_cost(aProfile, get_placement(aProfile) );
  }
  
  Placement_type get_placement( Profile const& aProfile ) const
  {
    return Get_placement(aProfile);
  }
  
  void insert_in_PQ( halfedge_descriptor const& aEdge, Edge_data& aData ) 
  {
    CGAL_SURF_SIMPL_TEST_assertion(is_primary_edge(aEdge)) ;
    CGAL_SURF_SIMPL_TEST_assertion(!aData.is_in_PQ());
    CGAL_SURF_SIMPL_TEST_assertion(!mPQ->contains(aEdge) ) ;

    aData.set_PQ_handle(mPQ->push(aEdge));

    CGAL_SURF_SIMPL_TEST_assertion(aData.is_in_PQ());
    CGAL_SURF_SIMPL_TEST_assertion(mPQ->contains(aEdge) ) ;
  }
  
  void update_in_PQ( halfedge_descriptor const& aEdge, Edge_data& aData )
  {
    CGAL_SURF_SIMPL_TEST_assertion(is_primary_edge(aEdge)) ;
    CGAL_SURF_SIMPL_TEST_assertion(aData.is_in_PQ());
    CGAL_SURF_SIMPL_TEST_assertion(mPQ->contains(aEdge) ) ;

    aData.set_PQ_handle(mPQ->update(aEdge,aData.PQ_handle())) ; 

    CGAL_SURF_SIMPL_TEST_assertion(aData.is_in_PQ());
    CGAL_SURF_SIMPL_TEST_assertion(mPQ->contains(aEdge) ) ;
  }   
  
  void remove_from_PQ( halfedge_descriptor const& aEdge, Edge_data& aData )
  {
    CGAL_SURF_SIMPL_TEST_assertion(is_primary_edge(aEdge)) ;
    CGAL_SURF_SIMPL_TEST_assertion(aData.is_in_PQ());
    CGAL_SURF_SIMPL_TEST_assertion(mPQ->contains(aEdge) ) ;

    aData.set_PQ_handle(mPQ->erase(aEdge,aData.PQ_handle()));

    CGAL_SURF_SIMPL_TEST_assertion(!aData.is_in_PQ());
    CGAL_SURF_SIMPL_TEST_assertion(!mPQ->contains(aEdge) ) ;
  }   
  
  optional<halfedge_descriptor> pop_from_PQ() 
  {
    optional<halfedge_descriptor> rEdge = mPQ->extract_top();
    if ( rEdge )
    {
      CGAL_SURF_SIMPL_TEST_assertion(is_primary_edge(*rEdge) ) ;
      CGAL_SURF_SIMPL_TEST_assertion(get_data(*rEdge).is_in_PQ()) ;

      get_data(*rEdge).reset_PQ_handle();

      CGAL_SURF_SIMPL_TEST_assertion(!get_data(*rEdge).is_in_PQ() ) ;
      CGAL_SURF_SIMPL_TEST_assertion(!mPQ->contains(*rEdge)) ;
    }  
    return rEdge ;  
  }

  /// Functions to ensure the backward compatibility before addition of the constrained edge map
  template<class AEdgeIsConstrainedMap>
  vertex_descriptor
  halfedge_collapse_bk_compatibility(
    halfedge_descriptor const& pq, AEdgeIsConstrainedMap aEdge_is_constrained_map)
  {
    return CGAL::Euler::collapse_edge(edge(pq,mSurface), mSurface, aEdge_is_constrained_map);
  }


  template<class ECM>
  vertex_descriptor
  halfedge_collapse_bk_compatibility(
    halfedge_descriptor const& pq, No_constrained_edge_map<ECM> )
  {
    //std::cerr << "call CGAL::Euler::collapse_edge()"<< std::endl;
    vertex_descriptor vd = CGAL::Euler::collapse_edge(edge(pq,mSurface), mSurface);
    //std::cerr << "after CGAL::Euler::collapse_edge()"<< std::endl;
    return vd;
  }

  
  /// We wrap this test to avoid penalizing runtime when no constraints are present
  template<class AEdgeIsConstrainedMap>
  bool
  is_edge_adjacent_to_a_constrained_edge(
    Profile const& aProfile, AEdgeIsConstrainedMap)
  {
    return is_constrained(aProfile.v0()) && is_constrained(aProfile.v1());
  }

  template<class ECM>
  bool
  is_edge_adjacent_to_a_constrained_edge(
    halfedge_descriptor const&, No_constrained_edge_map<ECM> )
  {
    return false;
  }
  ///

private:

  ECM&                   mSurface ;
  
  ShouldStop           const& Should_stop ;
  VertexIndexMap       const& Vertex_index_map ;
  EdgeIndexMap         const& Edge_index_map ;
  EdgeIsBorderMap      const& Edge_is_border_map;
  EdgeIsConstrainedMap const& Edge_is_constrained_map;
  GetCost              const& Get_cost ;
  GetPlacement         const& Get_placement ;
  VisitorT                    Visitor ;
  bool                        m_has_border ;
  
private:

  Edge_data_array mEdgeDataArray ;
  
  boost::scoped_ptr<PQ> mPQ ;
    
  std::size_t mInitialEdgeCount ;
  std::size_t mCurrentEdgeCount ; 

  FT          mcMaxDihedralAngleCos2 ;
  
  CGAL_ECMS_DEBUG_CODE ( unsigned mStep ; )
} ;

} // namespace Surface_mesh_simplification

} //namespace CGAL

#include <CGAL/Surface_mesh_simplification/Detail/Edge_collapse_impl.h>

#endif // CGAL_SURFACE_MESH_SIMPLIFICATION_I_EDGE_COLLAPSE_H //
// EOF //
 
