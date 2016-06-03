/*
Title: Particles
File Name: ParticleShader.glsl
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

#version 440 core

out vec4 color;

layout(binding = 0) uniform TransformationData 
{
    mat4 MVP;
    int randomWind[512];
};

layout(binding = 0) uniform samplerBuffer positionBufferTexture;
layout(binding = 0) writeonly uniform imageBuffer positionBufferTextureOut;

layout(binding = 1) uniform samplerBuffer velocityBufferTexture;
layout(binding = 1) writeonly uniform imageBuffer velocityBufferTextureOut;

void main(void)
{
    int i = int(gl_VertexID);

    vec3 pos = texelFetch( positionBufferTexture, i ).rgb;
    vec3 vel = texelFetch( velocityBufferTexture, i ).rgb;

    float dt = 0.001;
    
    // This math breaks the space (20.0 x 40.0 x 20.0, centered on the origin) up into an 8x8x8 grid of 2.5 x 5 x 2.5 blocks.
    // The equations are slightly padded, in float point error pushes them just over the exact exterior bounds of the grid.
    // Reads into invalid addresses on the GPU is bad (and writes into invalid addresses, even worse).
    // Then we use that location to get an index into the random wind stored in the uniform data.
    int blockIndex =            int((pos.x + 10.01f) * 8.0f/20.02f) +
                                int((pos.y + 20.01f) * 8.0f/40.02f) * 8 +
                                int((pos.z + 10.01f) * 8.0f/20.02f) * 64;

    int Wind = randomWind[blockIndex];

    // Wind is a random number between (and including) 0 and 999; here
    // we grab one digit of that for each dimension, giving a float ranging
    // from -4.5 to 4.5 for each direction.
    vec3 WindDir = vec3( float(Wind % 10) - 4.5f, float((Wind / 10) % 10) - 4.5f, float((Wind / 100) % 10) - 4.5f );
    vel.xyz += WindDir.xyz * 0.01;

    // Add a bit of gravity, with some particles 'heavier' than others.
    vel.y -= dt * 10.0 + float(i) * 0.000001 * dt;

    // If we exceed a certain speed (10.0), renormalize to that speed.
    float speed = length(vel);
    if( speed > 10.0 )
        vel = vel / speed * 10.0;

    // Add velocity to position.
    pos.xyz += vel.xyz*dt;

    // Make the position wrap around; this keeps it in the 
    // 20.0 x 40.0 x 20.0 space centered on the origin.
    // Otherwise, they would all be blown outside the initial box, 
    // and result in a boring demo in short order.
    pos.x = mod(pos.x + 10, 20) - 10.0;
    pos.y = mod(pos.y + 20.0, 40.0) - 20.0;
    pos.z = mod(pos.z + 10, 20) - 10.0;

    // imageStore allows you to write out to an image from within a shader.
    // Parameters are reasonably self-explanatory; image, location, value.
    // https://www.opengl.org/sdk/docs/man/html/imageStore.xhtml
    // There is also an imageLoad function: this is called exactly like texelFetch,
    // and with much the same results.
    imageStore( positionBufferTextureOut, i, vec4(pos, 1.0) );
    imageStore( velocityBufferTextureOut, i, vec4(vel, 1.0f) );



    vec4 ClipSpace = MVP * vec4( pos.xyz, 1.0 );

    // Color this particle based on its direction of motion.
    color = vec4(normalize(abs(vel.xzy)), 1.0);
    gl_Position = ClipSpace;
}													 

