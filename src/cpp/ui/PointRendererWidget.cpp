#include "PointRendererWidget.h"
#include "TucanoDebugRenderer.h"
#include <MemoryManagerTypes.h>
#include <QDebug>
#include <QTimer>

PointRendererWidget::PointRendererWidget( QWidget *parent )
: Tucano::QtFreecameraWidget( parent ),
m_distanceThresh( 1.f ),
m_renderTime( 0.f ),
m_desiredRenderTime( 0.f ),
draw_trackball( true ),
m_drawAuxViewports( false ),
m_octree( nullptr ),
m_renderer( nullptr )
{
	camera->setSpeed( 1.f );
}

PointRendererWidget::~PointRendererWidget()
{
	delete m_renderer;
	delete m_octree;
	delete m_timer;
}

void PointRendererWidget::initialize( const unsigned int& frameRate, const int& renderingTimeTolerance )
{
	// Init MemoryManager allowing 9GB of data.
// 	DefaultManager< MortonCode, Point, OctreeNode >::initInstance( 1024ul * 1024ul * 1024ul * 9 );
	
	//Ken12MemoryManager< MortonCode, Point, InnerNode, LeafNode >::initInstance(
	//	1.5f * 1024ul * 1024ul * 1024ul / sizeof( MortonCode ) /* 1.5GB for MortonCodes */,
	//	3.25f * 1024ul * 1024ul * 1024ul / sizeof( Point ) /* 3.25GB for Points */,
	//	1.625f * 1024ul * 1024ul * 1024ul / sizeof( InnerNode ) /* 1.625GB for Nodes */,
	//	1.625f * 1024ul * 1024ul * 1024ul / sizeof( LeafNode ) /* 1.625GB for Nodes */
	//);
	
// 	cout << "MemoryManager initialized: " << endl << SingletonMemoryManager::instance() << endl;
	
	Tucano::QtFreecameraWidget::initialize();
	
	setFrameRate( frameRate );
	m_renderingTimeTolerance = renderingTimeTolerance;
	
	//openMesh( "test/data/extended_point_octree.ply" );
	openMesh( "data/example/staypuff.ply" );
	//openMesh( "../../src/data/real/tempietto_all.ply" );
	//openMesh( "../../src/data/real/filippini1-4.ply" );
	//openMesh( "../../src/data/real/tempietto_sub_tot.ply" );
	
	m_timer = new QTimer( this );
	connect( m_timer, SIGNAL( timeout() ), this, SLOT( update() ) );
	m_timer->start( 16.666f ); // Update 60 fps.
}

void PointRendererWidget::resizeGL( int width, int height )
{
	// TODO: It seems that resing is resulting in memory leak ( probably in jump flooding code... ).
	
	camera->setViewport( Eigen::Vector2f( ( float )width, ( float )height ) );
	camera->setPerspectiveMatrix( camera->getFovy(), width / height, 0.1f, 500.0f );
	light_trackball.setViewport( Eigen::Vector2f( ( float )width, ( float )height ) );

	if( m_renderer )
	{
		m_renderer->getJumpFlooding().resize( width, height );
	}
	
	updateGL();
}


void PointRendererWidget::adaptRenderingThresh()
{
	float renderTimeDiff = m_renderTime - m_desiredRenderTime;
	if( abs( renderTimeDiff ) > m_renderingTimeTolerance )
	{
		m_distanceThresh += renderTimeDiff * 1.0e-6f;
		m_distanceThresh = std::max( m_distanceThresh, 1.0e-15f );
		m_distanceThresh = std::min( m_distanceThresh, 5.f );
	}
}

void PointRendererWidget::paintGL (void)
{
	//cout << "=== Painting starts ===" << endl << endl;
	
	auto frameStart = Profiler::now();
	makeCurrent();

	adaptRenderingThresh();
	
	m_renderer->setupRendering();
	
	// Render the scene.
	auto frontTrackingStart = Profiler::now();
	
	//OctreeStats stats = m_octree->traverse( *m_renderer, m_projThresh );
	FrontOctreeStats stats = m_octree->trackFront( *m_renderer, m_distanceThresh );
	
	int frontTrackingTime = Profiler::elapsedTime( frontTrackingStart );

	m_renderer->afterRendering();
	
	m_renderTime = Profiler::elapsedTime( frameStart );
	
	// Render debug data.
	stringstream debugSS;
	debugSS << "Total frame time: " << m_renderTime << " ms" << endl << endl
			<< "Render time (traversal + rendering): " << frontTrackingTime << " ms" << endl << endl
			<< "Time between frames: " << Profiler::elapsedTime( m_endOfFrameTime, frameStart ) << "ms" << endl
			<< stats
			<< "Desired render time: " << m_desiredRenderTime << " ms" << endl << endl
			<< "Rendering time tolerance: " << m_renderingTimeTolerance << " ms" << endl << endl
			<< "Projection threshold: " << m_distanceThresh << endl << endl;
			
	//cout << debugSS.str() << endl << endl;
	
	int textBoxWidth = width() * 0.3;
	int textBoxHeight = height() * 0.7;
	int margin = 10;
	debugInfoDefined( QString( debugSS.str().c_str() ) );
	
	glEnable(GL_DEPTH_TEST);
	if( draw_trackball )
	{
		camera->renderAtCorner();
	}
	
	m_endOfFrameTime = Profiler::now();
	
	if( m_drawAuxViewports )
	{
		glEnable( GL_SCISSOR_TEST );
		renderAuxViewport( FRONT );
		renderAuxViewport( SIDE );
		renderAuxViewport( TOP );
		glDisable( GL_SCISSOR_TEST );
	}
	
	//cout << "=== Painting ends ===" << endl << endl;
}

