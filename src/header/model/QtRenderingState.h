#ifndef QT_RENDERING_STATE_H
#define QT_RENDERING_STATE_H

#include "RenderingState.h"
#include "MortonCode.h"
#include "OctreeNode.h"
#include <QGLPainter>

namespace model
{
	/** RenderingState using Qt as render. USAGE: in addition to the RenderingState steps, setPainter() should be
	 * called when the QGLPainter is known and everytime it needs to be updated.
	 * @param Vec3 is the type for 3-dimensional vector.
	 * @param Float is the type for floating point numbers. */
	template< typename Vec3, typename Float >
	class QtRenderingState
	: public RenderingState< Vec3, Float >
	{
		using RenderingState = model::RenderingState< Vec3, Float >;
	public:
		QtRenderingState( const Attributes& attribs ) : RenderingState( attribs ) {  }
		
		virtual ~QtRenderingState() = 0;
		
		QGLPainter* getPainter() { return m_painter; };
		
		/** This method should be called on the starting of the rendering loop, when the painter is known. */
		void setPainter( QGLPainter* painter, const QSize& viewportSize );
		
		virtual bool isCullable( const pair< Vec3, Vec3 >& box ) const;
		
		/** This implementation will compare the size of the maximum box diagonal in window coordinates with the projection
		 * threshold.
		 *	@param projThresh is the threshold of the squared size of the maximum box diagonal in window coordinates. */
		virtual bool isRenderable( const pair< Vec3, Vec3 >& box, const Float& projThresh ) const;
		
		/** Draws the boundaries of the octree nodes.
		 * @param passProjTestOnly indicates if only the nodes that pass the projection test should be rendered. */
		template< typename Octree, typename MortonPrecision >
		void drawBoundaries( const Octree& octree, const bool& passProjTestOnly, const Float& projThresh ) const;
		
		/** Utility method to insert node boundary point into vectors for rendering. */
		static void insertBoundaryPoints( vector< Vec3 >& verts, vector< Vec3 >& colors,
										  const pair< Vec3, Vec3 >& box, const bool& isCullable,
									const bool& isRenderable );
	
	protected:
		QVector2D projToWindowCoords( const QVector4D& point, const QMatrix4x4& viewProj ) const;
		
		QGLPainter* m_painter;
		QVector2D m_viewportSize;
	};
	
	template< typename Vec3, typename Float >
	QtRenderingState< Vec3, Float >::~QtRenderingState() {}
	
	template< typename Vec3, typename Float >
	inline bool QtRenderingState< Vec3, Float >::isCullable( const pair< Vec3, Vec3 >& box ) const
	{
		Vec3 minBoxVert = box.first;
		Vec3 maxBoxVert = box.second;
		QBox3D qBox( QVector3D( minBoxVert.x, minBoxVert.y, minBoxVert.z ),
					 QVector3D( maxBoxVert.x, maxBoxVert.y, maxBoxVert.z ) );
		
		return m_painter->isCullable( qBox );
	}
	
	template< typename Vec3, typename Float >
	inline bool QtRenderingState< Vec3, Float >::isRenderable( const pair< Vec3, Vec3 >& box,
															   const Float& projThresh ) const
	{
		Vec3 rawMin = box.first;
		Vec3 rawMax = box.second;
		QVector4D min( rawMin.x, rawMin.y, rawMin.z, 1 );
		QVector4D max( rawMax.x, rawMax.y, rawMax.z, 1 );
		
		QGLPainter* painter = m_painter;
		QMatrix4x4 viewProj = painter->combinedMatrix();
		
		QVector2D proj0 = projToWindowCoords( min, viewProj );
		QVector2D proj1 = projToWindowCoords( max, viewProj );
		
		QVector2D diagonal0 = proj1 - proj0;
		
		Vec3 rawSize = rawMax - rawMin;
		QVector3D boxSize( rawSize.x, rawSize.y, rawSize.z );
		
		proj0 = projToWindowCoords( QVector4D( min.x() + boxSize.x(), min.y() + boxSize.y(), min.z(), 1 ), viewProj );
		proj1 = projToWindowCoords( QVector4D( max.x(), max.y(), max.z() + boxSize.z(), 1 ), viewProj );
		
		QVector2D diagonal1 = proj1 - proj0;
		
		Float maxDiagLength = glm::max( diagonal0.lengthSquared(), diagonal1.lengthSquared() );
		
		return maxDiagLength < projThresh;
	}
	
	template< typename Vec3, typename Float >
	void QtRenderingState< Vec3, Float >::setPainter( QGLPainter* painter, const QSize& viewportSize )
	{
		m_painter = painter;
		m_viewportSize.setX( viewportSize.width() );
		m_viewportSize.setY( viewportSize.height() );
	}
	
