#include <gtest/gtest.h>
#include <QApplication>

#include "HierarchyTestMethods.h"
#include "Octree.h"
#include <IndexedOctree.h>
#include <OutOfCoreOctree.h>
#include "Stream.h"

extern "C" string g_appPath;

namespace model
{
	namespace test
	{
		void generatePoints( PointVector& points )
		{
			// These points should define the boundaries of the octree hexahedron.
			PointPtr up( new Point( vec3( 0.01f, 0.02f, 0.03f ), vec3( 1.f, 15.f ,2.f ) ) );
			PointPtr down( new Point( vec3( 0.04f, 0.05f, 0.06f ), vec3( 3.f, -31.f ,4.f ) ) );
			PointPtr left( new Point( vec3( 0.07f, 0.08f, 0.09f ), vec3( -14.f, 5.f ,6.f ) ) );
			PointPtr right( new Point( vec3( 0.1f, 0.11f, 0.12f ), vec3( 46.f, 7.f ,8.f ) ) );
			PointPtr front( new Point( vec3( 0.13f, 0.14f, 0.15f ), vec3( 9.f, 10.f ,24.f ) ) );
			PointPtr back( new Point( vec3( 0.16f, 0.17f, 0.18f ), vec3( 11.f, 12.f ,-51.f ) ) );
			
			// Additional points inside the hexahedron.
			PointPtr addPoint0( new Point( vec3( 0.19f, 0.2f, 0.21f ), vec3( 13.f, -12.f, 9.f ) ) );
			PointPtr addPoint1( new Point( vec3( 0.22f, 0.23f, 0.24f ), vec3( -5.f, -8.f, 1.f ) ) );
			PointPtr addPoint2( new Point( vec3( 0.25f, 0.26f, 0.27f ), vec3( 14.f, 11.f, -4.f ) ) );
			PointPtr addPoint3( new Point( vec3( 0.28f, 0.29f, 0.30f ), vec3( 7.f, 3.f, -12.f ) ) );
			PointPtr addPoint4( new Point( vec3( 0.31f, 0.32f, 0.33f ), vec3( 12.f, 5.f, 0.f ) ) );
			
			points.push_back( back );
			points.push_back( front );
			points.push_back( right );
			points.push_back( left );
			points.push_back( down );
			points.push_back( up );
			
			points.push_back( addPoint0 );
			points.push_back( addPoint1 );
			points.push_back( addPoint2 );
			points.push_back( addPoint3 );
			points.push_back( addPoint4 );
		}
		
		void generatePoints( ExtendedPointVector& points )
		{
			using Point = ExtendedPoint;
			using PointPtr = ExtendedPointPtr;
			using PointVector = ExtendedPointVector;
			
			// These points should define the boundaries of the octree hexahedron.
			PointPtr up( new Point( vec3( 0.01f, 0.02f, 0.03f ), vec3( 0.01f, 0.02f, 0.03f ), vec3( 1.f, 15.f ,2.f ) ) );
			PointPtr down( new Point( vec3( 0.04f, 0.05f, 0.06f ), vec3( 0.04f, 0.05f, 0.06f ), vec3( 3.f, -31.f ,4.f ) ) );
			PointPtr left( new Point( vec3( 0.07f, 0.08f, 0.09f ), vec3( 0.07f, 0.08f, 0.09f ), vec3( -14.f, 5.f ,6.f ) ) );
			PointPtr right( new Point( vec3( 0.1f, 0.11f, 0.12f ), vec3( 0.1f, 0.11f, 0.12f ), vec3( 46.f, 7.f ,8.f ) ) );
			PointPtr front( new Point( vec3( 0.13f, 0.14f, 0.15f ), vec3( 0.13f, 0.14f, 0.15f ), vec3( 9.f, 10.f ,24.f ) ) );
			PointPtr back( new Point( vec3( 0.16f, 0.17f, 0.18f ), vec3( 0.16f, 0.17f, 0.18f ), vec3( 11.f, 12.f ,-51.f ) ) );
			
			// Additional points inside the hexahedron.
			PointPtr addPoint0( new Point( vec3( 0.19f, 0.2f, 0.21f ), vec3( 0.19f, 0.2f, 0.21f ), vec3( 13.f, -12.f, 9.f ) ) );
			PointPtr addPoint1( new Point( vec3( 0.22f, 0.23f, 0.24f ), vec3( 0.22f, 0.23f, 0.24f ), vec3( -5.f, -8.f, 1.f ) ) );
			PointPtr addPoint2( new Point( vec3( 0.25f, 0.26f, 0.27f ), vec3( 0.25f, 0.26f, 0.27f ), vec3( 14.f, 11.f, -4.f ) ) );
			PointPtr addPoint3( new Point( vec3( 0.28f, 0.29f, 0.30f ), vec3( 0.28f, 0.29f, 0.30f ), vec3( 7.f, 3.f, -12.f ) ) );
			PointPtr addPoint4( new Point( vec3( 0.31f, 0.32f, 0.33f ), vec3( 0.31f, 0.32f, 0.33f ), vec3( 12.f, 5.f, 0.f ) ) );
			
			points.push_back( back );
			points.push_back( front );
			points.push_back( right );
			points.push_back( left );
			points.push_back( down );
			points.push_back( up );
			
			points.push_back( addPoint0 );
			points.push_back( addPoint1 );
			points.push_back( addPoint2 );
			points.push_back( addPoint3 );
			points.push_back( addPoint4 );
		}
		
