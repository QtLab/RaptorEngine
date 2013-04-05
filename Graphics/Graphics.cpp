/*
 *  Graphics.cpp
 */

#include "Graphics.h"

#include <cmath>
#include <cfloat>
#include <OpenGL/glu.h>
#include <SDL/SDL.h>
#include "Endian.h"
#include "Num.h"
#include "ResourceManager.h"
#include "ClientConfig.h"
#include "RaptorGame.h"


Graphics::Graphics( void )
{
	Initialized = false;
	
	W = 640;
	H = 480;
	AspectRatio = ((float)( W )) / ((float)( H ));
	Fullscreen = false;
	FSAA = 0;
	AF = 16;
	ShaderFile = "model";
	
	Screen = NULL;
}


Graphics::~Graphics()
{
	// Don't free this, because it causes EXC_BAD_ACCESS.
	Screen = NULL;
	
	if( Initialized )
	{
		SDL_ShowCursor( SDL_ENABLE );
		TTF_Quit();
	}
	
	Initialized = false;
}


void Graphics::Initialize( void )
{
	// Initialize SDL.
	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "Unable to initialize SDL: %s\n", SDL_GetError() );
		return;
	};
	
	// Initialize SDL_ttf for text rendering.
	TTF_Init();
	
	Initialized = true;
	
	// Configure mode.
	Restart();
}


void Graphics::SetMode( int x, int y )
{
	SetMode( x, y, Fullscreen, FSAA, AF, ShaderFile );
}


