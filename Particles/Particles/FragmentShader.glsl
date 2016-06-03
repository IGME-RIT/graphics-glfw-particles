/*
Title: Particles
File Name: FragmentShader.glsl
Copyright © 2015
Original authors: Joshua Alway
Written under the supervision of David I. Schwartz, Ph.D., and
supported by a professional development seed grant from the B. Thomas
Golisano College of Computing & Information Sciences
(https://www.rit.edu/gccis) at the Rochester Institute of Technology.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Description:
This program draws 4 million point particles, updating their velocities 
and positions on the GPU in the vertex shader. It introduces uniform buffers, 
instead of setting uniform values. Uses imageStore to write velocity and position 
back into their respective textures. Also has FPS count in console.
*/

#version 440 core // Identifies the version of the shader, this line must be on a separate line from the rest of the shader code

layout(location = 0) out vec4 out_color; // Establishes the variable we will pass out of this shader.

in vec4 color;	// Take in a vec4 for color
 
void main(void)
{
    float modifier = 0.15;  // Modify the color by this constant; just dims things down, since there are a lot of particles and
                            // they are additively blended.
	out_color = vec4(color.rgb, 1.0) * modifier;
}