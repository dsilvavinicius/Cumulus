#ifndef TUCANO_RENDERING_STATE_H
#define TUCANO_RENDERING_STATE_H

#include <tucano.hpp>
#include <phongshader.hpp>
#include <imgSpacePBR.hpp>
#include "RenderingState.h"
#include "TextEffect.h"
#include <utils/frustum.hpp>

using namespace Tucano;
using namespace Effects;

namespace model
{
	/** RenderingState using Tucano library ( @link http://terra.lcg.ufrj.br/tucano/ ). */
	class TucanoRenderingState
	: public RenderingState
	{
	public:
		enum Effect
		{
			PHONG,
			JUMP_FLOODING
		};
		
		TucanoRenderingState( Camera* camera, Camera* lightCam , Mesh* mesh, const string& shaderPath,
							  uint maxOctreeDepth, const int& jfpbrFrameskip = 1, const Effect& effect = PHONG );
		
		~TucanoRenderingState();
		
		/** Updates the frustum after changes on camera. */
		void updateFrustum();
		
		/** This implementation clears up color and depth buffers, the attributes of the vertices and updates the viewing
		 * frustum data. */
		virtual void setupRendering() override;
		
		virtual unsigned int render();
		
		virtual bool isCullable( const AlignedBox3f& box ) const override;
		
		/** This implementation will compare the size of the maximum box diagonal in window coordinates with the projection
		 *	threshold.
		 *	@param projThresh is the threshold of the squared size of the maximum box diagonal in window coordinates. */
		virtual bool isRenderable( const AlignedBox3f& box, const Float projThresh ) const override;
		
		/** Rendering test based on distance.
		 * @param coarsestLoDSqrDistance is the normalized squared distance where the model wll be rendered with the
		 * coarsest level of detail. */
		bool isRenderableByDistance( const AlignedBox3f& box, uint nodeLvl, const Float coarsestLoDSqrDistance ) const
		override;
	
		virtual void renderText( const Vec3& pos, const string& str );
		
		/** Gets the image space pbr effect. The caller is reponsable for the correct usage.*/
		ImgSpacePBR& getJumpFlooding() { return *m_jfpbr; }
		
		/** Gets the phong effect. The caller is reponsable for the correct usage.*/
		Phong& getPhong() { return *m_phong; }
		
		/** Changes the effect used to render the points. */
		void selectEffect( const Effect& effect ) { m_effect = effect; }
		
		void setJfpbrFrameskip( const int& value ) { m_jfpbrFrameskip = value; }
		
		void clearAttribs()
		{
			RenderingState::clearAttribs();
			m_mesh->reset();
		}
		
	protected:
		/** Acquires current traball's view-projection matrix. */
		Matrix4f getViewProjection() const;
		
		/** Projects the point in world coordinates to window coordinates. */
		Vector2f projToWindowCoords( const Vector4f& point, const Matrix4f& viewProjection, const Vector2i& viewportSize )
		const;
		
		Frustum* m_frustum;
		Camera* m_camera;
		Camera* m_lightCamera;
		
		Matrix4f m_viewProj;
		
		Mesh* m_mesh;
		Phong* m_phong;
		ImgSpacePBR *m_jfpbr;
		
		Effect m_effect;
		
		TextEffect m_textEffect;
		
		/** Frameskip for the Jump Flooding effect. */
		int m_jfpbrFrameskip;
		
		/** Frame counter. Used in order to skip frames properly. */
		unsigned int m_nFrames;
		
		/** The maximum octree depth. */
		uint m_maxOctreeDepth;
	};
}

#endif