void Graphics::SetMode( int x, int y, bool fullscreen, int fsaa, int af, std::string shader_file )
{
	// Make sure we've initialized SDL.
	if( ! Initialized )
		Initialize();
	if( ! Initialized )
		return;
	
	// Set up the video properties.
	W = x;
	H = y;
	AspectRatio = (W && H) ? ((float)( W )) / ((float)( H )) : 1.f;
	Fullscreen = fullscreen;
	FSAA = fsaa;
	AF = af;
	ShaderFile = shader_file;
	
	// Enable double-buffering and vsync.
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );
	
	// 24-bit minimum depth buffer.
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	
	// Enable anti-aliasing.
	if( FSAA > 1 )
	{
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, FSAA );
	}
	
	// Make sure we're getting hardware-accelerated OpenGL.
	SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
	
	// The current OpenGL context will be invalid, so clear all textures from the ResourceManager first.
	Raptor::Game->Res.DeleteGraphics();
	Raptor::Game->ShaderMgr.DeleteShaders();
	
	// Free the current screen.
	if( Screen )
		SDL_FreeSurface( Screen );
	
	// Create a new screen.
	Screen = SDL_SetVideoMode( x, y, 32, SDL_OPENGL | SDL_ANYFORMAT | (Fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE) );
	
	if( ! Screen )
	{
		// We couldn't set that video mode, so try a few windowed modes.
		if( (x > 1024) || (y > 768) || fullscreen || (fsaa > 0) )
			SetMode( 1024, 768, false, 0, af, shader_file );
		else if( (x != 640) || (y != 480) || fullscreen || (fsaa > 0) )
			SetMode( 640, 480, false, 0, af, shader_file );
		else
		{
			fprintf( stderr, "Unable to set video mode: %s\n", SDL_GetError() );
			exit( -1 );
		}
		return;
	}
	
	// We successfully set the video mode, so pull the resolution from the SDL_Screen.
	W = Screen->w;
	H = Screen->h;
	AspectRatio = (W && H) ? ((float)( W )) / ((float)( H )) : 1.f;
	
	// Determine maximum AF.
	if( AF < 1 )
		AF = 1;
	GLint max_af = 1;
	glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_af );
	if( AF > max_af )
		AF = max_af;
	
	// Set hints to nicest.
	glHint( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
	glHint( GL_TEXTURE_COMPRESSION_HINT, GL_NICEST );
	glHint( GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	glHint( GL_FOG_HINT, GL_NICEST );
	
	// Allow fixed-function point sizes.
	glDisable( GL_PROGRAM_POINT_SIZE );
	
	// Tell the ResourceManager to reload any previously-loaded textures.
	Raptor::Game->Res.ReloadGraphics();
	Raptor::Game->ShaderMgr.LoadShaders( shader_file );
	
	// Set the titlebar name.
	SDL_WM_SetCaption( Raptor::Game->Game.c_str(), NULL );
	
	// Set the OpenGL state after creating the context with SDL_SetVideoMode.
	glViewport( 0, 0, W, H );
	
	// Enable alpha blending.
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	
	// Disable the system cursor.
	SDL_ShowCursor( SDL_DISABLE );
	
	
	// Update the client config.
	if( Fullscreen )
	{
		Raptor::Game->Cfg.Settings[ "g_fullscreen" ] = "true";
		
		// We use x,y instead of W,H so 0,0 (native res) can be retained.
		Raptor::Game->Cfg.Settings[ "g_res_fullscreen_x" ] = Num::ToString(x);
		Raptor::Game->Cfg.Settings[ "g_res_fullscreen_y" ] = Num::ToString(y);
	}
	else
	{
		Raptor::Game->Cfg.Settings[ "g_fullscreen" ] = "false";
		
		// We use x,y instead of W,H so 0,0 (native res) can be retained.
		Raptor::Game->Cfg.Settings[ "g_res_windowed_x" ] = Num::ToString(x);
		Raptor::Game->Cfg.Settings[ "g_res_windowed_y" ] = Num::ToString(y);
	}
	Raptor::Game->Cfg.Settings[ "g_fsaa" ] = Num::ToString(FSAA);
	Raptor::Game->Cfg.Settings[ "g_af" ] = Num::ToString(AF);
	Raptor::Game->Cfg.Settings[ "g_shader_file" ] = ShaderFile;
}


void Graphics::Restart( void )
{
	int x = 1024, y = 768;
	bool fullscreen = Raptor::Game->Cfg.SettingAsBool( "g_fullscreen" );
	int fsaa = Raptor::Game->Cfg.SettingAsInt( "g_fsaa" );
	int af = Raptor::Game->Cfg.SettingAsInt( "g_af" );
	std::string shader_file = Raptor::Game->Cfg.SettingAsString( "g_shader_file" );
	if( fullscreen )
	{
		x = Raptor::Game->Cfg.SettingAsInt( "g_res_fullscreen_x" );
		y = Raptor::Game->Cfg.SettingAsInt( "g_res_fullscreen_y" );
	}
	else
	{
		x = Raptor::Game->Cfg.SettingAsInt( "g_res_windowed_x" );
		y = Raptor::Game->Cfg.SettingAsInt( "g_res_windowed_y" );
	}
	
	SetMode( x, y, fullscreen, fsaa, af, shader_file );
}


bool Graphics::SelectDefaultFramebuffer( void )
{
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, W, H );
	return true;
}


bool Graphics::SelectFramebuffer( Framebuffer *fb )
{
	if( fb )
		return fb->Select();
	return false;
}


