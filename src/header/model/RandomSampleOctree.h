#ifndef RANDOM_SAMPLE__OCTREE_H
#define RANDOM_SAMPLE__OCTREE_H

#include "Stream.h"

#include <cassert>
#include <map>
#include <ctime>
#include <time.h>

#include "MortonCode.h"
#include "OctreeNode.h"
#include "OctreeMapTypes.h"
#include "TransientRenderingState.h"
#include "OctreeStats.h"
#include "PlyPointReader.h"
#include "PointAppender.h"
#include "OctreeParameterTypes.h"

using namespace std;
using namespace util;

namespace model
{
	/** Octree which inner nodes have points randomly sampled from child nodes. Provides a smooth transition
	 * between level of detail (LOD), but has the cost of more points being rendered per frame.
	 * @param OctreeParamaters is the struct defining the morton code, point, node and hierarchy types used by the
	 * octree. */
	template< typename OctreeParams >
	class RandomSampleOctree
	{
	public:
		using MortonCode = typename OctreeParams::Morton;
		using MortonCodePtr = shared_ptr< MortonCode >;
		
		using Point = typename OctreeParams::Point;
		using PointPtr = shared_ptr< Point >;
		using PointVector = vector< PointPtr, ManagedAllocator< PointPtr > >;
		using PointVectorPtr = shared_ptr< PointVector >;
		
		using OctreeNode = typename OctreeParams::Node;
		using OctreeNodePtr = shared_ptr< OctreeNode >;
		
		using OctreeMap = typename OctreeParams::Hierarchy;
		using OctreeMapPtr = shared_ptr< OctreeMap >;
		
		using PlyPointReader = util::PlyPointReader< Point >;
		using Precision = typename PlyPointReader::Precision;
		using RandomPointAppender = model::RandomPointAppender< MortonCode, Point >;
		
		/** Initialize data for building the octree, giving the desired max number of nodes per node and the maximum
		 * level of the hierarchy. */
		RandomSampleOctree( const int& maxPointsPerNode, const int& maxLevel );
		
		~RandomSampleOctree() {}
		
		/** Builds the octree for a given point cloud file. The points are expected to be in world coordinates.
		 * @param precision tells the floating precision in which coordinates will be interpreted.
		 * @param attribs tells which point attributes will be loaded from the file. */
		virtual void buildFromFile( const string& plyFileName, const Precision& precision );
		
		/** Builds the octree for a given point cloud. The points are expected to be in world coordinates. In any moment
		 * of the building, points can be cleared in order to save memory. */
		virtual void build( PointVector& points );
		
		/** Traverses the octree, rendering all necessary points.
		 * @returns the number of rendered points. */
		virtual OctreeStats traverse( RenderingState& renderingState, const Float& projThresh );
		
		/** Computes the boundaries of the node indicated by the given morton code.
		 * @returns a pair of vertices: the first is the box's minimum vertex an the last is the box's maximum
		 * vertex. */
		AlignedBox3f getBoundaries( MortonCodePtr ) const;
		
		virtual OctreeMapPtr getHierarchy() const;
		
		/** Gets the origin, which is the point contained in octree with minimun coordinates for all axis. */
		virtual shared_ptr< Vec3 > getOrigin() const;
		
		/** Gets the size of the octree, which is the extents in each axis from origin representing the space that the
		 * octree occupies. */
		virtual shared_ptr< Vec3 > getSize() const;
		
		/** Gets the size of the leaf nodes. */
		virtual shared_ptr< Vec3 > getLeafSize() const;
		
		/** Gets the maximum number of points that can be inside an octree node. */
		virtual unsigned int getMaxPointsPerNode() const;
		
		/** Gets the maximum level that this octree can reach. */
		virtual unsigned int getMaxLevel() const;
		
		/** Calculates the MortonCode of a Point. */
		MortonCode calcMorton( const Point& point ) const;
		
		template< typename P >
		friend ostream& operator<<( ostream& out, const RandomSampleOctree< P >& octree );
		
	protected:
		/** Calculates octree's boundaries. */
		virtual void buildBoundaries( const PointVector& points );
		
		/** Creates all nodes bottom-up. In any moment of the building, points can be cleared in order to save memory.*/
		virtual void buildNodes( PointVector& points );
		
