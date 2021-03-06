/*
   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//
// Sprite Model Plugin
//
// Code by Hydra aka Dominic Clifton
//
// Based on MD3Model source code by SPoG
//

#include "spritemodel.h"

void LoadSpriteModel( entity_interfaces_t *interfaces, const char *name ){
	IShader *pShader;

	pShader = QERApp_Shader_ForName( name );

	if ( !pShader ) {
		Sys_FPrintf( SYS_ERR, "ERROR: can't find shader (or image) for: %s\n", name );
		return;
	}

	CSpriteModel *model = new CSpriteModel();
	model->Construct( pShader );
	interfaces->pRender = (IRender*)model;
	interfaces->pRender->IncRef();
	//interfaces->pSelect = (ISelect*)model;
	//interfaces->pSelect->IncRef();
	interfaces->pSelect = NULL;
	interfaces->pEdit = NULL;
	model->DecRef();
	if( strstr( name, ".cube." ) != nullptr )
	{
		model->SetSpriteType( SPRITE_TYPE_CUBE );
	}
	else
	{
		model->SetSpriteType( SPRITE_TYPE_BILLBOARD );
	}
}

void CSpriteModel::Construct( IShader *pShader ){
	m_pShader = pShader;
	aabb_clear( &m_BBox );
	/*
	   md3Surface_t *pSurface = (md3Surface_t *)(((unsigned char *)pHeader) + pHeader->ofsSurfaces);
	   m_nSurfaces = pHeader->numSurfaces;
	   CMD3Surface* surfaces = new CMD3Surface[m_nSurfaces];
	   for (int i = 0; i < m_nSurfaces; i++ )
	   {
	   surfaces[i].Construct(pSurface);
	      pSurface = (md3Surface_t *) ((( char * ) pSurface) + pSurface->ofsEnd);
	   }
	   m_children = surfaces;
	   AccumulateBBox();
	 */
}

CSpriteModel::CSpriteModel(){
	refCount = 1;
	//m_nSurfaces = 0;
	//m_children = NULL;
	m_pShader = NULL;
}

CSpriteModel::~CSpriteModel(){
	// if(m_children) delete[] m_children;
	if ( m_pShader ) {
		m_pShader->DecRef();
	}
}

void CSpriteModel::Draw( int state, int rflags ) const {

/*
   // Draw a point in the middle of the bbox
   vec3_t middle = {0,0,0};
   g_QglTable.m_pfn_qglPointSize (4);
   g_QglTable.m_pfn_qglColor3f (0,1,0);
   g_QglTable.m_pfn_qglBegin (GL_POINTS);
   g_QglTable.m_pfn_qglVertex3fv (middle);
   g_QglTable.m_pfn_qglEnd ();
 */

	qtexture_t *q = m_pShader->getTexture();

	// setup opengl stuff
	g_QglTable.m_pfn_qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	g_QglTable.m_pfn_qglEnable( GL_BLEND );
	g_QglTable.m_pfn_qglClearColor( 0.0, 0.0, 0.0, 0.0 );
	g_QglTable.m_pfn_qglPushAttrib( GL_ALL_ATTRIB_BITS ); // GL_ENABLE_BIT
	//g_QglTable.m_pfn_qglColor3f (1,1,1);   //testing
	//g_QglTable.m_pfn_qglColor4f (1,1,1,1); //testing
	g_QglTable.m_pfn_qglBindTexture( GL_TEXTURE_2D, q->texture_number );

	//g_QglTable.m_pfn_qglEnable (GL_TEXTURE_2D); // FIXME: ? this forces textures, even in wireframe mode, bad... ?

	g_QglTable.m_pfn_qglAlphaFunc( GL_GREATER, 2.0f / 255.0f );
	g_QglTable.m_pfn_qglEnable( GL_ALPHA_TEST );
	g_QglTable.m_pfn_qglPolygonMode( GL_FRONT, GL_FILL );

	switch( spriteType )
	{
	case SPRITE_TYPE_BILLBOARD:
	default:
		DrawBillboard( state, rflags );
		break;
	case SPRITE_TYPE_CUBE:
		DrawCube( state, rflags );
		break;
	}

	g_QglTable.m_pfn_qglBindTexture( GL_TEXTURE_2D, 0 );
	g_QglTable.m_pfn_qglPopAttrib();
}

