#include "omicron/basic/morton_code.h"
#include "omicron/disk/octree_file.h"
#include "omicron/hierarchy/octree_dim_calculator.h"

#include <gtest/gtest.h>

namespace omicron::test::disk
{
    using namespace basic;
    using namespace hierarchy;
    
    using Morton = ShallowMortonCode;
    using Node = OctreeFile<Morton>::Node;
    using NodePtr = OctreeFile<Morton>::NodePtr;

    class OctreeFileTest : public ::testing::Test
    {
    protected:
        void SetUp() {}
    };

    void checkHierarchy(const NodePtr& rootPtr, const Surfel& rootSurfel, const Surfel& childSurfel0, const Surfel& childSurfel1, const Surfel& grandChildSurfel0, const Surfel& grandChildSurfel1)
    {
        ASSERT_EQ( rootPtr->parent(), nullptr );
        ASSERT_EQ( rootPtr->getContents().size(), 1 );
        ASSERT_EQ( rootPtr->getContents()[ 0 ], rootSurfel );
        ASSERT_EQ( rootPtr->isLeaf(), false );
        ASSERT_EQ( rootPtr->child().size(), 1 );
        
        Node& child = rootPtr->child()[ 0 ];
        
        ASSERT_EQ( child.parent(), rootPtr.get() );
        ASSERT_EQ( child.getContents().size(), 2 );
        ASSERT_EQ( child.getContents()[ 0 ], childSurfel0 );
        ASSERT_EQ( child.getContents()[ 1 ], childSurfel1 );
        ASSERT_EQ( child.isLeaf(), false );
        ASSERT_EQ( child.child().size(), 1 );
        
        Node& grandChild = child.child()[ 0 ];
        
        ASSERT_EQ( grandChild.parent(), &child );
        ASSERT_EQ( grandChild.getContents().size(), 2 );
        ASSERT_EQ( grandChild.getContents()[ 0 ], grandChildSurfel0 );
        ASSERT_EQ( grandChild.getContents()[ 1 ], grandChildSurfel1 );
        ASSERT_EQ( grandChild.isLeaf(), true );
        ASSERT_EQ( grandChild.child().size(), 0 );
    }

    TEST_F( OctreeFileTest, WriteAndRead )
    {
        Surfel rootSurfel( Vec3( 0.0f, 0.1f, 0.2f ), Vec3( 0.3f, 0.4f, 0.5f ), Vec3( 0.6f, 0.7f, 0.8f ) );
        
        Surfel childSurfel0( Vec3( 1.0f, 1.1f, 1.2f ), Vec3( 1.3f, 1.4f, 1.5f ), Vec3( 1.6f, 1.7f, 1.8f ) );
        Surfel childSurfel1( Vec3( 1.9f, 1.10f, 1.11f ), Vec3( 1.12f, 1.13f, 1.14f ), Vec3( 1.15f, 1.16f, 1.17f ) );
        
        Surfel grandChildSurfel0( Vec3( 2.0f, 2.1f, 2.2f ), Vec3( 2.3f, 2.4f, 2.5f ), Vec3( 2.6f, 2.7f, 2.8f ) );
        Surfel grandChildSurfel1( Vec3( 2.9f, 2.10f, 2.11f ), Vec3( 2.12f, 2.13f, 2.14f ), Vec3( 2.15f, 2.16f, 2.17f ) );
        
        Node root( Array< Surfel >( 1, rootSurfel ), false );
        {
            Array< Surfel > childSurfels( 2 );
            childSurfels[ 0 ] = childSurfel0;
            childSurfels[ 1 ] = childSurfel1;
            
            Node child( childSurfels, false );
            
            Array< Surfel > grandChildSurfels( 2 );
            grandChildSurfels[ 0 ] = grandChildSurfel0;
            grandChildSurfels[ 1 ] = grandChildSurfel1;
            Node grandChild( grandChildSurfels, true );
            
            Array< Node > childChildren( 1 );
            childChildren[ 0 ] = std::move( grandChild );
            child.setChildren( std::move( childChildren ) );
            
            Array< Node > rootChildren( 1 );
            rootChildren[ 0 ] = std::move( child );
            root.setChildren( std::move( rootChildren ) );
        }
        
        // First case: depth-first order.
        {
            OctreeFile<Morton> octFile;
            octFile.writeDepth( "test_octree_depth.boc", root );
            NodePtr rootPtr = octFile.read( "test_octree_depth.boc" );
            checkHierarchy(rootPtr, rootSurfel, childSurfel0, childSurfel1, grandChildSurfel0, grandChildSurfel1);
        }
        
        cout << "Depth-first order test passed." << endl << endl;

        // Second case: breadth-first order.
        {
            OctreeFile<Morton> octFile;
            octFile.writeBreadth( "test_octree_breadth.boc", root );
            NodePtr rootPtr = octFile.read( "test_octree_breadth.boc" );
            checkHierarchy(rootPtr, rootSurfel, childSurfel0, childSurfel1, grandChildSurfel0, grandChildSurfel1);
        }

        cout << "Breadth-first order test passed." << endl << endl;

        // Third case: async breadth-first oder.
        {
            OctreeFile<Morton> octFile;
            octFile.writeBreadth( "test_octree_breadth.boc", root );
            
            OctreeDimCalculator<Morton> octreeDimCalc;
            octreeDimCalc.insertPoint(Point(Vec3(1.f, 0.f, 0.f), rootSurfel.c));
            octreeDimCalc.insertPoint(Point(Vec3(1.f, 0.f, 0.f), childSurfel0.c));
            octreeDimCalc.insertPoint(Point(Vec3(1.f, 0.f, 0.f), childSurfel1.c));
            octreeDimCalc.insertPoint(Point(Vec3(1.f, 0.f, 0.f), grandChildSurfel0.c));
            octreeDimCalc.insertPoint(Point(Vec3(1.f, 0.f, 0.f), grandChildSurfel1.c));
            DimOriginScale<Morton> dimOriginScale = octreeDimCalc.dimensions(3);
            
            NodePtr root = octFile.asyncRead( "test_octree_breadth.boc", dimOriginScale.dimensions() );
            octFile.waitAsyncRead();

            checkHierarchy(root, rootSurfel, childSurfel0, childSurfel1, grandChildSurfel0, grandChildSurfel1);
        }

        cout << "Async breadth-first order test passed." << endl << endl;
    }
}