	template< typename Vec3, typename Float >
	template< typename Octree, typename MortonPrecision >
	void QtRenderingState< Vec3, Float >::drawBoundaries( const Octree& octree, const bool& passProjTestOnly,
														  const Float& projThresh ) const
	{
		// Saves current effect.
		QGLAbstractEffect* effect = m_painter->effect();
		
		m_painter->setStandardEffect(QGL::FlatPerVertexColor);
		vector< Vec3 > verts;
		vector< Vec3 > colors;
		
		for( pair< MortonCodePtr< MortonPrecision >, OctreeNodePtr< MortonPrecision, Float, Vec3 > > entry
			: *octree.getHierarchy() )
		{
			MortonCodePtr< MortonPrecision > code = entry.first;
			pair< Vec3, Vec3 > box = octree.getBoundaries( code );
			bool cullable = isCullable( box );
			bool renderable = isRenderable( box, projThresh );
			
			if( passProjTestOnly )
			{
				if (renderable)
				{
					insertBoundaryPoints( verts, colors, box, cullable, renderable );
				}
			}
			else
			{
				insertBoundaryPoints( verts, colors, box, cullable, renderable );
			}
		}
		
		// TODO: Find a way to parametrize precision here.
		QGLAttributeValue vertPosAttrib(3, GL_FLOAT, 0, &verts[0]);
		QGLAttributeValue colorAttrib(3, GL_FLOAT, 0, &colors[0]);
		
		m_painter->clearAttributes();
		m_painter->setVertexAttribute(QGL::Position, vertPosAttrib);
		m_painter->setVertexAttribute(QGL::Color, colorAttrib);
		m_painter->draw(QGL::Lines, verts.size());
		
		m_painter->clearAttributes();
		
		// Restores previous effect.
		m_painter->setUserEffect(effect);
	}
	
	template< typename Vec3, typename Float >
	inline void QtRenderingState< Vec3, Float >::insertBoundaryPoints( vector< Vec3 >& verts,
																	   vector< Vec3 >& colors,
																	const pair< Vec3, Vec3 >& box,
																	const bool& isCullable,
																	const bool& isRenderable )
	{
		Vec3 min = box.first;
		Vec3 max = box.second;
		QVector3D qv0( min.x, min.y, min.z );
		QVector3D qv6( max.x, max.y, max.z );
			
		Vec3 v0(qv0.x(), qv0.y(), qv0.z());
		Vec3 v6(qv6.x(), qv6.y(), qv6.z());
		Vec3 size = v6 - v0;
		Vec3 v1(v0.x + size.x, v0.y			, v0.z);
		Vec3 v2(v1.x		 , v1.y + size.y, v1.z);
		Vec3 v3(v2.x - size.x, v2.y			, v2.z);
		Vec3 v4(v0.x		 , v0.y			, v0.z + size.z);
		Vec3 v5(v4.x + size.x, v4.y			, v4.z);
		Vec3 v7(v6.x - size.x, v6.y			, v6.z);
		
		// Face 0.
		verts.push_back(v0); verts.push_back(v1);
		verts.push_back(v1); verts.push_back(v2);
		verts.push_back(v2); verts.push_back(v3);
		verts.push_back(v3); verts.push_back(v0);
		// Face 1.
		verts.push_back(v4); verts.push_back(v5);
		verts.push_back(v5); verts.push_back(v6);
		verts.push_back(v6); verts.push_back(v7);
		verts.push_back(v7); verts.push_back(v4);
		// Connectors.
		verts.push_back(v0); verts.push_back(v4);
		verts.push_back(v1); verts.push_back(v5);
		verts.push_back(v2); verts.push_back(v6);
		verts.push_back(v3); verts.push_back(v7);
		
		if (isCullable)
		{
			colors.insert(colors.end(), 24, Vec3(1, 0, 0));
		}
		else if (isRenderable)
		{
			colors.insert(colors.end(), 24, Vec3(1, 1, 1));
		}
		else
		{
			colors.insert(colors.end(), 24, Vec3(0, 0, 1));
		}
	}
	
	template< typename Vec3, typename Float >
	inline QVector2D QtRenderingState< Vec3, Float >::projToWindowCoords( const QVector4D& point, const QMatrix4x4& viewProj )
	const
	{
		QVector4D proj = viewProj.map( point );
		QVector2D normalizedProj( proj / proj.w() );
		//QVector2D windowProj = ( normalizedProj + QVector2D( 1.f, 1.f ) ) * 0.5f * m_viewportSize;
		//return windowProj;
		return normalizedProj;
	}
}

#endif