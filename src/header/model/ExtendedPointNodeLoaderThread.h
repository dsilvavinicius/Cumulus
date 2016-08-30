#ifndef EXTENDED_NODE_LOADER_THREAD_H
#define EXTENDED_NODE_LOADER_THREAD_H

#include <QThread>
#include <utils/qtfreecamerawidget.hpp>
#include "TbbAllocator.h"
#include "ExtendedPoint.h"
#include "O1OctreeNode.h"

namespace model
{
	class ExtendedPointNodeLoaderThread
	: public QThread
	{
		Q_OBJECT
	
	public:
		using Point = model::ExtendedPoint;
		using PointPtr = shared_ptr< Point >;
		using Alloc = TbbAllocator< Point >;
		using Node = O1OctreeNode< PointPtr >;
		using Siblings = Array< Node >;
		using NodePtrList = list< Node*, typename Alloc:: template rebind< Node* >::other >;
		using NodePtrListArray = Array< NodePtrList, typename Alloc:: template rebind< NodePtrList >::other >;
		
		using SiblingsList = list< Siblings, typename Alloc:: template rebind< Siblings >::other >;
		using SiblingsListArray = Array< SiblingsList, typename Alloc:: template rebind< SiblingsList >::other >;
		
		ExtendedPointNodeLoaderThread( QGLWidget* widget, const ulong gpuMemQuota );
		
		void pushRequests( NodePtrList& load, NodePtrList& unload, SiblingsList& release );
		
		bool reachedGpuMemQuota();
		
		const QGLWidget* widget();
		
	protected:
		void run() Q_DECL_OVERRIDE;
		
	private:
		void load( Node& node );
		void unload( Node& node );
		void release( Siblings& siblings );
		
		NodePtrList m_load;
		NodePtrList m_unload;
		SiblingsList m_release;
		
		mutex m_mutex;
		
		atomic_ulong m_availableGpuMem;
		ulong m_totalGpuMem;
		
		QGLWidget* m_widget;
	};
	
	inline ExtendedPointNodeLoaderThread::ExtendedPointNodeLoaderThread( QGLWidget* widget, const ulong gpuMemQuota )
	: QThread( widget ),
	m_widget( widget ),
	m_availableGpuMem( gpuMemQuota ),
	m_totalGpuMem( gpuMemQuota )
	{
		m_widget->doneCurrent();
		m_widget->context()->moveToThread( this );
	}
	
	inline void ExtendedPointNodeLoaderThread::pushRequests( NodePtrList& load, NodePtrList& unload, SiblingsList& release )
	{
		lock_guard< mutex > lock( m_mutex );
		m_load.splice( m_load.end(), load );
		m_unload.splice( m_unload.end(), unload );
		m_release.splice( m_release.end(), release );
		
		start();
	}
	
	inline bool ExtendedPointNodeLoaderThread::reachedGpuMemQuota()
	{
		return float( m_availableGpuMem ) < 0.05f * float( m_totalGpuMem );
	}
	
	inline const QGLWidget* ExtendedPointNodeLoaderThread::widget()
	{
		return m_widget;
	}
	
	inline void ExtendedPointNodeLoaderThread::run()
	{
		NodePtrList loadList;
		NodePtrList unloadList;
		SiblingsList releaseList;
		{
			lock_guard< mutex > lock( m_mutex );
			loadList.splice( loadList.end(), m_load );
			unloadList.splice( unloadList.end(), m_unload );
			releaseList.splice( releaseList.end(), m_release );
		}
		
		for( Node* node : loadList )
		{
			load( *node );
		}
		
		for( Node* node : unloadList )
		{
			unload( *node );
		}
		
		for( Siblings& siblings : releaseList )
		{
			release( siblings );
		}
	}
	
	inline void ExtendedPointNodeLoaderThread::load( Node& node )
	{
		Array< PointPtr > points = node.getContents();
		
		ulong neededGpuMem = 11 * sizeof( float ) * points.size(); // 4 floats for positions, 4 colors and 3 for normals.
		
		if( neededGpuMem <= m_availableGpuMem )
		{
			vector< Eigen::Vector4f > positions;
			vector< Eigen::Vector3f > normals;
			vector< Eigen::Vector4f > colors;
			
			for( ExtendedPointPtr point : points )
			{
				const Vec3& pos = point->getPos();
				positions.push_back( Vector4f( pos.x(), pos.y(), pos.z(), 1.f ) );
				
				normals.push_back( point->getNormal() );
				
				const Vec3& color = point->getColor();
				colors.push_back( Vector4f( color.x(), color.y(), color.z(), 1.f ) );
			}
			
			Mesh& mesh = node.mesh();
			mesh.selectPrimitive( Mesh::POINT );
			mesh.loadVertices( positions );
			mesh.loadNormals( normals );
			mesh.loadColors( colors );
			node.setLoadState( Node::LOADED );
			
			m_availableGpuMem -= neededGpuMem;
		}
	}
	
	inline void ExtendedPointNodeLoaderThread::unload( Node& node )
	{
		for( Node& node : node.child() )
		{
			if( node.loadState() == Node::LOADED )
			{
				unload( node ); 
			}
		}
		
		m_availableGpuMem += 11 * sizeof( float ) * node.getContents().size();
		node.mesh().reset();
	}
	
	inline void ExtendedPointNodeLoaderThread::release( Siblings& siblings )
	{
		for( Node& sibling : siblings )
		{
			Siblings& children = sibling.child();
			if( !children.empty() )
			{
				release( children );
			}
		}
		
		siblings.clear();
	}
}

#endif