void CSpriteModel::DrawBillboard( int state, int rflags ) const
{
	qtexture_t *q = m_pShader->getTexture();

	// convert pixels to units and divide in half again so we draw in the middle
	// of the bbox.
	int h = q->height / 8;
	int w = q->width / 8;

	// start billboarding
	bool use_billboarding = ( state & DRAW_GL_TEXTURE_2D );
	if( use_billboarding ) {
		vec3_t origin, angles;
		g_CameraTable.m_pfnGetCamera( origin, angles );
		g_QglTable.m_pfn_qglPushMatrix();
		g_QglTable.m_pfn_qglRotatef( -angles[1], 0, 0, -1 ); // yaw
		g_QglTable.m_pfn_qglRotatef( -angles[0], 0, 1, 0 ); // pitch
	}

	// draw the sprite
#if 0
	// using x/y axis, it appears FLAT without the proper transform and rotation.

	g_QglTable.m_pfn_qglBegin( GL_QUADS );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( 0 - w, 0 - h, 0 );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, 0 - h, 0 );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, h, 0 );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( 0 - w, h, 0 );
	g_QglTable.m_pfn_qglEnd();
#else

	// so draw it using y/z instead.
	g_QglTable.m_pfn_qglBegin( GL_QUADS );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( 0, w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( 0, 0 - w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( 0, 0 - w, 0 - h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( 0, w, 0 - h );
	g_QglTable.m_pfn_qglEnd();

#endif

	// end billboard
	if( use_billboarding ) {
		g_QglTable.m_pfn_qglPopMatrix();
	}
}

void CSpriteModel::DrawCube( int state, int rflags ) const
{
	qtexture_t    *q = m_pShader->getTexture();

	// convert pixels to units and divide in half again so we draw in the middle
	// of the bbox.
	int h = q->height / 8;
	int w = q->width / 8;

	g_QglTable.m_pfn_qglBegin( GL_QUADS );

	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, w, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, -w, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, -w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, w, h );

	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( -w, w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( -w, 0 - w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( -w, 0 - w, 0 - h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( -w, w, 0 - h );

	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, -w, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( -w, -w, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( -w, -w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, -w, h );

	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( -w, w, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( -w, w, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, w, -h );

	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, h, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( -w, h, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( -w, -h, -h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, -h, -h );

	g_QglTable.m_pfn_qglTexCoord2f( 1, 1 );
	g_QglTable.m_pfn_qglVertex3f( w, -h, h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 1 );
	g_QglTable.m_pfn_qglVertex3f( -w, -h, h );
	g_QglTable.m_pfn_qglTexCoord2f( 0, 0 );
	g_QglTable.m_pfn_qglVertex3f( -w, h, h );
	g_QglTable.m_pfn_qglTexCoord2f( 1, 0 );
	g_QglTable.m_pfn_qglVertex3f( w, h, h );

	g_QglTable.m_pfn_qglEnd();
}

/*
   bool CSpriteModel::TestRay(const ray_t *ray, vec_t *dist) const
   {
   vec_t depth_start = *dist;
   vec_t depth_local = *dist;

   if (aabb_test_ray(&m_BBox, ray) == 0)
    return false;

   for(int i=0; i<m_nSurfaces; i++)
   {
    if(m_children[i].TestRay(ray, &depth_local))
    {
      if (depth_local < *dist) *dist = depth_local;
    }
   }

   return *dist < depth_start;
   }
 */
