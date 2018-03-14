#ifndef WORLD_INCLUDED
#define WORLD_INCLUDED

#include "glad.h"  //Include order can matter here

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#elif __linux__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include "Vec3D.h"
#include "Camera.h"
#include "Util.h"
#include "WorldObject.h"

#include "timerutil.h"

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

class World{
private:
	int width;
	int height;

	//modelData
	int total_model_verts = 0;
	float* modelData;
	int CUBE_START = 0;
	int CUBE_VERTS = 0;
	int SPHERE_START = 0;
	int SPHERE_VERTS = 0;

	//OBJ data

	//VAO and VBO GLuints
	GLuint model_vao;
	GLuint model_vbo[1];	//interleaved vert pos, tex, norm
	GLuint obj_vao;
	GLuint obj_vbo[2];		//vertices and normals

	//Shader and Texture GLuints
	GLuint shaderProgram;
	GLuint tex0;
	GLuint tex1;

	//PRIVATE FUNCTIONS


public:
	//CONSTRUCTORS AND DESTRUCTORS
	World();
	World(int w, int h);
	~World();

	//SETTERS

	//GETTERS
	int getWidth();
	int getHeight();

	//OTHERS
	bool loadModelData();
	bool setupGraphics();
	void draw(Camera * cam);

};

#endif
