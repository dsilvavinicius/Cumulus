#include <gtest/gtest.h>
#include "FastParallelOctree.h"
#include <GLHiddenWidget.h>
#include "FastParallelOctreeTestParam.h"

extern "C" FastParallelOctreeTestParam g_fastParallelStressParam;

namespace model
{
	namespace test
	{
		TEST( FastParallelOctreeStressTest, DISABLED_Stress )
		{
			using Morton = MediumMortonCode;
			using Octree = FastParallelOctree< Morton >;
			using OctreeDim = Octree::Dim;
			using Sql = SQLiteManager< Point, Morton, Octree::Node >;
			using NodeArray = typename Sql::NodeArray;
			
			ASSERT_EQ( 0, AllocStatistics::totalAllocated() );
			
			{
				ofstream m_log( "FastParallelOctreeStressTest.log", ios_base::app );
				
				FastParallelOctreeTestParam param = g_fastParallelStressParam;
				m_log << "Params: " << param << endl;
				auto start = Profiler::now( "Octree construction", m_log );
				
				GLHiddenWidget hiddenWidget;
				NodeLoaderThread loaderThread( &hiddenWidget, 900ul * 1024ul * 1024ul );
				NodeLoader< Point > loader( &loaderThread, 1 );
				
				Octree octree( param.m_plyFilename, param.m_hierarchyLvl, loader,
							   RuntimeSetup( param.m_nThreads, param.m_workItemSize, param.m_memoryQuota ) );
				
				Profiler::elapsedTime( start, "Octree construction", m_log );
				
				#ifdef HIERARCHY_STATS
					m_log << "Processed nodes: " << octree.m_processedNodes << endl << endl;
				#endif
			}
			
			ASSERT_EQ( 0, AllocStatistics::totalAllocated() );
		}
	}
}