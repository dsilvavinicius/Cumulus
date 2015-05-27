#include <gtest/gtest.h>
#include "SQLiteManager.h"
#include "MortonCode.h"

namespace model
{
	namespace test
	{
        class SQLiteManagerTest
        : public ::testing::Test
		{
		protected:
			void SetUp(){}
		};
		
		TEST_F( SQLiteManagerTest, InsertAndGetPoints )
		{
			using Point = model::Point< float, vec3 >;
			using OctreeNode = model::ShallowOctreeNode< float, vec3 >;
			using SQLiteManager = util::SQLiteManager< Point, ShallowMortonCode, OctreeNode >;
			
			Point p0( vec3( 11.321565f, 4.658535f, 7.163479f ), vec3( 7.163479f, 4.658535f, 11.321565f ) );
			Point p1( vec3( 11.201763f, 5.635769f, 6.996898f ), vec3( 6.996898f, 5.635769f, 11.201763f ) );
			Point p2( vec3( 11.198129f, 4.750132f, 7.202037f ), vec3( 7.202037f, 4.750132f, 11.198129f ) );
			
			SQLiteManager sqLite;
			sqLite.insertPoint( p0 );
			sqLite.insertPoint( p1 );
			sqLite.insertPoint( p2 );
			
			float epsilon = 1.e-15;
			Point p = sqLite.getPoint( 0 );
			ASSERT_TRUE( p.equal( p0, epsilon ) );
			
			p = sqLite.getPoint( 1 );
			ASSERT_TRUE( p.equal( p1, epsilon ) );
			
			p = sqLite.getPoint( 2 ); 
			ASSERT_TRUE( p.equal( p2, epsilon ) );
		}
		
		TEST_F( SQLiteManagerTest, InsertAndGetNodes )
		{
			using Point = model::Point< float, vec3 >;
			using MortonCode = model::ShallowMortonCode;
			using Contents = vector< int >;
			using OctreeNode = model::ShallowOctreeNode< float, vec3 >;
			using LeafNode = model::ShallowLeafNode< float, vec3, Contents >;
			using InnerNode = model::ShallowInnerNode< float, vec3, Contents >;
			using SQLiteManager = util::SQLiteManager< Point, MortonCode, OctreeNode >;
			
			MortonCode leafCode;
			leafCode.build( 2, 3, 4, 5 );
			LeafNode leafNode;
			Contents leafPoints = { 5, 4, 3, 2 };
			leafNode.setContents( leafPoints );
			
			SQLiteManager sqLite;
			cout << "Inserting first node." << endl;
			sqLite.insertNode< Contents >( leafCode, leafNode );
			
			MortonCode innerCode;
			innerCode.build( 1, 2, 3, 4 );
			InnerNode innerNode;
			Contents innerPoints = { 4, 3, 2, 1 };
			innerNode.setContents( innerPoints );
			
			cout << "Inserting second node." << endl;
			sqLite.insertNode< Contents >( innerCode, innerNode );
			
			cout << "Quering first node." << endl;
			OctreeNode* queriedNode = sqLite.getNode< Contents >( leafCode );
			ASSERT_EQ( *queriedNode->getContents< Contents >(), leafPoints );
			delete queriedNode;
			
			cout << "Quering second node." << endl;
			queriedNode = sqLite.getNode< Contents >( innerCode );
			ASSERT_EQ( *queriedNode->getContents< Contents >(), innerPoints );
			delete queriedNode;
		}
		
		/*
		TEST_F( SQLiteManagerTest, InsertAndGetExtendedPoints )
		{
			using Point = ExtendedPoint< float, vec3 >;
			using OctreeNode = ShallowOctreeNode< float, vec3 >;
			
			ShallowOutOfCoreOctree< float, vec3, Point>  octree( 1, 10 );
			octree.buildFromFile( g_appPath + "/data/test_extended_points.ply", ExtendedPointReader::SINGLE,
								  COLORS_AND_NORMALS );
			
			float epsilon = 1.e-15;
			
			SQLiteManager< Point, ShallowMortonCode, OctreeNode >& sqLite = octree.getSQLiteManager();
			
			Point p = sqLite.getPoint( 0 );
			Point expected( vec3( 0.003921569f, 0.007843137f, 0.011764706f ), vec3( 11.321565f, 4.658535f, 7.163479f ),
							vec3( 7.163479f, 4.658535f, 11.321565f ) );
			ASSERT_TRUE( p.equal( expected, epsilon ) );
			
			p = sqLite.getPoint( 1 );
			expected = Point( vec3( 0.015686275f, 0.019607843f, 0.023529412f ), vec3( 11.201763f, 5.635769f, 6.996898f ),
							  vec3( 6.996898f, 5.635769f, 11.201763f ) ); 
			ASSERT_TRUE( p.equal( expected, epsilon ) );
			
			p = sqLite.getPoint( 2 );
			expected = Point( vec3( 0.02745098f, 0.031372549f, 0.035294118f ), vec3( 11.198129f, 4.750132f, 7.202037f ),
							  vec3( 7.202037f, 4.750132f, 11.198129f ) ); 
			ASSERT_TRUE( p.equal( expected, epsilon ) );
		}*/
	}
}