/*
Title: Particles
File Name: Main.cpp
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

#include "glew\glew.h"
#include "glfw\glfw3.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <Windows.h>

// This is your reference to your shader program.
// This will be assigned with glCreateProgram().
// This program will run on your GPU.
GLuint program;

// These are your references to your actual compiled shaders
GLuint vertex_shader;
GLuint fragment_shader;

// A reference to the data we will use for vertex locations.
GLuint positionBuffer;
GLuint positionBufferTexture;

GLuint velocityBuffer;
GLuint velocityBufferTexture;

GLuint transformData;

// Number of triangles we find in the model file.
int numTriangles;
int numVerts;

// These are 4x4 transformation matrices, which you will locally modify before passing into the vertex shader via uniMVP
glm::mat4 trans;
glm::mat4 proj;
glm::mat4 view;

glm::mat4 MVP;

float* uniformBufferData;

// This runs once a frame, before renderScene
void update()
{
    static float orbit = 0;
    orbit += 0.005f;
    const float radius = 30.5f;
    view = glm::lookAt( glm::vec3( sin( orbit )*radius, 20.0f, -cos( orbit )*radius ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );


    MVP = proj * view * trans;
}

// This function runs every frame
void renderScene()
{
    // This sets our uniform data every frame. This is how most game engines set
    // uniform values. This allows you to set up a small block of memory and
    // write whatever you need to it. The limit on use of this space is around
    // 64KB; for anything much larger than a few KB, use textures or buffers.
    glBindBuffer( GL_UNIFORM_BUFFER, transformData );
    memcpy( &uniformBufferData[0], &MVP, sizeof( float ) * 16 );
    glBufferData( GL_UNIFORM_BUFFER, sizeof( float ) * 16 + sizeof( int ) * 4 * 512, uniformBufferData, GL_DYNAMIC_DRAW );
    glBindBufferBase( GL_UNIFORM_BUFFER, 0, transformData );

    // Clear the color buffer and the depth buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Clear the screen to a neutral gray
    glClearColor( 0.0, 0.0, 0.0, 1.0 );

    // Tell OpenGL to use the shader program you've created.
    glUseProgram( program );



    // Bind the texture to texture unit 0 so we can attach it to a sampler.
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, positionBufferTexture );

    // Bind the same texture to image unit 0 so we can write to it in the shader.
    // Notably, this is a separate set of indices from texture units!!! As such, 
    // we can bind it to 0, but it is a different 0 from the texture unit index
    // set above. OpenGL is just wonderfully ambiguous like that. More info on the function:
    // https://www.opengl.org/sdk/docs/man/html/glBindImageTexture.xhtml
    glBindImageTexture( 0, positionBufferTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );



    // Same as above, but with the velocity data for our particles.
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, velocityBufferTexture );
    glBindImageTexture( 1, velocityBufferTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F );

    // Draw our particles as points.
    glDrawArrays( GL_POINTS, 0, numVerts );

}


std::string readShader( std::string fileName )
{
    std::string shaderCode;
    std::string line;

    std::ifstream file( fileName, std::ios::in );

    if( !file.good() )
    {
        std::cout << "Can't read file: " << fileName.data() << std::endl;

        return "";
    }


    file.seekg( 0, std::ios::end );					// Moves the "get" position to the end of the file.
    shaderCode.resize( (unsigned int)file.tellg() );	// Resizes the shaderCode string to the size of the file being read, given that tellg will give the current "get" which is at the end of the file.
    file.seekg( 0, std::ios::beg );					// Moves the "get" position to the start of the file.

    file.read( &shaderCode[0], shaderCode.size() );

    file.close(); // Now that we're done, close the file and return the shaderCode.

    return shaderCode;
}


GLuint createShader( std::string sourceCode, GLenum shaderType )
{
    GLuint shader = glCreateShader( shaderType );
    const char *shader_code_ptr = sourceCode.c_str(); // We establish a pointer to our shader code string
    const int shader_code_size = sourceCode.size();   // And we get the size of that string.

    glShaderSource( shader, 1, &shader_code_ptr, &shader_code_size );
    glCompileShader( shader ); // This just compiles the shader, given the source code.

    GLint isCompiled = 0;

    glGetShaderiv( shader, GL_COMPILE_STATUS, &isCompiled );

    if( isCompiled == GL_FALSE )
    {
        char infolog[1024];
        glGetShaderInfoLog( shader, 1024, NULL, infolog );

        std::cout << "The shader failed to compile with the error:" << std::endl << infolog << std::endl;

        glDeleteShader( shader ); // Don't leak the shader.
    }

    return shader;
}

void initializeParticlePositions( float** positionDataOut, int* nPositionsLoadedOut )
{

    std::vector< float > positionData;

    for( int i = 0; i < 4000000; i++ )
    {
        positionData.push_back( (((float)(rand() % 1000)) / 50.0f) - 10.0f );
        positionData.push_back( (((float)(rand() % 1000)) / 50.0f) - 10.0f );
        positionData.push_back( (((float)(rand() % 1000)) / 50.0f) - 10.0f );
        positionData.push_back( 1.0 );
    }

    // 3 pieces of data per position
    *nPositionsLoadedOut = positionData.size() / 4;
    *positionDataOut = new float[positionData.size()];
    memcpy( *positionDataOut, &positionData[0], positionData.size() * sizeof( float ) );
}


void initBuffers()
{
    GLuint buffers[3];
    glGenBuffers( 3, buffers );
    positionBuffer = buffers[0];
    velocityBuffer = buffers[1];
    transformData = buffers[2];

    GLuint textures[2];
    glGenTextures( 2, textures );
    positionBufferTexture = textures[0];
    velocityBufferTexture = textures[1];

    int nPositionsLoaded;
    float* positionData;

    initializeParticlePositions( &positionData, &nPositionsLoaded );
    numVerts = nPositionsLoaded;


    glBindBuffer( GL_TEXTURE_BUFFER, positionBuffer );
    glBufferData( GL_TEXTURE_BUFFER, sizeof( float ) * 4 * nPositionsLoaded, positionData, GL_STATIC_DRAW );

    glBindTexture( GL_TEXTURE_BUFFER, positionBufferTexture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, positionBuffer );       // We need to retrieve 3 floats for a position, so store them as 4 channel



    // Just initialize our velocity data to 0s. Fun fact: 0 in float is the same
    // underlying data as 0 in integers, so memset works for that.
    char* velocityData = new char[sizeof( float ) * 2 * nPositionsLoaded];
    memset( velocityData, 0, sizeof( float ) * 2 * nPositionsLoaded );

    glBindBuffer( GL_TEXTURE_BUFFER, velocityBuffer );
    glBufferData( GL_TEXTURE_BUFFER, sizeof( float ) * 2 * nPositionsLoaded, velocityData, GL_STATIC_DRAW );

    glBindTexture( GL_TEXTURE_BUFFER, velocityBufferTexture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA16F, velocityBuffer );       // We need to retrieve 3 floats for a velocity, so store them as 4 channel

    srand( 49770 );
    int* randomWind = new int[512 * 4];
    for( int i = 0; i < 512; i++ )
    {
        randomWind[i * 4] = rand() % 1000;  // This will be our random wind direction for our particles; we will use it
                                            // based on location, then extract a direction from each decimal place. We
                                            // could use floating point values in a vec3, but there is a question which needs begging.
                                            // Which leads me to:

        randomWind[i * 4 + 1] = 0;          // Q: But wait: what are we feeding all these 0s into a buffer for?
        randomWind[i * 4 + 2] = 0;          // A: Because each element in a uniform buffer sent to the GPU is
        randomWind[i * 4 + 3] = 0;          // 16 byte aligned. Or in other words, the int[512] this is being
    }                                       // fed to is actually int4[512] under the hood. In terms of memory,
                                            // this is wasteful, but it would otherwise require additional
                                            // computation in the GPU to access, and so it leaves it to you to
                                            // decide whether the cost of extracting from 16 byte structures
                                            // is worth the extra memory saved.


    uniformBufferData = new float[16 + 512 * 4];
    memcpy( &uniformBufferData[16], randomWind, sizeof( int ) * 512 * 4 );

    delete[] velocityData;
    delete[] randomWind;
}

// Initialization code
void init()
{
    glewInit();



    std::string vertShader = readShader( "ParticleShader.glsl" );
    std::string fragShader = readShader( "FragmentShader.glsl" );


    vertex_shader = createShader( vertShader, GL_VERTEX_SHADER );
    fragment_shader = createShader( fragShader, GL_FRAGMENT_SHADER );

    program = glCreateProgram();
    glAttachShader( program, vertex_shader );		// This attaches our vertex shader to our program.
    glAttachShader( program, fragment_shader );	// This attaches our fragment shader to our program.
    glLinkProgram( program );


    initBuffers();


    view = glm::lookAt( glm::vec3( 0.0f, 0.0f, -1.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    proj = glm::perspective( 45.0f, 800.0f / 600.0f, 0.1f, 1000.0f );
    trans = glm::translate( trans, glm::vec3( 0.0, 0.0, 0.0 ) );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
}

int main( int argc, char **argv )
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow( 1200, 900, "Particles!", nullptr, nullptr );

    glfwMakeContextCurrent( window );

    glfwSwapInterval( 1 );

    init();

    unsigned long f;
    QueryPerformanceFrequency( (LARGE_INTEGER*)&f );
    double timerFrequency = (1.0 / f);
    unsigned long startTime;
    unsigned long endTime;

    unsigned long frameTime;
    unsigned long frameEndTime;
    double longestFrame;

    int FrameCounter = 0;
    int interval = 100;
    while( !glfwWindowShouldClose( window ) )
    {
        QueryPerformanceCounter( (LARGE_INTEGER *)&frameTime );
        if( FrameCounter % interval == 0 )
        {
            QueryPerformanceCounter( (LARGE_INTEGER *)&startTime );
            longestFrame = 0;
        }

        update();

        renderScene();

        // Waits for GPU commands to finish before proceeding; beneficial in this case.
        glFinish();

        glfwSwapBuffers( window );

        glfwPollEvents();

        QueryPerformanceCounter( (LARGE_INTEGER *)&frameEndTime );
        if( ((frameEndTime - frameTime) * timerFrequency) > longestFrame )
            longestFrame = ((frameEndTime - frameTime) * timerFrequency);

        if( FrameCounter % interval == interval - 1 )
        {
            QueryPerformanceCounter( (LARGE_INTEGER *)&endTime );
            double timeDifference = ((endTime - startTime) * timerFrequency);
            std::cout << (interval - 1) / (timeDifference + 0.000001) << " Avg : " << 1.0 / (longestFrame + 0.000001) << " Longest" << std::endl;
        }
        FrameCounter++;
    }

    glDeleteShader( vertex_shader );
    glDeleteShader( fragment_shader );
    glDeleteProgram( program );


    glfwTerminate();

    return 0;
}