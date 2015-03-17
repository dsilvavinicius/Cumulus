#include "PointRendererWidget.h"
#include <QDebug>
#include <QTimer>

PointRendererWidget::PointRendererWidget( QWidget *parent )
: Tucano::QtTrackballWidget(parent),
m_projThresh( 0.001f ),
m_renderTime( 0.f ),
m_desiredRenderTime( 0.f ),
m_endOfFrameTime( clock() ),
draw_trackball( true ),
m_octree( nullptr ),
m_renderer( nullptr )
{
}

PointRendererWidget::~PointRendererWidget()
{
	delete m_renderer;
	delete m_octree;
	delete m_timer;
}

void PointRendererWidget::initialize( const unsigned int& frameRate )
{
	setFrameRate( frameRate );
	
	Tucano::QtTrackballWidget::initialize();
	openMesh( "../../src/data/real/staypuff.ply" );

	m_timer = new QTimer( this );
	connect( m_timer, SIGNAL( timeout() ), this, SLOT( update() ) );
	m_timer->start( 16.666f ); // Update 60 fps.
}

void PointRendererWidget::resizeGL( int width, int height )
{
	camera.setViewport( Eigen::Vector2f( ( float )width, ( float )height ) );
	camera.setPerspectiveMatrix( camera.getFovy(), width / height, 0.1f,
											1000.0f );
	light_trackball.setViewport( Eigen::Vector2f( ( float )width, ( float )height ) );

	if( m_renderer )
	{
		m_renderer->getJumpFlooding().resize( width, height );
	}
	
	updateGL();
}


void PointRendererWidget::adaptProjThresh( float desiredRenderTime )
{
	float renderTimeDiff = m_renderTime - desiredRenderTime;
	m_projThresh += renderTimeDiff * 1.0e-6f;
	m_projThresh = std::max( m_projThresh, 1.0e-15f );
	m_projThresh = std::min( m_projThresh, 1.f );
}

void PointRendererWidget::paintGL (void)
{
	clock_t startOfFrameTime = clock();
	clock_t totalTiming = startOfFrameTime;
	makeCurrent();

	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	
	//cout << "STARTING PAINTING!" << endl;
	//m_octree->drawBoundaries(painter, true);
	
	adaptProjThresh( m_desiredRenderTime );
	
	mesh.reset();
	m_renderer->clearAttribs();
	m_renderer->updateFrustum();
	
	// Render the scene.
	clock_t timing = clock();
	//OctreeStats stats = m_octree->traverse( painter, m_attribs, m_projThresh );
	FrontOctreeStats stats = m_octree->trackFront( *m_renderer, m_projThresh );
	timing = clock() - timing;
	
	m_renderTime = float( timing ) / CLOCKS_PER_SEC * 1000;

	totalTiming = clock() - totalTiming;
	
	// Render debug data.
	stringstream debugSS;
	debugSS << "Total loop time: " << float( totalTiming ) / CLOCKS_PER_SEC * 1000 << endl << endl
			<< "Render time (traversal + rendering): " << m_renderTime << " ms" << endl << endl
			<< "Time between frames: " << float( startOfFrameTime - m_endOfFrameTime ) / CLOCKS_PER_SEC * 1000 <<
			"ms" << endl
			<< stats
			<< "Desired render time: " << m_desiredRenderTime << "ms" << endl << endl
			<< "Projection threshold: " << m_projThresh << endl << endl;
			
	//cout << debugSS.str() << endl << endl;
	
	int textBoxWidth = width() * 0.3;
	int textBoxHeight = height() * 0.7;
	int margin = 10;
	debugInfoDefined( QString( debugSS.str().c_str() ) );
	
	glEnable(GL_DEPTH_TEST);
	if( draw_trackball )
	{
		camera.render();
	}
	
	m_endOfFrameTime = clock();
}

void PointRendererWidget::toggleWriteFrames()
{
	m_renderer->getJumpFlooding().toggleWriteFrames();	
	updateGL();
}

void PointRendererWidget::toggleEffect( int id )
{
	m_renderer->selectEffect( ( TucanoRenderingState::Effect ) id );
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

void PointRendererWidget::openMesh( const string& filename )
{
	Attributes vertAttribs = COLORS_AND_NORMALS;
	
	if( m_octree )
	{
		delete m_octree;
	}
	m_octree = new ShallowFrontOctree( 1, 10 );
	m_octree->build( filename, ExtendedPointReader::SINGLE, vertAttribs );
	
	mesh.reset();
	if( m_renderer )
	{
		delete m_renderer;
	}
	// Render the scene one time, traveling from octree's root to init m_renderTime for future projection
	// threshold adaptations.
	m_renderer = new TucanoRenderingState( camera, light_trackball, mesh, vertAttribs,
											QApplication::applicationDirPath().toStdString() +
											"/shaders/tucano/" );
	clock_t timing = clock();
	m_octree->traverse( *m_renderer, m_projThresh );
	timing = clock() - timing;
	m_renderTime = float( timing ) / CLOCKS_PER_SEC * 1000;
}