#include "World.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace std;

//HELPER FUNCTION DECLARATIONS
static bool TinyOBJLoad(const char* filename, const char* basepath, tinyobj::attrib_t &attrib,
												vector<tinyobj::shape_t> &shapes, vector<tinyobj::material_t> &materials);

/*----------------------------*/
// CONSTRUCTORS AND DESTRUCTORS
/*----------------------------*/
World::World()
{
	width = 0;
	height = 0;
}

World::World(int w, int h, int num)
{
	width = w;
	height = h;
	num_objects = num;
}

World::~World()
{
	delete[] modelData;
	//delete floor;
}

/*----------------------------*/
// SETTERS
/*----------------------------*/
void World::setCubeIndices(int start, int tris)
{
	CUBE_START = start;
	CUBE_VERTS = tris;
}

void World::setSphereIndices(int start, int tris)
{
	SPHERE_START = start;
	SPHERE_VERTS = tris;
}

/*----------------------------*/
// GETTERS
/*----------------------------*/
int World::getWidth()
{
	return width;
}

int World::getHeight()
{
	return height;
}

/*----------------------------*/
// OTHERS
/*----------------------------*/
//load in all models and store data into the modelData array
bool World::loadModelData()
{
	/////////////////////////////////
	//LOAD IN MODELS
	/////////////////////////////////
	//CUBE
	int CUBE_VERTS = 0;
	float* cubeData = util::loadModel("models/cube.txt", CUBE_VERTS);
	cout << "\nNumber of vertices in cube model : " << CUBE_VERTS << endl;
	total_verts += CUBE_VERTS;
	setCubeIndices(0, CUBE_VERTS);

	//SPHERE
	int SPHERE_VERTS = 0;
	float* sphereData = util::loadModel("models/sphere.txt", SPHERE_VERTS);
	cout << "\nNumber of vertices in sphere model : " << SPHERE_VERTS << endl;
	total_verts += SPHERE_VERTS;
	setSphereIndices(CUBE_VERTS, SPHERE_VERTS);

	/////////////////////////////////
	//BUILD MODELDATA ARRAY
	/////////////////////////////////
	if (!(cubeData != nullptr && sphereData != nullptr))
	{
		delete[] cubeData;
		delete[] sphereData;
		return false;
	}
	modelData = new float[total_verts * 8];
	//copy data into modelData array
	copy(cubeData, cubeData + CUBE_VERTS * 8, modelData);
	copy(sphereData, sphereData + SPHERE_VERTS * 8, modelData + (CUBE_VERTS * 8));
	delete[] cubeData;
	delete[] sphereData;

	/////////////////////////////////
	//LOAD IN OBJ
	/////////////////////////////////
	tinyobj::attrib_t obj_attrib;
	vector<tinyobj::shape_t> obj_shapes;
	vector<tinyobj::material_t> obj_materials;

	if (!TinyOBJLoad("models/cylinder.obj", "models/", obj_attrib, obj_shapes, obj_materials))
	{
		return false;
	}

	cout << "\nOBJ loaded successfully" << endl;


	return true;
}

//
bool World::setupGraphics()
{
	/////////////////////////////////
	//BUILD VERTEX ARRAY OBJECT
	/////////////////////////////////
	//This stores the VBO and attribute mappings in one object
	glGenVertexArrays(1, &model_vao); //Create a model_vao
	glBindVertexArray(model_vao); //Bind the above created model_vao to the current context

	/////////////////////////////////
	//BUILD VERTEX BUFFER OBJECT
	/////////////////////////////////
	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	glGenBuffers(1, model_vbo);  //Create 1 buffer called model_vbo
	glBindBuffer(GL_ARRAY_BUFFER, model_vbo[0]); //Set the model_vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, total_verts * 8 * sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to model_vbo

	/////////////////////////////////
	//SETUP SHADERS
	/////////////////////////////////
	shaderProgram = util::LoadShader("Shaders/phongTex.vert", "Shaders/phongTex.frag");

	//load in textures
	tex0 = util::LoadTexture("textures/wood.bmp");
	tex1 = util::LoadTexture("textures/grey_stones.bmp");

	if (tex0 == -1 || tex1 == -1 || shaderProgram == -1)
	{
		cout << "\nCan't load texture(s)" << endl;
		printf(strerror(errno));
		return false;
	}

	//Tell OpenGL how to set fragment shader input
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	//Attribute, vals/attrib., type, normalized?, stride, offset
	//Binds to VBO current GL_ARRAY_BUFFER
	glEnableVertexAttribArray(posAttrib);

	GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0); //Unbind the model_vao in case we want to create a new one

	glEnable(GL_DEPTH_TEST);
	return true;
}

//loops through WObj array and draws each
//also draws floor
void World::draw(Camera * cam)
{
	glClearColor(.2f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram); //Set the active shader (only one can be used at a time)

	//vertex shader uniforms
	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");

	//build view matrix from Camera
	glm::mat4 view = glm::lookAt(
		util::vec3DtoGLM(cam->getPos()),
		util::vec3DtoGLM(cam->getPos() + cam->getDir()),  //Look at point
		util::vec3DtoGLM(cam->getUp()));

	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 proj = glm::perspective(3.14f / 4, 800.0f / 600.0f, 0.1f, 100.0f); //FOV, aspect, near, far
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex0"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex1);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex1"), 1);

	glBindVertexArray(model_vao);
	glUniform1i(uniTexID, -1);


}

/*----------------------------*/
// PRIVATE FUNCTIONS
/*----------------------------*/

/*----------------------------*/
// HELPER FUNCTIONS
/*----------------------------*/
static bool TinyOBJLoad(const char* filename, const char* basepath, tinyobj::attrib_t &attrib,
												vector<tinyobj::shape_t> &shapes, vector<tinyobj::material_t> &materials)
{
  cout << "\nLoading " << filename << endl;

  timerutil t;
  t.start();
  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename,
                              basepath, true);
  t.end();
  printf("Parsing time: %lu [msecs]\n", t.msec());

  if (!err.empty()) {
    cerr << err << endl;
  }

  if (!ret) {
    printf("Failed to load/parse .obj.\n");
    return false;
  }

	printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
  printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
  printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
  printf("# of materials = %d\n", (int)materials.size());
  printf("# of shapes    = %d\n", (int)shapes.size());

  return true;
}
