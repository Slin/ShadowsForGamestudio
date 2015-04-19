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

#include <acknex.h>
#include <default.c>

#include "shadows/shadows.c"

void main()
{
	level_load(NULL);
	vec_set(sky_color, vector(255, 200, 100));
	
	sun_light = 100;
	
	vec_set(camera.x, vector(0, 0, 10));
	camera.clip_near = 0.1;
	camera.clip_far = 1500;
	
	ENTITY *ground = ent_create("resources/cube.mdl", vector(0, 0, -1), NULL);
	set(ground, SHADOW);
	vec_set(ground.scale_x, vector(100, 100, 1.0/8.0));
	
	int x;
	int y;
	for(x = -800; x < 800; x += 100)
	{
		for(y = -800; y < 800; y += 100)
		{
			ENTITY *box = ent_create("cube.mdl", vector(x, y, 8), NULL);
			set(box, SHADOW);
		}
	}
	
	pssm_run(4);
}