		/** Creates all leaf nodes and put points inside them. */
		virtual void buildLeaves( const PointVector& points );
		
		/** Inserts a point into the octree leaf it belongs. Creates the node in the process if necessary. */
		virtual void insertPointInLeaf( const PointPtr& point );
		
		/** Creates all inner nodes, with LOD. Bottom-up. If a node has only leaf chilren and the accumulated number of
		 * children points is less than a threshold, the children are merged into parent. */
		virtual void buildInners();
		
		/** Builds the inner node given all child nodes, inserting it into the hierarchy. */
		virtual void buildInnerNode( typename OctreeMap::iterator& firstChildIt,
									 const typename OctreeMap::iterator& currentChildIt,
									 const MortonCodePtr& parentCode, const vector< OctreeNodePtr >& children );
		
		/** Erase a range of nodes, represented by iterator for first (inclusive) and last (not inclusive). */
		virtual void eraseNodes( const typename OctreeMap::iterator& first, const typename OctreeMap::iterator& last );
		
		/** Traversal recursion. */
		virtual void traverse( MortonCodePtr nodeCode, RenderingState& renderingState, const Float& projThresh );
		
		virtual void setupNodeRendering( OctreeNodePtr node, RenderingState& renderingState );
		
		/** Setups the rendering of an inner node, putting all necessary points into the rendering list. */
		virtual void setupInnerNodeRendering( OctreeNodePtr innerNode, MortonCodePtr code, RenderingState& renderingState );
		
		/** Setups the rendering of an leaf node, putting all necessary points into the rendering list. */
		virtual void setupLeafNodeRendering( OctreeNodePtr innerNode, MortonCodePtr code, RenderingState& renderingState );
		
		/** Handler called whenever a node is culled on traversal. Default implementation does nothing. */
		virtual void handleCulledNode( const MortonCodePtr code ) {};
		
		/** Event called on traversal ending, before rendering. Default implementation does nothing. */
		virtual void onTraversalEnd() {};
		
		/** The hierarchy itself. */
		OctreeMapPtr m_hierarchy;
		
		/** Octree origin, which is the point contained in octree with minimum coordinates for all axis. */
		shared_ptr< Vec3 > m_origin;
		
		/** Spatial size of the octree. */
		shared_ptr< Vec3 > m_size;
		
		/** Spatial size of the leaf nodes. */
		shared_ptr< Vec3 > m_leafSize;
		
		/** Maximum number of points per node. */
		unsigned int m_maxPointsPerNode;
		
		/** Maximum level of this octree. */
		unsigned int m_maxLevel;
		
		/** Maximum level that the type used as morton code of this octree can represent. */
		unsigned int m_maxMortonLevel;
		
	private:
		void setMaxLvl( const int& maxLevel, const ShallowMortonCode& );
		void setMaxLvl( const int& maxLevel, const MediumMortonCode& );
		
		/** Builds the inner node given all child points. */
		OctreeNodePtr buildInnerNode( const PointVector& childrenPoints ) const;
	};
	
