#ifndef PLY_POINT_READER_H
#define PLY_POINT_READER_H

#include <locale.h>
#include <string>
#include <rply/rply.h>
#include "omicron/basic/point.h"
#include "omicron/renderer/rendering_state.h"
#include "omicron/disk/point_reader.h"
#include "omicron/util/profiler.h"

namespace omicron::disk
{
    using namespace std;
    using namespace util;
    
	class PlyPointWritter;

	/** Reader for a point .ply file. The file is opened at constructor and closed at destructor. */
	class PlyPointReader
	: public PointReader
	{
	public:
		friend PlyPointWritter;
		
		/** Checks if the file is valid, opens it, reads its header and discovers the number of points in it.
		 * @throws runtime_error if the file or its header cannot be read.  */
		PlyPointReader( const string& fileName );
		
		~PlyPointReader() { ply_close( m_ply ); }
		
		/** Copies the header of the file managed by this reader to other file. */
		p_ply copyHeader( const string& outFilename );
		
		/** Reads a .ply file. */
		void read( const function< void( const Point& ) >& onPointDone ) override;
	
		long getNumPoints() { return m_numPoints; }
	
	protected:
		void setupAdditionalCallbacks( p_ply ply, pair< Point*, function< void( const Point& ) >* >& cbNeededData );
		
		/** Internal customizable reading method. Should setup reading needed data, callbacks, do the reading itself and free reading
		 * needed data.
		 * @returns ply_read() return code. */
		virtual int doRead( p_ply& ply );
		
		/** Copies the property to out's header. */
		void copyProperty( p_ply_property property, p_ply out );
		
		/** Internal method that calculates the property flag for each invocation of the reading vertex callback. */
		static unsigned int getPropFlag( const unsigned int& propIndex );
		
		/** Method used as RPly vertex callback when normals are present. */
		static int vertexCBNormals( p_ply_argument argument );
		
		/** Method used as RPly vertex callback when normals are not present. */
		static int vertexCBPosOnly( p_ply_argument argument );
		
		long m_numPoints;
		
		function< void( const Point& ) > m_onPointDone;
		
		p_ply m_ply;
		
		string m_filename;
		
		bool m_hasNormals;
	};
	
	inline PlyPointReader::PlyPointReader( const string& fileName )
	: PointReader(),
	m_filename( fileName ),
	m_hasNormals( false )
	{
		auto now = Profiler::now( "PlyPointReader init" );
		
		cout << "Setup read of " << m_filename << endl << endl;
		
		// Open ply.
		m_ply = ply_open( m_filename.c_str(), NULL, 0, NULL );
		if( !m_ply )
		{
			throw runtime_error( m_filename + ": cannot open .ply point file." );
		}
		
		// Read header.
		if( !ply_read_header( m_ply ) )
		{
			ply_close( m_ply );
			throw runtime_error( "Cannot read point file header." );
		}
		
		// Verify the properties of the vertex element in the .ply file in order to set the normal flag.
		p_ply_element element = ply_get_next_element( m_ply, NULL );
		ply_get_element_info( element, NULL, &m_numPoints );
		
		cout << "Vertices in file: " << m_numPoints << endl << endl << "=== Elements in header ===" << endl << endl;
		
		while( element != NULL )
		{
			p_ply_property  property = ply_get_next_property( element, NULL );
			long numElements = 0;
			const char* elementName;
			ply_get_element_info( element, &elementName, &numElements );
			
			cout << elementName << ": " << numElements << " instances" << endl;
			
			while( property != NULL )
			{
				const char* name;
				ply_get_property_info( property, &name, NULL, NULL, NULL );
				cout << "Prop name: " << name << endl;
				
				if( strcmp( name, "nx" ) == 0 )
				{
					m_hasNormals = true;
				}
				
				property = ply_get_next_property( element, property );
			}
			
			cout << endl;
			
			element = ply_get_next_element( m_ply, element );
		}
		
		m_initTime = Profiler::elapsedTime( now, "PlyPointReader init" );
	}
	
