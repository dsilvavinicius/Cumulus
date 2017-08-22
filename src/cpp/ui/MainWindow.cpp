#include <QFileDialog>
#include "MainWindow.h"
#include "GLHiddenWidget.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	
	auto hiddenWidget = new GLHiddenWidget( ui->centralWidget );
	hiddenWidget->setVisible( false );
	auto loaderThread = new NodeLoaderThread( hiddenWidget, 900ul * 1024ul * 1024ul );
	m_loader = new NodeLoader( loaderThread, 1 );
	
	m_pointRenderWidget = new PointRendererWidget( *m_loader, ui->centralWidget );
	m_pointRenderWidget->setObjectName(QStringLiteral("pointRendererWidget"));
	QSizePolicy sizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
	sizePolicy.setHorizontalStretch( 0 );
	sizePolicy.setVerticalStretch( 0 );
	sizePolicy.setHeightForWidth( m_pointRenderWidget->sizePolicy().hasHeightForWidth() );
	m_pointRenderWidget->setSizePolicy(sizePolicy);

	ui->horizontaLayout->insertWidget( 0, m_pointRenderWidget );
}

MainWindow::~MainWindow()
{
    delete ui;
	delete m_loader;
}

void MainWindow::initialize()
{
    m_pointRenderWidget->initialize( ui->sld_frame_rate->value(), ui->sld_frame_tolerance->value() );
    
	connect( ui->bt_save_octree, &QPushButton::clicked, m_pointRenderWidget, &PointRendererWidget::saveOctree );
    
	connect( ui->sld_frame_rate, &QSlider::valueChanged, m_pointRenderWidget, &PointRendererWidget::setFrameRate );
	
	connect( ui->sld_frame_tolerance, &QSlider::valueChanged, m_pointRenderWidget,
			 &PointRendererWidget::setRenderingTimeTolerance );
    
	connect( ui->check_trackball, &QCheckBox::stateChanged, m_pointRenderWidget, &PointRendererWidget::toggleDrawTrackball );
	
	connect( ui->ckbx_draw_viewports, &QCheckBox::stateChanged, m_pointRenderWidget,
			 &PointRendererWidget::toggleDrawAuxViewports );
	
	connect( ui->bt_draw_node_debug, &QCheckBox::stateChanged, m_pointRenderWidget,
			 &PointRendererWidget::toggleNodeDebugDraw );
	
	connect( m_pointRenderWidget, &PointRendererWidget::debugInfoDefined, ui->debug_info, &QTextBrowser::setText);
}

void MainWindow::on_bt_open_cloud_clicked()
{
	QFileDialog dialog;
	dialog.setFileMode( QFileDialog::ExistingFile );
	dialog.setNameFilter( "Octree files or point files (*.oct *.ply);;Octree files (*.oct);;Point files (*.ply)" );
	dialog.setDirectory( QDir::currentPath().append( "/../../src/data/real/" ) );
	
	if( dialog.exec() )
	{
		QString filename = dialog.selectedFiles().first();
		m_pointRenderWidget->openMesh( filename.toStdString() );
	}
}

void MainWindow::keyPressEvent(QKeyEvent *ke)
{
    int key = ke->key();
    int modifiers = ke->modifiers();

    if (modifiers == 0 && key == Qt::Key_Escape)
    {
        close();
    }

    ke->accept();
}

void MainWindow::closeEvent( QCloseEvent * event )
{
	m_pointRenderWidget->close();
	event->accept();
}