	template< typename OctreeParams >
	RandomSampleOctree< OctreeParams >
	::RandomSampleOctree( const int& maxPointsPerNode, const int& maxLevel )
	: m_size( new Vec3() ),
	m_leafSize( new Vec3() ),
	m_origin( new Vec3() ),
	m_maxPointsPerNode( maxPointsPerNode ),
	m_hierarchy( new OctreeMap() )
	{
		setMaxLvl( maxLevel, MortonCode() );
		srand( 1 );
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::build( PointVector& points )
	{
		buildBoundaries( points );
		buildNodes( points );
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >
	::buildFromFile( const string& plyFileName, const Precision& precision )
	{
		PointVector points;
		PlyPointReader *reader = new PlyPointReader( plyFileName );
		reader->read(
			[ & ]( const Point& point )
			{
				points.push_back( makeManaged< Point >( point ) );
			}, precision
		);
		
		cout << "After reading points" << endl << endl;
		
		//
		/*cout << "Read points" << endl << endl;
		for( int i = 0; i < 10; ++i )
		{
			cout << *points[ i ] << endl << endl;
		}*/
		//
		
		//cout << "Read points: " << endl << points << endl; 
		
		// From now on the reader is not necessary. Delete it in order to save memory.
		delete reader;
		
		build( points );
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::buildBoundaries( const PointVector& points )
	{
		Float negInf = -numeric_limits< Float >::max();
		Float posInf = numeric_limits< Float >::max();
		Vec3 minCoords( posInf, posInf, posInf );
		Vec3 maxCoords( negInf, negInf, negInf );
		
		for( PointPtr point : points )
		{
			const Vec3& pos = point->getPos();
			
			for( int i = 0; i < 3; ++i )
			{
				minCoords[ i ] = std::min( minCoords[ i ], pos[ i ] );
				maxCoords[ i ] = std::max( maxCoords[ i ], pos[ i ] );
			}
		}
		
		*m_origin = minCoords;
		*m_size = maxCoords - minCoords;
		*m_leafSize = *m_size * ( ( Float )1 / ( ( unsigned long long )1 << m_maxLevel ) );
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::buildNodes( PointVector& points )
	{
		cout << "Before leaf nodes build." << endl << endl;
		buildLeaves(points);
		cout << "After leaf nodes build." << endl << endl;
		
		// From now on the point vector is not necessary. Clear it to save memory.
		points.clear();
		
		buildInners();
		cout << "After inner nodes build." << endl << endl;
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::buildLeaves( const PointVector& points )
	{
		for( PointPtr point : points )
		{
			insertPointInLeaf( point );
		}
	}
	
	template< typename OctreeParams >
	inline typename OctreeParams::Morton RandomSampleOctree< OctreeParams >
	::calcMorton( const Point& point ) const
	{
		const Vec3& pos = point.getPos();
		Vec3 index = ( pos - ( *m_origin ) ).array() / m_leafSize->array();
		MortonCode code;
		code.build( index( 0 ), index( 1 ), index( 2 ), m_maxLevel );
		
		return code;
	}
	
	template< typename OctreeParams >
	inline void RandomSampleOctree< OctreeParams >::insertPointInLeaf( const PointPtr& point )
	{
		MortonCodePtr code = makeManaged< MortonCode >( calcMorton( *point ) );
		typename OctreeMap::iterator genericLeafIt = m_hierarchy->find( code );
		
		if( genericLeafIt == m_hierarchy->end() )
		{
			// Creates leaf node.
			OctreeNodePtr leafNode = makeManaged< OctreeNode >( true );
			
			PointVector points;
			points.push_back( point );
			leafNode->setContents( points );
			( *m_hierarchy )[ code ] = leafNode;
		}
		else
		{
			// Node already exists. Appends the point there.
			OctreeNodePtr leafNode = genericLeafIt->second;
			PointVector& nodePoints = leafNode->getContents();
			nodePoints.push_back( point );
		}
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::buildInners()
	{
		// Do a bottom-up per-level construction of inner nodes.
		for( int level = m_maxLevel - 1; level > -1; --level )
		{
			cout << "========== Octree construction, level " << level << " ==========" << endl << endl;
			
			// The idea behind this boundary is to get the minimum morton code that is from lower levels than
			// the current. This is the same of the morton code filled with just one 1 bit from the level immediately
			// below the current one. 
			unsigned long long mortonLvlBoundary = ( unsigned long long )( 1 ) << ( 3 * ( level + 1 ) + 1 );
			
			//cout << "Morton lvl boundary: 0x" << hex << mortonLvlBoundary << dec << endl;
			
			typename OctreeMap::iterator firstChildIt = m_hierarchy->begin(); 
			
			// Loops per siblings in a level.
			while( firstChildIt != m_hierarchy->end() && firstChildIt->first->getBits() < mortonLvlBoundary )
			{
				MortonCodePtr parentCode = firstChildIt->first->traverseUp();
				
				auto children = vector< OctreeNodePtr >();
				
				// Puts children into vector.
				children.push_back( firstChildIt->second );
				
				typename OctreeMap::iterator currentChildIt = firstChildIt;
				while( ( ++currentChildIt ) != m_hierarchy->end() && *currentChildIt->first->traverseUp() == *parentCode )
				{
					OctreeNodePtr currentChild = currentChildIt->second;
					children.push_back( currentChild );
				}
				
				buildInnerNode( firstChildIt, currentChildIt, parentCode, children );
			}
			
			cout << "========== End of level " << level << " ==========" << endl << endl;
		}
	}

	template< typename OctreeParams >
	inline void RandomSampleOctree< OctreeParams >::buildInnerNode(
		typename OctreeMap::iterator& firstChildIt, const typename OctreeMap::iterator& currentChildIt,
		const MortonCodePtr& parentCode, const vector< OctreeNodePtr >& children )
	{
		// These counters are used to check if the accumulated number of child node points is less than a threshold.
		// In this case, the children are deleted and their points are merged into the parent.
		int numChildren = 0;
		int numLeaves = 0;
		
		// Points to be accumulated for LOD or to be merged into the parent.
		auto childrenPoints = PointVector();
		
		for( OctreeNodePtr child : children )
		{
			RandomPointAppender::appendPoints( child, childrenPoints, numChildren, numLeaves );
		}

		if( numChildren == numLeaves && childrenPoints.size() <= m_maxPointsPerNode )
		{
			//cout << "Merging child into " << endl << parentCode->getPathToRoot( true ) << endl;
			
			// All children are leaves, but they have less points than the threshold and must be merged.
			auto tempIt = firstChildIt;
			advance( firstChildIt, numChildren );
			
			eraseNodes( tempIt, currentChildIt );
			
			// Creates leaf to replace children.
			OctreeNodePtr mergedNode = makeManaged< OctreeNode >( true );
			mergedNode->setContents( childrenPoints );
			
			( *m_hierarchy )[ parentCode ] = mergedNode;
		}
		else
		{
			//cout << "Creating LOD" << endl << parentCode->getPathToRoot( true )  << endl;
			
			// No merge or absorption is needed. Just does LOD.
			advance( firstChildIt, numChildren );
			
			( *m_hierarchy )[ parentCode ] = buildInnerNode( childrenPoints );
		}
	}
	
	template< typename OctreeParams >
	inline void RandomSampleOctree< OctreeParams >::eraseNodes( const typename OctreeMap::iterator& first,
																		 const typename OctreeMap::iterator& last )
	{
		m_hierarchy->erase( first, last );
	}
	
	template< typename OctreeParams >
	inline shared_ptr< typename OctreeParams::Node > RandomSampleOctree< OctreeParams >
	::buildInnerNode( const PointVector& childrenPoints ) const
	{
		unsigned int numChildrenPoints = childrenPoints.size();
		
		OctreeNodePtr node = makeManaged< OctreeNode >( false );
		int numSamplePoints = std::max( 1., numChildrenPoints * 0.125 );
		PointVector selectedPoints( numSamplePoints );
		
		// Gets random 1/8 of the number of points.
		for( int i = 0; i < numSamplePoints; ++i )
		{
			int choosenPoint = rand() % numChildrenPoints;
			//cout << "Iter " << i << ". Choosen point index: " << choosenPoint << endl;
			selectedPoints[ i ] = childrenPoints[ choosenPoint ];
		}
		
		node->setContents( selectedPoints );
		return node;
	}
	
	template< typename OctreeParams >
	OctreeStats RandomSampleOctree< OctreeParams >::traverse( RenderingState& renderingState,
																	   const Float& projThresh )
	{
		clock_t timing = clock();
		
		MortonCodePtr rootCode = makeManaged< MortonCode >();
		rootCode->build( 0x1 );
		
		traverse( rootCode, renderingState, projThresh );
		
		onTraversalEnd();
		
		timing = clock() - timing;
		float traversalTime = float( timing ) / CLOCKS_PER_SEC * 1000;
		
		timing = clock();
		
		unsigned int numRenderedPoints = renderingState.render();
		timing = clock() - timing;
		
		float renderingTime = float( timing ) / CLOCKS_PER_SEC * 1000;
		
		return OctreeStats( traversalTime, renderingTime, numRenderedPoints );
	}
	
	template< typename OctreeParams >
	inline AlignedBox3f RandomSampleOctree< OctreeParams >::getBoundaries( MortonCodePtr code ) const
	{
		unsigned int level = code->getLevel();
		auto nodeCoordsVec = code->decode(level);
		Vec3 nodeCoords( nodeCoordsVec[ 0 ], nodeCoordsVec[ 1 ], nodeCoordsVec[ 2 ] );
		Float nodeSizeFactor = Float( 1 ) / Float( 1 << level );
		Vec3 levelNodeSize = ( *m_size ) * nodeSizeFactor;
		
		Vec3 minBoxVert = ( *m_origin ) + ( nodeCoords.array() * levelNodeSize.array() ).matrix();
		Vec3 maxBoxVert = minBoxVert + levelNodeSize;
		
		return AlignedBox3f( minBoxVert, maxBoxVert );
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::traverse( MortonCodePtr nodeCode, RenderingState& renderingState,
																const Float& projThresh )
	{
		//cout << "TRAVERSING " << *nodeCode << endl << endl;
		auto nodeIt = m_hierarchy->find( nodeCode );
		if ( nodeIt != m_hierarchy->end() )
		{
			MortonCodePtr code = nodeIt->first;
			OctreeNodePtr node = nodeIt->second;
			AlignedBox3f box = getBoundaries( code );
			
			if( !renderingState.isCullable( box ) )
			{
				//cout << *nodeCode << "NOT CULLED!" << endl << endl;
				if( renderingState.isRenderable( box, projThresh ) )
				{
					//cout << *nodeCode << "RENDERED!" << endl << endl;
					if ( node->isLeaf() )
					{
						setupLeafNodeRendering( node, code, renderingState );
					}
					else
					{
						setupInnerNodeRendering( node, code, renderingState );
					}
				}
				else
				{
					if (node->isLeaf())
					{
						setupLeafNodeRendering( node, code, renderingState );
					}
					else
					{
						vector< MortonCodePtr > childrenCodes = nodeCode->traverseDown();
			
						for ( MortonCodePtr childCode : childrenCodes )
						{
							traverse( childCode, renderingState, projThresh );
						}
					}
				}
			}
			else
			{
				handleCulledNode( code );
			}
		}
	}
	
	template< typename OctreeParams >
	inline void RandomSampleOctree< OctreeParams >
	::setupNodeRendering( OctreeNodePtr node, RenderingState& renderingState )
	{
		PointVector points = node->getContents();
		
		renderingState.handleNodeRendering( points );
	}
	
	template< typename OctreeParams >
	inline void RandomSampleOctree< OctreeParams >
	::setupInnerNodeRendering( OctreeNodePtr innerNode, MortonCodePtr code, RenderingState& renderingState )
	{
		assert( !innerNode->isLeaf() && "innerNode cannot be leaf." );
		
		setupNodeRendering( innerNode, renderingState );
	}
	
	template< typename OctreeParams >
	inline void RandomSampleOctree< OctreeParams >
	::setupLeafNodeRendering( OctreeNodePtr leafNode, MortonCodePtr code, RenderingState& renderingState )
	{
		assert( leafNode->isLeaf() && "leafNode cannot be inner." );
		
		PointVector& points = leafNode->getContents();
		renderingState.handleNodeRendering( points );
	}
	
	template< typename OctreeParams >
	inline shared_ptr< typename OctreeParams::Hierarchy > RandomSampleOctree< OctreeParams >
	::getHierarchy() const
	{
		return m_hierarchy;
	}
		
	/** Gets the origin, which is the point contained in octree with minimun coordinates for all axis. */
	template< typename OctreeParams >
	inline shared_ptr< Vec3 > RandomSampleOctree< OctreeParams >::getOrigin() const { return m_origin; }
		
	/** Gets the size of the octree, which is the extents in each axis from origin representing the space that the octree occupies. */
	template< typename OctreeParams >
	inline shared_ptr< Vec3 > RandomSampleOctree< OctreeParams >::getSize() const { return m_size; }
	
	template< typename OctreeParams >
	inline shared_ptr< Vec3 > RandomSampleOctree< OctreeParams >::getLeafSize() const { return m_leafSize; }
		
	/** Gets the maximum number of points that can be inside an octree node. */
	template< typename OctreeParams >
	inline unsigned int RandomSampleOctree< OctreeParams >::getMaxPointsPerNode() const
	{
		return m_maxPointsPerNode;
	}
		
	/** Gets the maximum level that this octree can reach. */
	template< typename OctreeParams >
	inline unsigned int RandomSampleOctree< OctreeParams >::getMaxLevel() const { return m_maxLevel; }
	
	template< typename OctreeParams >
	ostream& operator<<( ostream& out, const RandomSampleOctree< OctreeParams >& octree )
	{
		using PointPtr = shared_ptr< typename OctreeParams::Point >;
		using PointVector = vector< PointPtr, ManagedAllocator< PointPtr > >;
		using MortonCodePtr = shared_ptr< typename OctreeParams::Morton >;
		using OctreeMapPtr = shared_ptr< typename OctreeParams::Hierarchy >;
		
		out << "=========== Begin Octree ============" << endl << endl
			<< "origin: " << *octree.m_origin << endl
			<< "size: " << *octree.m_size << endl
			<< "leaf size: " << *octree.m_leafSize << endl
			<< "max points per node: " << octree.m_maxPointsPerNode << endl << endl;
		
		/** Maximum level of this octree. */
		unsigned int m_maxLevel;
		OctreeMapPtr hierarchy = octree.getHierarchy();
		for( auto nodeIt = hierarchy->begin(); nodeIt != hierarchy->end(); ++nodeIt )
		{
			MortonCodePtr code = nodeIt->first;
			OctreeNodePtr< PointVector > genericNode = nodeIt->second;
			
			out << code->getPathToRoot( true ) << endl;
			//out << "Node: {" << endl << code->getPathToRoot( true ) << "," << endl;
			//genericNode-> template output< PointVector >( out );
			//out << endl << "}" << endl << endl;
		}
		out << "=========== End Octree ============" << endl;
		return out;
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::setMaxLvl( const int& maxLevel, const ShallowMortonCode& )
	{
		m_maxLevel = maxLevel;
		m_maxMortonLevel = 10; // 0 to 10.
		
		assert( m_maxLevel <= m_maxMortonLevel && "Octree level cannot exceed maximum." );
	}
	
	template< typename OctreeParams >
	void RandomSampleOctree< OctreeParams >::setMaxLvl( const int& maxLevel, const MediumMortonCode& )
	{
		m_maxLevel = maxLevel;
		m_maxMortonLevel = 20; // 0 to 20.
		
		assert( m_maxLevel <= m_maxMortonLevel && "Octree level cannot exceed maximum." );
	}
	
	// ==========
	// Type Sugar
	// ==========
	
	// Macro used to declare Octree types based on OctreeParams.
	// Naming convention:
	// PREFIX_NAME, where PREFIX codifies the types in OctreeParams and NAME is original Octree type.
	//
	// PREFIX = XYZzW, where:
	// X = morton code type: S for Shallow, M for Medium
	// Y = point type: P for Point, E for Extended
	// Zz = node type: 	Op for OctreeNode< PointVector >, Oi for OctreeNode< IndexVector >,
	//					1p for O1OctreeNode< PointVector >, 1i for O1OctreeNode< IndexVector >
	// W = hierarchy type: S for serial, C for concurrent
	#define DECLARE_OCTREE_TYPE(PREFIX,LHS_NAME,RHS_NAME,MORTON,POINT,NODE,HIERARCHY) \
	using PREFIX##_##LHS_NAME = RHS_NAME<\
									OctreeParams<\
										MORTON, POINT, NODE, HIERARCHY< MORTON, NODE >\
									>\
								>;\
	using PREFIX##_##LHS_NAME##Ptr = shared_ptr< PREFIX##_##LHS_NAME >;
	
	DECLARE_OCTREE_TYPE(SPOpS,RandomSampleOctree,RandomSampleOctree,ShallowMortonCode,Point,OctreeNode< PointVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(MPOpS,RandomSampleOctree,RandomSampleOctree,MediumMortonCode,Point,OctreeNode< PointVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(SEOpS,RandomSampleOctree,RandomSampleOctree,ShallowMortonCode,ExtendedPoint,OctreeNode< ExtendedPointVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(MEOpS,RandomSampleOctree,RandomSampleOctree,MediumMortonCode,ExtendedPoint,OctreeNode< ExtendedPointVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(SPOiS,RandomSampleOctree,RandomSampleOctree,ShallowMortonCode,Point,OctreeNode< IndexVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(MPOiS,RandomSampleOctree,RandomSampleOctree,MediumMortonCode,Point,OctreeNode< IndexVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(SEOiS,RandomSampleOctree,RandomSampleOctree,ShallowMortonCode,ExtendedPoint,OctreeNode< IndexVector >,OctreeMap)
	
	DECLARE_OCTREE_TYPE(MEOiS,RandomSampleOctree,RandomSampleOctree,MediumMortonCode,ExtendedPoint,OctreeNode< IndexVector >,OctreeMap)
}

#endif