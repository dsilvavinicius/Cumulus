#include <gtest/gtest.h>
#include <utils/qtflycamerawidget.hpp>
#include <QApplication>
#include "PlyPointReader.h"
#include "StreamingRenderer.h"
#include "OglUtils.h"

using namespace std;
using namespace util;

namespace model
{
	namespace test
	{
		class StreamingRendererTestWidget
		: public QtFlycameraWidget
		{
		public:
			using PointVector = vector< ExtendedPointPtr >;
			using Renderer = StreamingRenderer< ExtendedPoint >;
			
			StreamingRendererTestWidget( QWidget *parent = 0 )
			: QtFlycameraWidget( parent )
			{}
			
			~StreamingRendererTestWidget()
			{
				delete m_renderer;
			}
			
			void initialize()
			{
				QtFlycameraWidget::initialize();
				
				PlyPointReader< ExtendedPoint > reader( "../data/example/staypuff.ply" );
				ulong nPoints = reader.getNumPoints();
				
				Float negInf = -numeric_limits< Float >::max();
				Float posInf = numeric_limits< Float >::max();
				Vec3 origin = Vec3( posInf, posInf, posInf );
				Vec3 maxCoords( negInf, negInf, negInf );
				
				reader.read(
					[ & ]( const ExtendedPoint& p )
					{
						m_points.push_back( make_shared< ExtendedPoint >( p ) );
						
						const Vec3& pos = p.getPos();
						
						for( int i = 0; i < 3; ++i )
						{
							origin[ i ] = std::min( origin[ i ], pos[ i ] );
							maxCoords[ i ] = std::max( maxCoords[ i ], pos[ i ] );
						}
					}
				);
				
				Vec3 boxSize = maxCoords - origin;
				float scale = 1.f / std::max( std::max( boxSize.x(), boxSize.y() ), boxSize.z() );
				
				for( ExtendedPointPtr p : m_points )
				{
					Vec3& pos = p->getPos();
					pos = ( pos - origin ) * scale;
				}
				
				m_segmentSize = 50000;
				m_nSegments = ceil( float( reader.getNumPoints() ) / float( m_segmentSize ) );
				m_renderer = new Renderer( camera, &light_trackball, "../shaders/tucano/" );
				m_renderer->selectSegments( 0, 2 );
				OglUtils::checkOglErrors();
			}
			
			void paintGL() override
			{
				loadSegments();
				
				m_renderer->setupRendering();
				
				OglUtils::checkOglErrors();
				
				cout << "Rendering" << endl << endl;
				m_renderer->render();
				
				OglUtils::checkOglErrors();
				
				camera->renderAtCorner();
			}
		
		protected:
			virtual void keyPressEvent( QKeyEvent * event ) override
			{
				if( event->key() == Qt::Key_N )
				{
					cout << "Selecting segments" << endl << endl;
					
					uint segSelectionIdx = m_renderer->segSelectionIdx() + m_renderer->segSelectionSize();
					uint segSelectionSize;
					
					if( segSelectionIdx >= m_nSegments )
					{
						segSelectionIdx = 0;
						segSelectionSize = 2;
					}
					else
					{
						segSelectionSize = ( segSelectionIdx + 2 <= m_nSegments ) ? 2 : 1;
					}
					
					m_renderer->selectSegments( segSelectionIdx, segSelectionSize );
				}
				else
				{
					QtFlycameraWidget::keyPressEvent( event );
				}
			}
		
		private:
			void loadSegments()
			{
				m_renderer->mapAttribs();
				uint segSelectionIdx = m_renderer->segSelectionIdx();
				uint segSelectionSize = m_renderer->segSelectionSize();
				OglUtils::checkOglErrors();
				
				for( uint i = segSelectionIdx; i < segSelectionIdx + segSelectionSize; ++i )
				{
					cout << "Loading segment " << i << endl << endl;
					
					for( uint j = i * m_segmentSize; j < ( i + 1 ) * m_segmentSize && j < m_points.size(); ++j )
					{
						Array< ExtendedPointPtr > ptArray( 1 );
						ptArray[ 0 ] = m_points[ j ];
						m_renderer->handleNodeRendering( ptArray, i );
					}
				}
			}
			
			Renderer* m_renderer;
			PointVector m_points;
			uint m_segmentSize;
			int m_nSegments;
		};
		
        class StreamingRendererTest : public ::testing::Test
		{
		protected:
			void SetUp() {}
		};
		
		TEST_F( StreamingRendererTest, All )
		{
			StreamingRendererTestWidget widget;
			widget.resize( 640, 480 );
			widget.show();
			widget.initialize();

			QApplication::exec();
		}
	}
}