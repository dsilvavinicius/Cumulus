#ifndef TEXT_EFFECT
#define TEXT_EFFECT

#include <effect.hpp>
#include <texture.hpp>
#include <mesh.hpp>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace Tucano;
using namespace Eigen;

namespace model
{
	/**
	* The atlas struct holds a texture that contains the visible US-ASCII characters
	* of a certain font rendered with a certain character height.
	* It also contains an array that contains all the information necessary to
	* generate the appropriate vertex and texture coordinates for each character.
	*
	* After the constructor is run, you don't need to use any FreeType functions anymore.
	*/
	struct Atlas {
		Texture tex;	// texture object

		unsigned int w;	// width of texture in pixels
		unsigned int h;	// height of texture in pixels

		struct {
			float ax;	// advance.x
			float ay;	// advance.y

			float bw;	// bitmap.width;
			float bh;	// bitmap.height;

			float bl;	// bitmap_left;
			float bt;	// bitmap_top;

			float tx;	// x offset of glyph in texture coordinates
			float ty;	// y offset of glyph in texture coordinates
		} c[128];		// character information
		
		Atlas( FT_Face face, int height )
		{
			// Maximum texture width
			int MAXWIDTH = 1024;
			
			FT_Set_Pixel_Sizes( face, 0, height );
			FT_GlyphSlot g = face->glyph;

			unsigned int roww = 0;
			unsigned int rowh = 0;
			w = 0;
			h = 0;

			memset( c, 0, sizeof c );

			/* Find minimum size for a texture holding all visible ASCII characters */
			for( int i = 32; i < 128; i++ )
			{
				if( FT_Load_Char( face, i, FT_LOAD_RENDER ) )
				{
					stringstream ss;
					ss << "Loading character " << i << " failed!";
					throw logic_error( ss.str() );
				}
				if( roww + g->bitmap.width + 1 >= MAXWIDTH )
				{
					w = std::max( w, roww );
					h += rowh;
					roww = 0;
					rowh = 0;
				}
				roww += g->bitmap.width + 1;
				rowh = std::max( rowh, g->bitmap.rows );
			}

			w = std::max( w, roww );
			h += rowh;

			/* Create a texture that will be used to hold all ASCII glyphs */
			tex.create( GL_TEXTURE_2D, GL_ALPHA, w, h, GL_ALPHA, GL_UNSIGNED_BYTE );

			/* We require 1 byte alignment when uploading texture data */
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			/* Clamping to edges is important to prevent artifacts when scaling */
			/* Linear filtering usually looks best for text */
			tex.setTexParameters( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR );
			
			tex.bind();

			/* Paste all glyph bitmaps into the texture, remembering the offset */
			int ox = 0;
			int oy = 0;

			rowh = 0;

			for( int i = 32; i < 128; i++ )
			{
				if( FT_Load_Char( face, i, FT_LOAD_RENDER ) )
				{
					stringstream ss;
					ss << "Loading character " <<  i << " failed!";
					throw logic_error( ss.str() );
				}

				if( ox + g->bitmap.width + 1 >= MAXWIDTH )
				{
					oy += rowh;
					rowh = 0;
					ox = 0;
				}
				
				glTexSubImage2D( GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE,
								 g->bitmap.buffer );
				c[i].ax = g->advance.x >> 6;
				c[i].ay = g->advance.y >> 6;

				c[i].bw = g->bitmap.width;
				c[i].bh = g->bitmap.rows;

				c[i].bl = g->bitmap_left;
				c[i].bt = g->bitmap_top;

				c[i].tx = ox / ( float ) w;
				c[i].ty = oy / ( float ) h;

				rowh = std::max( rowh, g->bitmap.rows );
				ox += g->bitmap.width + 1;
			}

			fprintf( stderr, "Generated a %d x %d (%d kb) texture atlas\n", w, h, w * h / 1024 );
		}
	};
	