void Graphics::Clear( void )
{
	glClearColor( 0.f, 0.f, 0.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


void Graphics::Clear( GLfloat red, GLfloat green, GLfloat blue )
{
	glClearColor( red, green, blue, 1.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


void Graphics::Clear( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
	glClearColor( red, green, blue, alpha );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


void Graphics::SwapBuffers( void )
{
	SDL_GL_SwapBuffers();
}


void Graphics::Setup2D( void )
{
	Setup2D( 0, 0, W, H );
}


void Graphics::Setup2D( double y1, double y2 )
{
	double h = y2 - y1;
	double w = h * (double) W / (double) H;
	double extra = (w - h) / 2.;
	double x1 = y1 - extra;
	double x2 = y2 + extra;
	
	Setup2D( x1, y1, x2, y2 );
}


void Graphics::Setup2D( double x1, double y1, double x2, double y2 )
{
	glDisable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( x1, x2, y2, y1, -1, 1 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}


void Graphics::Setup3D( Camera *cam )
{
	Setup3D( cam->FOV,  cam->X, cam->Y, cam->Z,  cam->X + cam->Fwd.X, cam->Y + cam->Fwd.Y, cam->Z + cam->Fwd.Z,  cam->Up.X, cam->Up.Y, cam->Up.Z );
}


void Graphics::Setup3D( double fov_w, double cam_x, double cam_y, double cam_z, double yaw )
{
	Setup3D( fov_w,  cam_x, cam_y, cam_z,  cam_x + cos(yaw), cam_y + sin(yaw), cam_z,  0, 0, 1 );
}


void Graphics::Setup3D( double fov_w, double cam_x, double cam_y, double cam_z, double yaw, double pitch )
{
	Setup3D( fov_w,  cam_x, cam_y, cam_z,  cam_x + cos(yaw)*cos(pitch), cam_y + sin(yaw)*cos(pitch), cam_z + sin(pitch),  0, 0, 1 );
}


void Graphics::Setup3D( double fov_w, double cam_x, double cam_y, double cam_z, double cam_look_x, double cam_look_y, double cam_look_z, double cam_up_x, double cam_up_y, double cam_up_z )
{
	// If we pass FOV=0, calculate a good default.  4:3 is FOV 90, widescreen is scaled appropriately.
	if( fov_w == 0. )
		fov_w = 67.5 * AspectRatio;
	// If we pass FOV<0, treat its absolute value as fov_h.
	else if( fov_w < 0. )
		fov_w *= -AspectRatio;
	
	glEnable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( fov_w / AspectRatio, AspectRatio, 1., FLT_MAX );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	gluLookAt( cam_x, cam_y, cam_z,  cam_look_x, cam_look_y, cam_look_z,  cam_up_x, cam_up_y, cam_up_z );
}


// ---------------------------------------------------------------------------


void Graphics::DrawRect2D( int x1, int y1, int x2, int y2, GLuint texture )
{
	if( texture )
	{
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, texture );
	}
	
	glBegin( GL_QUADS );
		
		// Top-left
		if( texture )
			glTexCoord2i( 0, 0 );
		glVertex2i( x1, y1 );
		
		// Bottom-left
		if( texture )
			glTexCoord2i( 0, 1 );
		glVertex2i( x1, y2 );
		
		// Bottom-right
		if( texture )
			glTexCoord2i( 1, 1 );
		glVertex2i( x2, y2 );
		
		// Top-right
		if( texture )
			glTexCoord2i( 1, 0 );
		glVertex2i( x2, y1 );
		
	glEnd();
	
	if( texture )
		glDisable( GL_TEXTURE_2D );
}


void Graphics::DrawRect2D( double x1, double y1, double x2, double y2, GLuint texture )
{
	if( texture )
	{
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, texture );
	}
	
	glBegin( GL_QUADS );
		
		// Top-left
		if( texture )
			glTexCoord2i( 0, 0 );
		glVertex2d( x1, y1 );
		
		// Bottom-left
		if( texture )
			glTexCoord2i( 0, 1 );
		glVertex2d( x1, y2 );
		
		// Bottom-right
		if( texture )
			glTexCoord2i( 1, 1 );
		glVertex2d( x2, y2 );
		
		// Top-right
		if( texture )
			glTexCoord2i( 1, 0 );
		glVertex2d( x2, y1 );
		
	glEnd();
	
	if( texture )
		glDisable( GL_TEXTURE_2D );
}


void Graphics::DrawRect2D( int x1, int y1, int x2, int y2, GLuint texture, float r, float g, float b, float a )
{
	glColor4f( r, g, b, a );
	DrawRect2D( x1, y1, x2, y2, texture );
}


void Graphics::DrawRect2D( double x1, double y1, double x2, double y2, GLuint texture, float r, float g, float b, float a )
{
	glColor4f( r, g, b, a );
	DrawRect2D( x1, y1, x2, y2, texture );
}


// ---------------------------------------------------------------------------


void Graphics::DrawBox2D( double x1, double y1, double x2, double y2, float r, float g, float b, float a )
{
	glColor4f( r, g, b, a );
	
	glBegin( GL_LINE_LOOP );
		glVertex2d( x1, y1 );
		glVertex2d( x1, y2 );
		glVertex2d( x2, y2 );
		glVertex2d( x2, y1 );
	glEnd();
}


// ---------------------------------------------------------------------------


void Graphics::DrawCircle2D( double x, double y, double r, int res, GLuint texture )
{
	DrawCircle2D( x, y, r, res, texture, 1.f, 1.f, 1.f, 1.f );
}


void Graphics::DrawCircle2D( double x, double y, double r, int res, GLuint texture, float red, float green, float blue, float alpha )
{
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture );
	glColor4f( red, green, blue, alpha );
	
	glBegin( GL_TRIANGLE_FAN );
		
		glTexCoord2d( 0.5, 0.5 );
		glVertex2d( x, y );
		
		for( int i = 0; i <= res; i ++ )
		{
			double percent = ((double) i) / (double) res;
			double unit_x = cos( percent * 2. * M_PI );
			double unit_y = sin( percent * 2. * M_PI );
			double px = x + unit_x * r;
			double py = y + unit_y * r;
			double tx = (unit_x + 1.) / 2.;
			double ty = (unit_y + 1.) / 2.;
			
			glTexCoord2d( tx, ty );
			glVertex2d( px, py );
		}
		
	glEnd();
	
	glDisable( GL_TEXTURE_2D );
}


// ---------------------------------------------------------------------------


void Graphics::DrawCircleOutline2D( double x, double y, double r, int res )
{
	DrawCircleOutline2D( x, y, r, res, 1.f, 1.f, 1.f, 1.f );
}


void Graphics::DrawCircleOutline2D( double x, double y, double r, int res, float red, float green, float blue, float alpha )
{
	glColor4f( red, green, blue, alpha );
	
	glBegin( GL_LINE_STRIP );
		
		for( int i = 0; i <= res; i ++ )
		{
			double percent = ((double) i) / (double) res;
			double unit_x = cos( percent * 2. * M_PI );
			double unit_y = sin( percent * 2. * M_PI );
			double px = x + unit_x * r;
			double py = y + unit_y * r;
			
			glVertex2d( px, py );
		}
		
	glEnd();
}


// ---------------------------------------------------------------------------


void Graphics::DrawLine2D( double x1, double y1, double x2, double y2, float r, float g, float b, float a )
{
	glColor4f( r, g, b, a );
	
	glBegin( GL_LINES );
		glVertex2d( x1, y1 );
		glVertex2d( x2, y2 );
	glEnd();
}


// ---------------------------------------------------------------------------


void Graphics::DrawSphere3D( double x, double y, double z, double r, int res, GLuint texture, uint32_t texture_mode )
{
	DrawSphere3D( x, y, z, r, res, texture, texture_mode, 1.f, 1.f, 1.f, 1.f );
}


void Graphics::DrawSphere3D( double x, double y, double z, double r, int res, GLuint texture, float red, float green, float blue, float alpha )
{
	DrawSphere3D( x, y, z, r, res, texture, TEXTURE_MODE_Y_ASIN, red, green, blue, alpha );
}


void Graphics::DrawSphere3D( double x, double y, double z, double r, int res, GLuint texture, uint32_t texture_mode, float red, float green, float blue, float alpha )
{
	int res_vertical = res;
	int res_around = res * 2;
	double step_vertical = 1. / (double) res_vertical;
	double step_around = 1. / (double) res_around;
	
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture );
	glColor4f( red, green, blue, alpha );
	
	glBegin( GL_QUADS );
		
		for( int i = 0; i < res_vertical; i ++ )
		{
			double percent_vertical = ((double) i) / (double) res_vertical;
			double z1 = r * cos( percent_vertical * M_PI );
			double r1 = r * sin( percent_vertical * M_PI );
			double z2 = r * cos( (percent_vertical + step_vertical) * M_PI );
			double r2 = r * sin( (percent_vertical + step_vertical) * M_PI );
			
			for( int j = 0; j < res_around; j ++ )
			{
				double percent_around = ((double) j) / (double) res_around;
				double x1 = cos( percent_around * 2. * M_PI );
				double y1 = sin( percent_around * 2. * M_PI );
				double x2 = cos( (percent_around + step_around) * 2. * M_PI );
				double y2 = sin( (percent_around + step_around) * 2. * M_PI );
				
				// Texture coordinates.
				double tc[ 4 ][ 2 ];
				tc[ 0 ][ 0 ] = percent_around;
				tc[ 0 ][ 1 ] = percent_vertical;
				tc[ 1 ][ 0 ] = percent_around;
				tc[ 1 ][ 1 ] = percent_vertical + step_vertical;
				tc[ 2 ][ 0 ] = percent_around + step_around;
				tc[ 2 ][ 1 ] = percent_vertical + step_vertical;
				tc[ 3 ][ 0 ] = percent_around + step_around;
				tc[ 3 ][ 1 ] = percent_vertical;
				
				// Experimental texture coordinate distortions.
				if( texture_mode & TEXTURE_MODE_Y_ASIN )
				{
					// Scale traversal of Y values to the radius of the current ring.
					tc[ 0 ][ 1 ] = asin( percent_vertical * 2. - 1. ) / M_PI + 0.5;
					tc[ 1 ][ 1 ] = asin( (percent_vertical + step_vertical) * 2. - 1. ) / M_PI + 0.5;
					tc[ 2 ][ 1 ] = tc[ 1 ][ 1 ];
					tc[ 3 ][ 1 ] = tc[ 0 ][ 1 ];
				}
				if( texture_mode & TEXTURE_MODE_X_DIV_R )
				{
					// Remove corners of texture by reducing the X distance traversed at top and bottom.
					double r1scale = r1 / r;
					tc[ 0 ][ 0 ] = percent_around * r1scale + (1. - r1scale) / 2.;
					tc[ 3 ][ 0 ] = tc[ 0 ][ 0 ] + r1scale * step_around;
					double r2scale = r2 / r;
					tc[ 1 ][ 0 ] = percent_around * r2scale + (1. - r2scale) / 2.;
					tc[ 2 ][ 0 ] = tc[ 1 ][ 0 ] + r2scale * step_around;
				}
				
				// Calculate vertices.
				double px[ 4 ] = { x + r1*x1, x + r2*x1, x + r2*x2, x + r1*x2 };
				double py[ 4 ] = { y + r1*y1, y + r2*y1, y + r2*y2, y + r1*y2 };
				double pz[ 2 ] = { z + z1, z + z2 };
				
				/*
				// Calculate per-vertex normals.
				// Commented-out because OpenGL seems to only utilize per-face normals.
				Vec3D normals[ 4 ] = { Vec3D(r1*x1,r1*y1,z1), Vec3D(r2*x1,r2*y1,z2), Vec3D(r2*x2,r2*y2,z2), Vec3D(r1*x2,r1*y2,z1) };
				for( int k = 0; k < 4; k ++ )
					normals[ k ].ScaleTo( 1. );
				*/
				
				// Set face normal vector.
				Vec3D normal = Vec3D( px[1] - px[0], py[1] - py[0], pz[1] - pz[0] ).Cross( Vec3D( px[3] - px[0], py[3] - py[0], 0. ) );
				if( i == 0 )
					normal = Vec3D( px[3] - px[2], py[3] - py[2], pz[1] - pz[0] ).Cross( Vec3D( px[1] - px[2], py[1] - py[2], 0. ) );
				normal.ScaleTo( 1. );
				glNormal3d( normal.X, normal.Y, normal.Z );
				
				// Top-left
				glTexCoord2d( tc[ 0 ][ 0 ], tc[ 0 ][ 1 ] );
				glVertex3d( px[ 0 ], py[ 0 ], pz[ 0 ] );
				//glNormal3d( normals[ 0 ].X, normals[ 0 ].Y, normals[ 0 ].Z );
				
				// Bottom-left
				glTexCoord2d( tc[ 1 ][ 0 ], tc[ 1 ][ 1 ] );
				glVertex3d( px[ 1 ], py[ 1 ], pz[ 1 ] );
				//glNormal3d( normals[ 1 ].X, normals[ 1 ].Y, normals[ 1 ].Z );
				
				// Bottom-right
				glTexCoord2d( tc[ 2 ][ 0 ], tc[ 2 ][ 1 ] );
				glVertex3d( px[ 2 ], py[ 2 ], pz[ 1 ] );
				//glNormal3d( normals[ 2 ].X, normals[ 2 ].Y, normals[ 2 ].Z );
				
				// Top-right
				glTexCoord2d( tc[ 3 ][ 0 ], tc[ 3 ][ 1 ] );
				glVertex3d( px[ 3 ], py[ 3 ], pz[ 0 ] );
				//glNormal3d( normals[ 3 ].X, normals[ 3 ].Y, normals[ 3 ].Z );
			}
		}
		
	glEnd();
	
	glDisable( GL_TEXTURE_2D );
}


// ---------------------------------------------------------------------------


void Graphics::DrawLine3D( double x1, double y1, double z1, double x2, double y2, double z2, float r, float g, float b, float a )
{
	glColor4f( r, g, b, a );
	
	glBegin( GL_LINES );
		glVertex3d( x1, y1, z1 );
		glVertex3d( x2, y2, z2 );
	glEnd();
}


// ---------------------------------------------------------------------------


GLuint Graphics::SDL_GL_LoadTexture( SDL_Surface *surface, GLfloat *texcoord, GLint texture_mode )
{
	// Use the surface width and height expanded to powers of 2
	int w = Num::NextPowerOfTwo( surface->w );
	int h = Num::NextPowerOfTwo( surface->h );
	texcoord[0] = 0.f;                                          // Min X
	texcoord[1] = 0.f;                                          // Min Y
	texcoord[2] = ((GLfloat)( surface->w )) / ((GLfloat)( w )); // Max X
	texcoord[3] = ((GLfloat)( surface->h )) / ((GLfloat)( h ));	// Max Y
	
	SDL_Surface *image = SDL_CreateRGBSurface(
	        SDL_SWSURFACE,
	        w, h,
	        32,
#ifdef ENDIAN_BIG
	        // RGBA
	        0xFF000000,
	        0x00FF0000,
	        0x0000FF00,
	        0x000000FF
#else
	        // ABGR
	        0x000000FF,
	        0x0000FF00,
	        0x00FF0000,
	        0xFF000000
#endif
       		);
	if( ! image )
		return 0;
	
	// Save the alpha blending attributes.
	Uint32 saved_flags = surface->flags & ( SDL_SRCALPHA | SDL_RLEACCELOK );
	Uint8 saved_alpha = surface->format->alpha;
	if( saved_flags & SDL_SRCALPHA )
		SDL_SetAlpha( surface, 0, 0 );
	
	// Copy the surface into the GL texture image.
	SDL_Rect area;
	area.x = 0;
	area.y = 0;
	area.w = surface->w;
	area.h = surface->h;
	SDL_BlitSurface( surface, &area, image, &area );
	
	// Restore the alpha blending attributes.
	if( saved_flags & SDL_SRCALPHA )
		SDL_SetAlpha( surface, saved_flags, saved_alpha );
	
	// Determine best texture modes and if we need to generate mipmaps.
	GLint texture_mode_mag = texture_mode;
	GLint texture_mode_min = texture_mode;
	bool mipmap = false;
	if( (texture_mode == GL_LINEAR_MIPMAP_LINEAR) || (texture_mode == GL_LINEAR_MIPMAP_NEAREST) )
	{
		texture_mode_mag = GL_LINEAR;
		mipmap = true;
	}
	else if( (texture_mode == GL_NEAREST_MIPMAP_LINEAR) || (texture_mode == GL_NEAREST_MIPMAP_NEAREST) )
	{
		texture_mode_mag = GL_NEAREST;
		mipmap = true;
	}
	else if( texture_mode == GL_NEAREST )
		texture_mode_min = GL_LINEAR;
	
	// Create an OpenGL texture for the image.
	GLuint texture = 0;
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_mode_mag );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_mode_min );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );
	if( mipmap )
	{
		#if !( defined(_ppc_) || defined(__ppc__) )
			glGenerateMipmap( GL_TEXTURE_2D );
		#else
			gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );
		#endif
	}
	SDL_FreeSurface( image );
	
	return texture;
}