	inline void PlyPointReader::read( const function< void( const Point& ) >& onPointDone )
	{
		auto now = Profiler::now( "PlyPointReader read" );
		
		m_onPointDone = onPointDone;
		
		/* Save application locale */
// 		const char *old_locale = setlocale( LC_NUMERIC, NULL );
		/* Change to PLY standard */
// 		setlocale( LC_NUMERIC, "C" );
		
		int resultCode = doRead( m_ply );
		
		if( !resultCode )
		{
			ply_close( m_ply );
// 			setlocale( LC_NUMERIC, old_locale );
			throw runtime_error( "Problem while reading points." );
		}
		
		/* Restore application locale when done */
// 		setlocale( LC_NUMERIC, old_locale );

		m_readTime = Profiler::elapsedTime( now, "PlyPointReader read" );
	}
	
	inline int PlyPointReader::doRead( p_ply& ply )
	{
		/** Temp point used to hold intermediary incomplete data before sending it to its final destiny. */
		Point tempPoint;
		pair< Point*, function< void( const Point& ) >* > cbNeededData( &tempPoint, &m_onPointDone );
		
		p_ply_read_cb callback = ( m_hasNormals ) ? PlyPointReader::vertexCBNormals : PlyPointReader::vertexCBPosOnly;
		
		ply_set_read_cb( ply, "vertex", "x", callback, &cbNeededData, 0 );
		ply_set_read_cb( ply, "vertex", "y", callback, &cbNeededData, 1 );
		ply_set_read_cb( ply, "vertex", "z", callback, &cbNeededData, 2 );
		
		if( m_hasNormals )
		{
			ply_set_read_cb( ply, "vertex", "nx", callback, &cbNeededData, 3 );
			ply_set_read_cb( ply, "vertex", "ny", callback, &cbNeededData, 4 );
			ply_set_read_cb( ply, "vertex", "nz", callback, &cbNeededData, 5 );
		}
		
		return ply_read( ply );
	}
	
	inline int PlyPointReader::vertexCBNormals( p_ply_argument argument )
	{
		long index;
		void *rawReadingData;
		ply_get_argument_user_data( argument, &rawReadingData, &index );
		
		auto readingData = ( pair< Point*, function< void( const Point& ) >* >* ) rawReadingData;
		
		float value = ply_get_argument_value( argument );
		
		Point* tempPoint = readingData->first;
		
		switch( index )
		{
			case 0: case 1: case 2:
			{
				tempPoint->getPos()[ index ] = value;
				break;
			}
			case 3: case 4:
			{
				// Normal case.
				tempPoint->getNormal()[ index % 3 ] = value;
				break;
			}
			case 5:
			{
				// Last point component. Send complete point to vector.
				tempPoint->getNormal()[ index % 3 ] = ( float ) value;
				( *readingData->second )( Point( tempPoint->getNormal(), tempPoint->getPos() ) );
				break;
			}
		}
		
		return 1;
	}
	
	inline int PlyPointReader::vertexCBPosOnly( p_ply_argument argument )
	{
		long index;
		void *rawReadingData;
		ply_get_argument_user_data( argument, &rawReadingData, &index );
		
		auto readingData = ( pair< Point*, function< void( const Point& ) >* >* ) rawReadingData;
		
		float value = ply_get_argument_value( argument );
		
		Point* tempPoint = readingData->first;
		
		switch( index )
		{
			case 0: case 1:
			{
				tempPoint->getPos()[ index ] = value;
				break;
			}
			case 2:
			{
				// Last point component. Send complete point to vector.
				tempPoint->getPos()[ index ] = value;
				( *readingData->second )( Point( Vec3( 1.f, 0.f, 0.f ), tempPoint->getPos() ) );
				break;
			}
		}
		
		return 1;
	}
}

#endif