	/** Effect for rendering text using character atlasses. Implementation of OpenGL Programming/Modern OpenGL Tutorial
	 * Text Rendering 02 available in
	 * https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02 */
	class TextEffect
	: public Effect
	{
	public:
		TextEffect( string shadersDir = "shaders/" );
		
		~TextEffect();
		
		virtual void initialize() {};
		
		virtual void initialize( const string& fontFilename );
		
		/**
		* Render text using the currently loaded font and currently set font size.
		* Rendering starts at coordinates (x, y), z is always 0.
		* The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
		*/
		virtual void render( const char *text, float x, float y, float sx, float sy );
		
		/** Sets the text color. Supports alpha channel. */
		void setColor( const Vector4f& color );
		
	protected:
		Atlas* m_a48;	/** 48pt atlas */
		Atlas* m_a24;	/** 24pt atlas */
		Atlas* m_a12;	/** 12pt atlas */
		Atlas* m_a6;	/** 6pt atlas */
		
		Shader m_textShader;
	};
	
	inline TextEffect::TextEffect( string shadersDir )
	: Effect( shadersDir )
	{}
	
	inline TextEffect::~TextEffect()
	{
		delete m_a48;
		delete m_a24;
		delete m_a12;
		delete m_a6;
	}
	
	inline void TextEffect::initialize( const string& fontFilename )
	{
		FT_Library ft;
		FT_Face face;
		
		/* Initialize the FreeType2 library */
		if( FT_Init_FreeType( &ft ) )
		{
			throw runtime_error( "Could not init freetype library." );
		}

		/* Load a font */
		if( FT_New_Face( ft, fontFilename.c_str(), 0, &face ) )
		{
			stringstream ss;
			ss << "Could not open font " << fontFilename;
			throw logic_error( ss.str() );
		}

		loadShader( m_textShader, "Text" );

		/* Create texture atlasses for several font sizes */
		m_a48 = new Atlas( face, 48 );
		m_a24 = new Atlas( face, 24 );
		m_a12 = new Atlas( face, 12 );
		m_a6 = new Atlas( face, 6 );
	}
	
	inline void TextEffect::render( const char *text, float x, float y, float sx, float sy )
	{
		const uint8_t *p;
		Atlas* a = m_a12;

		vector< Vector4f > coords( 6 * strlen(text) );
		int c = 0;

		/* Loop through all characters */
		for( p = ( const uint8_t * )text; *p; p++ )
		{
			/* Calculate the vertex and texture coordinates */
			float x2 = x + a->c[ *p ].bl * sx;
			float y2 = -y - a->c[ *p ].bt * sy;
			float w = a->c[ *p ].bw * sx;
			float h = a->c[ *p ].bh * sy;

			/* Advance the cursor to the start of the next character */
			x += a->c[ *p ].ax * sx;
			y += a->c[ *p ].ay * sy;

			/* Skip glyphs that have no pixels */
			if( !w || !h )
				continue;

			coords[ c++ ] = Vector4f( x2,		-y2,	 a->c[ *p ].tx,							a->c[ *p ].ty );
			coords[ c++ ] = Vector4f( x2 + w,	-y2,	 a->c[ *p ].tx + a->c[ *p ].bw / a->w,	a->c[ *p ].ty );
			coords[ c++ ] = Vector4f( x2,		-y2 - h, a->c[ *p ].tx,							a->c[ *p ].ty + a->c[ *p ].bh / a->h );
			coords[ c++ ] = Vector4f( x2 + w,	-y2,	 a->c[ *p ].tx + a->c[ *p ].bw / a->w,	a->c[ *p ].ty );
			coords[ c++ ] = Vector4f( x2,		-y2 - h, a->c[ *p ].tx,							a->c[ *p ].ty + a->c[ *p ].bh / a->h );
			coords[ c++ ] = Vector4f( x2 + w,	-y2 - h, a->c[ *p ].tx + a->c[ *p ].bw / a->w,	a->c[ *p ].ty + a->c[ *p ].bh / a->h );
		}

		m_textShader.bind();
		/* Use the texture containing the atlas */
		m_textShader.setUniform( "tex", a->tex.bind() );
		
		Mesh mesh;
		mesh.loadVertices( coords );
		mesh.setAttributeLocation( m_textShader );
		mesh.render();
	}
	
	inline void TextEffect::setColor( const Vector4f& color )
	{
		m_textShader.bind();
		m_textShader.setUniform( "color", color );
	}
}

#endif