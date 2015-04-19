////////////////////////////////////////////////////////////////
// shadows.c - parallel-split shadow mapping
// (c) oP group 2010
//
// Call pssm_run(3) for adding pssm to an outdoor level
// Read shader workshop 7 for the basic algorithm
// We use DirectX classes and generally write '->' here
////////////////////////////////////////////////////////////////
#ifndef shadows_c
#define shadows_c
#include <d3d9.h>

#define fx_shadow "vp_pssm.fx"	// single-pass shadow shader
#define fx_depth "vp_depth.fx"	// depth rendering shader

//#define DEBUG_PSSM	// comment this in for displaying the shadowmaps

// PSSM global variables /////////////////////////////////////////
var pssm_res = 1024;			// shadow map resolution
var pssm_numsplits = 3;		// number of splits: 3 for small, 4 for large levels
var pssm_splitweight = 0.5; 	// logarithmic / uniform ratio
var pssm_splitdist[5];				// split distances
var pssm_transparency = 0.6;	// shadow transparency
float pssm_fbias = 0.000045;		// shadow depth bias -> must be adjusted

#include "view_to_split_custom.c"


// PSSM helper functions ////////////////////////////////////////

// Calculate a practical split scheme
// weight parameter scales between logarithmic and uniform distances
function pssm_split(VIEW* view,var num,var weight)
{
	var i;
	for(i=0; i<num; i++)
	{
		var idm = i/num;
		var cli = view->clip_near * pow(view->clip_far/view->clip_near,idm);
		var cui = view->clip_near + (view->clip_far-view->clip_near)*idm;
		pssm_splitdist[i] = cli*weight + cui*(1-weight);
	}
	pssm_splitdist[0] = view->clip_near;
	pssm_splitdist[num] = view->clip_far;
}

// Calculate a matrix to transform points to shadowmap texture coordinates
float* pssm_texscale(float fSize)
{
	static float fTexScale[16] =
	{
		0.5, 0.0, 0.0, 0.0,
		0.0,-0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	};
	fTexScale[12] = 0.5 + (0.5/fSize);
	fTexScale[13] = 0.5 + (0.5/fSize);
	return fTexScale;
}

// copy view frustum geometry to another view
void pssm_viewcpy(VIEW* from,VIEW* to)
{
	vec_set(to->x,from->x);
	vec_set(to->pan,from->pan);
	to->arc = from->arc;
 	to->aspect = from->aspect;
	to->clip_near = from->clip_near;
	to->clip_far = from->clip_far;
	to->size_x = from->size_x;
	to->size_y = from->size_y;
}


// pssm main function ////////////////////////////////////////////////////

// pssm_run(1..4) -> enable PSSM for the current level with 1..4 split views
// pssm_run(0) -> disable PSSM - required before changing the level or video resolution
function pssm_run(var numsplits)
{
	pssm_numsplits = clamp(numsplits,0,4);
	if (!numsplits) return;		// switch off shadow mapping
	shadow_stencil = 8; 			// disable other shadows
	while(!level_ent) wait(1); 	// wait until the level is loaded
	level_ent->flags |= SHADOW;	// enable shadow on level geometry
// calculate a minimum sun distance for placing the sun slightly outside the level
	sun_angle.roll = 1.1*maxv(vec_length(level_ent->max_x),vec_length(level_ent->min_x));

// create the shadow material and view	
	MATERIAL* mtlShadow = mtl_create();
 	effect_load(mtlShadow,fx_shadow);
	BMAP** pShadow = &mtlShadow->skin1; // pointer array for accessing skin1...skin4 

	VIEW* viewShadow = view_create(-1);
	viewShadow->flags |= SHOW|UNTOUCHABLE|NOSHADOW|NOPARTICLE|NOLOD|NOSKY;
	viewShadow->material = mtlShadow;
	viewShadow->bg = pixel_for_vec(COLOR_BLACK,0,8888); // clear to transparent black

// use the render_stencil bitmap for automatically rendering shadows in the camera view
	viewShadow->bmap = bmap_createblack(screen_size.x,screen_size.y,32);
	render_stencil = viewShadow->bmap;	

// create the depth materials and views
	VIEW* viewSplit[4];
	var i;
	for(i=0; i<pssm_numsplits; i++)
	{ 
		viewSplit[i] = view_create(-2);
		viewSplit[i]->flags |= viewShadow->flags|ISOMETRIC|SHADOW; // render only shadow entities
		viewSplit[i]->lod = shadow_lod;
		viewSplit[i]->bg = pixel_for_vec(COLOR_WHITE,0,8888);
		viewSplit[i]->bmap = bmap_createblack(pssm_res,pssm_res,14);		
		pShadow[i] = viewSplit[i]->bmap;
		viewSplit[i]->material = mtl_create();
		effect_load(viewSplit[i]->material,fx_depth);
	}

// increase the z buffer to cover the shadow maps
	bmap_zbuffer(bmap_createblack(maxv(screen_size.x,pssm_res),maxv(screen_size.y,pssm_res),32));
	//effect_load(mat_shadow,"st_stencilBlur.fx"); // poisson blur - not really needed

// PSSM main loop
	while(pssm_numsplits>0)
	{
// update the views _after_ the camera was updated!
		proc_mode = PROC_LATE;

// set up the split distances and the shadow view
		pssm_split(camera,pssm_numsplits,pssm_splitweight);
		pssm_viewcpy(camera,viewShadow);

// set up the split view transformation matrices
		D3DXMATRIX matSplit[4]; 
		for(i=0; i<pssm_numsplits; i++) 
		{
// look from the sun onto the scene			
			viewSplit[i]->pan = 180 + sun_angle.pan;
			viewSplit[i]->tilt = -sun_angle.tilt;
			vec_set(viewSplit[i]->x,sun_pos);

// calculate the split view clipping borders and transformation matrix
			view_to_split_custom(camera,pssm_splitdist[i], pssm_splitdist[i+1], pssm_fbias*pow(2, i), viewSplit[i], &matSplit[i]);
			
// create a texture matrix from the split view proj matrix			
			D3DXMatrixMultiply(&matSplit[i],&matSplit[i],pssm_texscale(pssm_res));
		
#ifdef DEBUG_PSSM
			DEBUG_BMAP(viewSplit[i]->bmap,300 + i*220,0.2);
			var pssm_fps = 16/time_frame;
			DEBUG_VAR(pssm_fps,200);
			DEBUG_VAR(pssm_splitdist[i+1],220 + i*20);
#endif
		}

// use a DX function to copy the 4 texture matrices to the shadow shader
		LPD3DXEFFECT fx = viewShadow->material->d3deffect; 
		if(fx) fx->SetMatrixArray("matTex",matSplit,pssm_numsplits);

#ifdef DEBUG_PSSM		
		DEBUG_BMAP(viewShadow->bmap,20,0.2);
#endif
		wait(1);
	}

// disable pssm, clean up and remove all objects	
	ptr_remove(viewShadow->material);
	ptr_remove(viewShadow->bmap);
	ptr_remove(viewShadow);
	render_stencil = NULL;
	for(i=0; i<numsplits; i++) {
		ptr_remove(viewSplit[i]->material);
		ptr_remove(viewSplit[i]->bmap);
		ptr_remove(viewSplit[i]);
	}
}
#endif