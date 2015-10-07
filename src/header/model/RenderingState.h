#ifndef RENDERING_STATE_H
#define RENDERING_STATE_H

#include <stdexcept>
#include <vector>
#include "ExtendedPoint.h"
#include "Stream.h"

using namespace std;

namespace model
{
	enum Attributes
	{
		COLORS = 0x1,
		NORMALS = 0x2,
		COLORS_AND_NORMALS = 0x3
	};
	
	/** Renders related data used while traversing octree. USAGE: call handleNodeRendering() to indicate that the
	 * contents of octree nodes should be rendered. After all nodes are issued for rendering, call render() to
	 * render them all.
	 * @param Vec3 is the type for 3-dimensional vector.
	 * @param Float is the type for floating point numbers. */
	
	class RenderingState
	{
	public:
		RenderingState( const Attributes& attribs );
		
		/** Event ocurring to setup rendering. Must be called before handling any node in a rendering loop.
		 * Default implementation does nothing. */
		virtual void setupRendering(){};
		
		/** Renders the current state. Should be called at the end of the traversal, when all rendered nodes have
		 * been already handled.
		 * @returns the number of rendered points. */
		virtual unsigned int render() = 0;
		
		/** Event ocurring to setup rendering. Must be called after render() call. Default implementation does nothing. */
		virtual void afterRendering(){};
		
		/** Checks if the axis-aligned box is culled by camera frustum.
		 * @returns true if the box should be culled and false otherwise. */
		virtual bool isCullable( const pair< Vec3, Vec3 >& box ) const = 0;
		
		/** Checks if the axis-aligned box is renderable with the current projection threshold.
		 * @returns true if the box has a projection compatible with the current threshold and, thus, should be
		 * rendered. False otherwise (indicating that the traversal should proceed deeper in the hierarchy). */
		virtual bool isRenderable( const pair< Vec3, Vec3 >& box, const Float& projThresh ) const = 0;
		
		/** Indicates that the node contents passed should be rendered. */
		virtual void handleNodeRendering( const PointPtr& point );
		
		/** Indicates that the passed string should be rendered at the position also passed as parameter. Useful for
		 * debugging and labelling. */
		virtual void renderText( const Vec3& pos, const string& str ) = 0;
		
		/** Indicates that the node contents passed should be rendered. */
		virtual void handleNodeRendering( const PointVector& points );
		
		/** Indicates that the node contents passed should be rendered. */
		virtual void handleNodeRendering( const ExtendedPointPtr& point );
		
		/** Indicates that the node contents passed should be rendered. */
		virtual void handleNodeRendering( const ExtendedPointVector& points );
		
		/** Indicates that the node contents passed should be rendered. */
		virtual void handleNodeRendering( const IndexVector& points );
		
		vector< Vec3 >& getPositions() { return m_positions; }
		vector< Vec3 >& getColors() { return m_colors; }
		vector< Vec3 >& getNormals() { return m_normals; }
		vector< unsigned int >& getIndices() { return m_indices; }
		Attributes getAttribs() { return m_attribs; }
	
		/** Clears all attrib vectors. */
		void clearAttribs();
		
		/** Clears indices. */
		void clearIndices();
		
	protected:
		vector< Vec3 > m_positions;
		vector< Vec3 > m_colors;
		vector< Vec3 > m_normals;
		vector< unsigned int > m_indices;
		Attributes m_attribs;
	};
	
	inline void RenderingState::handleNodeRendering( const PointPtr& point )
	{
		m_positions.push_back( point->getPos() );
		if( m_attribs == COLORS )
		{
			m_colors.push_back( point->getColor() );
		}
		else
		{
			m_normals.push_back( point->getColor() );
		}
	}
	
	inline void RenderingState::handleNodeRendering( const PointVector& points )
	{
		for( PointPtr point : points )
		{
			handleNodeRendering( point );
		}
	}
	
	inline void RenderingState::handleNodeRendering( const ExtendedPointPtr& point )
	{
		m_positions.push_back( point->getPos() );
		m_colors.push_back( point->getColor() );
		m_normals.push_back( point->getNormal() );
	}
	
	inline void RenderingState::handleNodeRendering( const ExtendedPointVector& points )
	{
		for( ExtendedPointPtr point : points )
		{
			handleNodeRendering( point );
		}
	}
		
	inline void RenderingState::handleNodeRendering( const IndexVector& points )
	{
		for( unsigned int index : points )
		{
			m_indices.push_back( index );
		}
	}
}

#endif