void PointRendererWidget::renderAuxViewport( const Viewport& viewport )
{
	Vector2f viewportPos;
	
	switch( viewport )
	{
		case FRONT: viewportPos[ 0 ] = 0.f; viewportPos[ 1 ] = 0.f; break;
		case SIDE: viewportPos[ 0 ] = size().width() * 0.333f; viewportPos[ 1 ] = 0.f; break;
		case TOP: viewportPos[ 0 ] = size().width() * 0.666f; viewportPos[ 1 ] = 0.f; break;
	}
	
	Vector4f auxViewportSize( viewportPos[ 0 ], viewportPos[ 1 ], size().width() * 0.333f, size().height() * 0.333f );
	glScissor( auxViewportSize.x(), auxViewportSize.y(), auxViewportSize.z(), auxViewportSize.w() );
	glClear( GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT );
	
	Trackball tempCamera;
	tempCamera.setViewport( auxViewportSize );
	tempCamera.setPerspectiveMatrix( tempCamera.getFovy(), auxViewportSize.z() / auxViewportSize.w(), 0.1f, 10000.0f );
	tempCamera.resetViewMatrix();
	
	switch( viewport )
	{
		case FRONT:
		{
			 tempCamera.translate( Vector3f( 0.f, 0.f, -200.f ) );
			 break;
		}
		case SIDE:
		{
			tempCamera.rotate( Quaternionf( AngleAxisf( 0.5 * M_PI, Vector3f::UnitY() ) ) );
			tempCamera.translate( Vector3f( 200.f, 0.f, 0.f ) );
			break;
		}
		case TOP:
		{
			tempCamera.rotate( Quaternionf( AngleAxisf( 0.5 * M_PI, Vector3f::UnitX() ) ) );
			tempCamera.translate( Vector3f( 0.f, -200.f, 0.f ) );
			break;
		}
	}
	
	Phong &phong = m_renderer->getPhong();
	phong.render( mesh, tempCamera, light_trackball );
}

void PointRendererWidget::toggleWriteFrames()
{
	m_renderer->getJumpFlooding().toggleWriteFrames();	
	updateGL();
}

void PointRendererWidget::toggleEffect( int id )
{
	m_renderer->selectEffect( ( Renderer::Effect ) id );
	updateGL();
}

void PointRendererWidget::reloadShaders( void )
{
	m_renderer->getPhong().reloadShaders();
	m_renderer->getJumpFlooding().reloadShaders();
	updateGL();
}

void PointRendererWidget::setFrameRate( const unsigned int& frameRate )
{
	m_desiredRenderTime = 1000.f / ( float ) frameRate;
}

void PointRendererWidget::setJFPBRFirstMaxDistance( double value )
{
	m_renderer->getJumpFlooding().setFirstMaxDistance( ( float )value );
	updateGL();
}

void PointRendererWidget::toggleDrawTrackball( void )
{
	draw_trackball = !draw_trackball;
	updateGL();
}

void PointRendererWidget::toggleDrawAuxViewports( void )
{
	m_drawAuxViewports = !m_drawAuxViewports;
	updateGL();
}

void PointRendererWidget::toggleNodeDebugDraw( const int& value )
{
// 	m_octree->toggleDebug( value );
	updateGL();
}

void PointRendererWidget::setJfpbrFrameskip( const int& value )
{
	m_renderer->setJfpbrFrameskip( value );
}
	
void PointRendererWidget::setRenderingTimeTolerance( const int& tolerance )
{
	m_renderingTimeTolerance = tolerance;
}

void PointRendererWidget::openMesh( const string& filename )
{
	if( m_octree )
	{
		delete m_octree;
	}
	//m_octree = new Octree( 1, 10 );
	
	int nameBeginning = filename.find_last_of( "/" ) + 1;
	int nameEnding = filename.find_last_of( "." );
	string dbFilename = filename.substr( nameBeginning, nameEnding - nameBeginning ) + ".db";
	cout << endl << "Database filename: " << dbFilename << endl << endl;
// 	m_octree = new Octree( 1, 10, dbFilename );
// 	m_octree->buildFromFile( filename, PointReader::SINGLE );
	
	uint octreeDepth = 12;
	m_octree = new Octree( filename, octreeDepth, 1024, 1024ul * 1024ul * 1024ul * 1ul, 4, true );
	
	cout << "Octree built." << endl;
	
	mesh.reset();
	if( m_renderer )
	{
		delete m_renderer;
	}
	
	// Render the scene one time, traveling from octree's root to init m_renderTime for future projection
	// threshold adaptations.
	m_renderer = new Renderer( /*m_octree->getPoints(),*/ camera, &light_trackball, &mesh, "shaders/tucano/", octreeDepth );
	
	cout << "Renderer built." << endl;
	
	auto frontTrackingStart = Profiler::now();
	
	m_octree->trackFront( *m_renderer, m_distanceThresh );
	updateGL();
	
	m_endOfFrameTime = Profiler::now();
	m_renderTime = Profiler::elapsedTime( frontTrackingStart, m_endOfFrameTime );
}