		class SimplePointTest
		: public ::testing::Test
		{
		public:
			static void SetUpTestCase()
			{
				m_plyFileName = new string( g_appPath + "/data/simple_point_octree.ply" );
			}
		
			static void TearDownTestCase()
			{
				delete m_plyFileName;
				m_plyFileName = nullptr;
			}
			
			static string* m_plyFileName;
		};
		string* SimplePointTest::m_plyFileName;
		
		class ExtendedPointTest
		: public ::testing::Test
		{
		public:
			static void SetUpTestCase()
			{
				m_plyFileName = new string( g_appPath + "/data/extended_point_octree.ply" );
			}
			
			static void TearDownTestCase()
			{
				delete m_plyFileName;
				m_plyFileName = nullptr;
			}
			
			static string* m_plyFileName;
		};
		string* ExtendedPointTest::m_plyFileName;
		
		template< typename P >
        class OctreeTest {};
		
		template<>
		class OctreeTest< ShallowOctree >
		: public SimplePointTest
		{
			using Octree = ShallowOctree;
			using OctreePtr = ShallowOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
			
		protected:
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree; 
		};
		
		template<>
		class OctreeTest< MediumOctree >
		: public SimplePointTest
		{
			using Octree = MediumOctree;
			using OctreePtr = MediumOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
			
		protected:
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree; 
		};
		
