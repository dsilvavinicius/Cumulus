#ifndef POINT_SORTER_H
#define POINT_SORTER_H

#include "MortonCode.h"
#include "PlyPointReader.h"

using namespace std;
using namespace util;

namespace model
{
	/** Sorts a point .ply file in z order.
	 * @param M is the MortonCode type.
	 * @param P is the Point type. */
	template< typename M, typename P >
	class PointSorter
	{
		using Reader = PlyPointReader< P >;
		
	public:
		PointSorter( const string& input, uint zCurveLvl );
		~PointSorter();
		void sort( const string& outFilename );
	
	private:
		M calcMorton( const P& point );
		void write( const Point& p );
		void write( const ExtendedPoint& p );
		
		Reader* m_reader;
		p_ply m_output;
		
		P* m_points;
		long m_nPoints;
		
		Vec3 m_origin;
		Vec3 m_leafSize;
		uint m_zCurveLvl;
	};
	
	template< typename M, typename P >
	PointSorter< M, P >::PointSorter( const string& input, uint zCurveLvl )
	: m_zCurveLvl( zCurveLvl )
	{
		m_reader = new Reader( input );
		m_nPoints = m_reader->getNumPoints();
		m_points = ( P* ) malloc( sizeof( P ) * m_nPoints );
		
		Float negInf = -numeric_limits< Float >::max();
		Float posInf = numeric_limits< Float >::max();
		m_origin = Vec3( posInf, posInf, posInf );
		Vec3 maxCoords( negInf, negInf, negInf );
		
		long i = 0;
		m_reader->read( Reader::SINGLE,
			[ & ]( const P& p )
			{
				m_points[ i++ ] = p;
				Vec3 pos = p.getPos();
				for( int i = 0; i < 3; ++i )
				{
					m_origin[ i ] = glm::min( m_origin[ i ], pos[ i ] );
					maxCoords[ i ] = glm::max( maxCoords[ i ], pos[ i ] );
				}
			}
		);
		
		m_leafSize = ( maxCoords - m_origin ) * ( ( Float )1 / ( ( unsigned long long )1 << zCurveLvl ) );
	}
	
	template< typename M, typename P >
	PointSorter< M, P >::~ PointSorter()
	{
		free( m_points );
		delete m_reader;
	}
	
	template< typename M, typename P >
	class PointComp
	{
	public:
		PointComp( const Vec3& origin, const Vec3& leafSize, uint zCurveLvl )
		: m_origin( m_origin ),
		m_leafSize( leafSize ),
		m_zCurveLvl( zCurveLvl )
		{}
		
		M calcMorton( const P& point ) const
		{
			const Vec3& pos = point.getPos();
			Vec3 index = ( pos - m_origin ) / m_leafSize;
			M code;
			code.build( index.x, index.y, index.z, m_zCurveLvl );
			
			return code;
		}
		
		bool operator()( const P& p0, const P& p1 )
		{
			return calcMorton( p0 ) < calcMorton( p1 );
		}
		
		Vec3 m_origin;
		Vec3 m_leafSize;
		uint m_zCurveLvl;
	};
	
	template< typename M, typename P >
	void PointSorter< M, P >::sort( const string& outFilename )
	{
		PointComp< M, P > comp( m_origin, m_leafSize, m_zCurveLvl );
		std::sort( m_points, m_points + m_nPoints, comp );
		
		// Write output file
		m_output = m_reader->copyHeader( outFilename );
		for( int i = 0; i < m_nPoints; ++i )
		{
			write( m_points[ i ] );
		}
		
		ply_close( m_output );
	}
	
	template< typename M, typename P >
	inline void PointSorter< M, P >::write( const Point& p )
	{
		Vec3 pos = p.getPos();
		ply_write( m_output, pos.x );
		ply_write( m_output, pos.y );
		ply_write( m_output, pos.z );
		
		Vec3 normal = p.getColor();
		ply_write( m_output, normal.x );
		ply_write( m_output, normal.y );
		ply_write( m_output, normal.z );
	}
	
	template< typename M, typename P >
	inline void PointSorter< M, P >::write( const ExtendedPoint& p )
	{
		Vec3 pos = p.getPos();
		ply_write( m_output, pos.x );
		ply_write( m_output, pos.y );
		ply_write( m_output, pos.z );
		
		Vec3 normal = p.getNormal();
		ply_write( m_output, normal.x );
		ply_write( m_output, normal.y );
		ply_write( m_output, normal.z );
		
		Vec3 color = p.getColor();
		ply_write( m_output, color.x );
		ply_write( m_output, color.y );
		ply_write( m_output, color.z );
	}
}

#endif