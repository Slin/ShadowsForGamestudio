//The MIT License (MIT)
//
//Copyright (c) 2015 Nils Daumann
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

void view_to_split_custom(VIEW *frustview, var nearplane, var farplane, VIEW *split, float *matSplit)
{
	VECTOR nearcenter;
	VECTOR farleft;
	VECTOR farright;
	VECTOR center;
	
	nearcenter.x = screen_size.x*0.5;
	nearcenter.y = screen_size.y*0.5;
	nearcenter.z = nearplane;
	vec_for_screen(&nearcenter, frustview);
		
	farleft.x = 0.0;
	farleft.y = 0.0;
	farleft.z = farplane;
	vec_for_screen(&farleft, frustview);
	
	farright.x = screen_size.x;
	farright.y = screen_size.y;
	farright.z = farplane;
	vec_for_screen(&farright, frustview);
	
	vec_set(&center, &farleft);
	vec_add(&center, &farright);
	vec_scale(&center, 0.5);
	vec_add(&center, &nearcenter);
	vec_scale(&center, 0.5);
	
	float dist = floor(vec_dist(center, farleft)+0.5)+1.0;
	
	split->left = -dist;
	split->right = dist;
	split->bottom = -dist;
	split->top = dist;
	split->clip_far = 100000;
	split->clip_near = 1;
	
	vec_set(split.x, vector(-10000.0, 0.0, 0.0));
	vec_rotate(split.x, vector(sun_angle.pan+180, -sun_angle.tilt, 0));
	vec_add(split.x, &center);
	
	//--------------------------------------------------------
	//move light view in pixel steps, to remove flickering
	D3DXMATRIX rot, invrot;
	ang_to_matrix(vector(sun_angle.pan+180, -sun_angle.tilt, 0), &rot);
	D3DXMatrixInverse(invrot, NULL, rot);
	float pxsize = 2.0*dist/(float)pssm_res;
	D3DXVECTOR4 pos;
	D3DXVECTOR3 pos3;
	pos3.x = split.x;
	pos3.y = split.z;
	pos3.z = split.y;
	D3DXVec3Transform(&pos, &pos3, &invrot);
	pos3.x = pos.x;
	pos3.y = pos.y;
	pos3.z = pos.z;
	D3DXVec3Scale(&pos3, &pos3, 1.0/pxsize);
	pos3.x = floor(pos3.x);
	pos3.y = floor(pos3.y);
	pos3.z = floor(pos3.z);
	D3DXVec3Scale(&pos3, &pos3, pxsize);
	D3DXVec3Transform(&pos, &pos3, &rot);
	split.x = pos.x;
	split.y = pos.z;
	split.z = pos.y;
	//--------------------------------------------------------
	
	D3DXMATRIX matview;
	D3DXMATRIX matproj;
	view_to_matrix(split, &matview, &matproj);
	D3DXMatrixMultiply(matSplit, &matview, &matproj);
}
