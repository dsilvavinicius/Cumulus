#include <Frustum.h>

#include <gtest/gtest.h>

namespace model
{
	namespace test
	{
        class FrustumTest : public ::testing::Test
		{
		protected:
			void SetUp() {}
		};

		TEST_F( FrustumTest, CullingProjectionOnly )
		{
			// Default OpenGL camera is at origin, pointing to the negative z axis. Thus, model-view is only a reflection
			// of the x and z axes.
			Matrix4f modelView;
			modelView << -1.f, 0.f,  0.f,  0.f,
						  0.f, 1.f,  1.f,  0.f,
						  0.f, 0.f, -1.f,  0.f,
						  0.f, 0.f,  0.f, -1.f;
			
			// This projection matrix represents a frustum with planes in the following coordinates in the related view space axis:
			// left: -0.5, right: 0.5, bottom: -0.5, top: 0.5, near: 1, far: 10.
			Matrix4f proj;
			proj << 2.f, 0.f, 0.f   , 0.f   ,
					0.f, 2.f, 0.f   , 0.f   ,
					0.f, 0.f, -1.22f, -2.22f,
					0.f, 0.f, -1.0f , 0.f;
			
			Frustum frustum( modelView , proj );
			// 1st case: outside box.
			AlignedBox< float, 3 > outsideBox( Vector3f( 0.6f, -0.1f, -0.45f ), Vector3f( 3.f, 0.2f, -0.2f ) ); 
			ASSERT_TRUE( frustum.isCullable( outsideBox ) );
			
			// 2nd case: intersecting box.
			AlignedBox< float, 3 > intersectingBox( Vector3f( -3.f, 0.f, -5.f ), Vector3f( 0.f, 0.25f, 0.f ) ); 
			ASSERT_FALSE( frustum.isCullable( intersectingBox ) );
			
			// 3rd case: inside box.
			AlignedBox< float, 3 > insideBox( Vector3f( 0.3f, 0.1f, -9.5f ), Vector3f( 0.4f, 0.4f, -8.f ) ); 
			ASSERT_FALSE( frustum.isCullable( insideBox ) );
		}
		
		TEST_F( FrustumTest, CullingModelViewProjection )
		{
		}
	}
}