		template<>
		class OctreeTest< ShallowExtOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = ShallowExtOctree;
			using OctreePtr = ShallowExtOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumExtOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = MediumExtOctree;
			using OctreePtr = MediumExtOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowRandomSampleOctree >
		: public SimplePointTest
		{
			using Octree = ShallowRandomSampleOctree;
			using OctreePtr = ShallowRandomSampleOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumRandomSampleOctree >
		: public SimplePointTest
		{
			using Octree = MediumRandomSampleOctree;
			using OctreePtr = MediumRandomSampleOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowExtRandomSampleOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = ShallowExtRandomSampleOctree;
			using OctreePtr = ShallowExtRandomSampleOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumExtRandomSampleOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = MediumExtRandomSampleOctree;
			using OctreePtr = MediumExtRandomSampleOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowIndexedOctree >
		: public SimplePointTest
		{
			using Octree = ShallowIndexedOctree;
			using OctreePtr = ShallowIndexedOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumIndexedOctree >
		: public SimplePointTest
		{
			using Octree = MediumIndexedOctree;
			using OctreePtr = MediumIndexedOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowExtIndexedOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = ShallowExtIndexedOctree;
			using OctreePtr = ShallowExtIndexedOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumExtIndexedOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = MediumExtIndexedOctree;
			using OctreePtr = MediumExtIndexedOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowFrontOctree >
		: public SimplePointTest
		{
			using Octree = ShallowFrontOctree;
			using OctreePtr = ShallowFrontOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumFrontOctree >
		: public SimplePointTest
		{
			using Octree = MediumFrontOctree;
			using OctreePtr = MediumFrontOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowExtFrontOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = ShallowExtFrontOctree;
			using OctreePtr = ShallowExtFrontOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 10 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumExtFrontOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = MediumExtFrontOctree;
			using OctreePtr = MediumExtFrontOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				PointVector points;
				generatePoints( points );
				m_octree = OctreePtr( new Octree( 1, 20 ) );
				m_octree->build( points );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowOutOfCoreOctree >
		: public SimplePointTest
		{
			using Octree = ShallowOutOfCoreOctree;
			using OctreePtr = ShallowOutOfCoreOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				m_octree = OctreePtr( new Octree( 1, 10, g_appPath + "/Octree.db" ) );
				m_octree->buildFromFile( *m_plyFileName, SimplePointReader::SINGLE, Attributes::COLORS );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumOutOfCoreOctree >
		: public SimplePointTest
		{
			using Octree = MediumOutOfCoreOctree;
			using OctreePtr = MediumOutOfCoreOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				m_octree = OctreePtr( new Octree( 1, 20, g_appPath + "/Octree.db" ) );
				m_octree->buildFromFile( *m_plyFileName, SimplePointReader::SINGLE, Attributes::COLORS );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< ShallowExtOutOfCoreOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = ShallowExtOutOfCoreOctree;
			using OctreePtr = ShallowExtOutOfCoreOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				m_octree = OctreePtr( new Octree( 1, 10, g_appPath + "/Octree.db" ) );
				m_octree->buildFromFile( *m_plyFileName, ExtendedPointReader::SINGLE, Attributes::COLORS_AND_NORMALS );
			}
			
			OctreePtr m_octree;
		};
		
		template<>
		class OctreeTest< MediumExtOutOfCoreOctree >
		: public ExtendedPointTest
		{
			using Point = ExtendedPoint;
			using PointVector = ExtendedPointVector;
			using Octree = MediumExtOutOfCoreOctree;
			using OctreePtr = MediumExtOutOfCoreOctreePtr;
			using Test = model::test::OctreeTest< Octree >;
		
		protected:
			/** Creates points that will be inside the octree and the associated expected results of octree construction. */
			void SetUp()
			{
				m_octree = OctreePtr( new Octree( 1, 20, g_appPath + "/Octree.db" ) );
				m_octree->buildFromFile( *m_plyFileName, ExtendedPointReader::SINGLE, Attributes::COLORS_AND_NORMALS );
			}
			
			OctreePtr m_octree;
		};
		
		void testBoundaries( const ShallowOctree& octree )
		{
			testShallowBoundaries( octree );
		}
		
		void testHierarchy( const ShallowOctree& octree )
		{
			checkHierarchy( octree.getHierarchy() );
		}
		
		void testBoundaries( const MediumOctree& octree )
		{
			testMediumBoundaries( octree );
		}
		
		void testHierarchy( const MediumOctree& octree )
		{
			checkHierarchy( octree.getHierarchy() );
		}
		
		void testBoundaries( const ShallowExtOctree& octree )
		{
			testShallowBoundaries( octree );
		}
		
		void testHierarchy( const ShallowExtOctree& octree )
		{
			checkHierarchy( octree.getHierarchy() );
		}
		
		void testBoundaries( const MediumExtOctree& octree )
		{
			testMediumBoundaries( octree );
		}
		
		void testHierarchy( const MediumExtOctree& octree )
		{
			checkHierarchy( octree.getHierarchy() );
		}
		
		using testing::Types;
		
		typedef Types< 	ShallowOctree, ShallowExtOctree, MediumOctree, MediumExtOctree, ShallowRandomSampleOctree,
						ShallowExtRandomSampleOctree, MediumRandomSampleOctree, MediumExtRandomSampleOctree,
						ShallowIndexedOctree, ShallowExtIndexedOctree, MediumIndexedOctree, MediumExtIndexedOctree,
						ShallowFrontOctree, ShallowExtFrontOctree, MediumFrontOctree, MediumExtFrontOctree,
						ShallowOutOfCoreOctree, ShallowExtOutOfCoreOctree, MediumOutOfCoreOctree,
						MediumExtOutOfCoreOctree > Implementations;
		
		TYPED_TEST_CASE( OctreeTest, Implementations );

		/** Tests the calculated boundaries of the ShallowOctree. */
		TYPED_TEST( OctreeTest, Boundaries )
		{
			testBoundaries( *this->m_octree );
		}

		/** Tests the ShallowOctree created hierarchy. */
		TYPED_TEST( OctreeTest, Hierarchy )
		{
			testHierarchy( *this->m_octree );
		}
	}
}