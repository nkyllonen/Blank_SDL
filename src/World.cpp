#include "World.h"

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

World::World(int w, int h)
{
	width = w;
	height = h;
}

World::~World()
{
	delete[] modelData;
	delete floor;
	delete obj;
}

void World::init()
{
	//initialize floor
	floor = new WorldObject(Vec3D(0,-0.5*height - 2, 0));
	floor->setVertexInfo(CUBE_START, CUBE_VERTS);

	Material mat = Material();
	mat.setAmbient(glm::vec3(0.7, 0.7, 0.7));
	mat.setDiffuse(glm::vec3(0.7, 0.7, 0.7));
	mat.setSpecular(glm::vec3(0, 0, 0));

	floor->setMaterial(mat);
	floor->setSize(Vec3D(width*5, 0.1, width)); //xz plane

	//initialize obj cylinder
	obj = new WorldObject(Vec3D(0,-3,0));
	obj->setVertexInfo(0, total_obj_triangles);
	obj->setMaterial(mat);
	obj->setSize(Vec3D(1,1,1));
	obj->hasIBO = true;
}

/*----------------------------*/
// SETTERS
/*----------------------------*/

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
	CUBE_START = 0;
	CUBE_VERTS = 0;
	float* cubeData = util::loadModel("models/cube.txt", CUBE_VERTS);
	cout << "\nNumber of vertices in cube model : " << CUBE_VERTS << endl;
	cout << "--------------------------------------------------" << endl;
	total_model_verts += CUBE_VERTS;

	//SPHERE
	SPHERE_START = CUBE_VERTS;
	SPHERE_VERTS = 0;
	float* sphereData = util::loadModel("models/sphere.txt", SPHERE_VERTS);
	cout << "\nNumber of vertices in sphere model : " << SPHERE_VERTS << endl;
	cout << "--------------------------------------------------" << endl;
	total_model_verts += SPHERE_VERTS;

	/////////////////////////////////
	//BUILD MODELDATA ARRAY
	/////////////////////////////////
	if (!(cubeData != nullptr && sphereData != nullptr))
	{
		delete[] cubeData;
		delete[] sphereData;
		return false;
	}
	modelData = new float[total_model_verts * 8];
	//copy data into modelData array
	copy(cubeData, cubeData + CUBE_VERTS * 8, modelData);
	copy(sphereData, sphereData + SPHERE_VERTS * 8, modelData + (CUBE_VERTS * 8));
	delete[] cubeData;
	delete[] sphereData;

	/////////////////////////////////
	//LOAD IN OBJ
	/////////////////////////////////
	if (!TinyOBJLoad("models/cylinder.obj", "models/", obj_attrib, obj_shapes, obj_materials))
	{
		return false;
	}

	total_obj_triangles = (int)obj_attrib.vertices.size() / 3;
	cout << "\nOBJ loaded successfully" << endl;
	cout << "--------------------------------------------------" << endl;
	return true;
}

//
bool World::setupGraphics()
{
	/////////////////////////////////
	//BUILD MODEL VAO
	/////////////////////////////////
	//This stores the VBO and attribute mappings in one object
	glGenVertexArrays(1, &model_vao); //Create a model_vao
	glBindVertexArray(model_vao); //Bind the above created model_vao to the current context

	/////////////////////////////////
	//BUILD MODEL VBO
	/////////////////////////////////
	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	glGenBuffers(1, model_vbo);  //Create 1 buffer called model_vbo
	glBindBuffer(GL_ARRAY_BUFFER, model_vbo[0]); //Set the model_vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, total_model_verts * 8 * sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to model_vbo

	/////////////////////////////////
	//SETUP SHADERS
	/////////////////////////////////
	phongProgram = util::LoadShader("Shaders/phongTex.vert", "Shaders/phongTex.frag");

	//load in textures
	tex0 = util::LoadTexture("textures/wood.bmp");
	tex1 = util::LoadTexture("textures/grey_stones.bmp");

	if (tex0 == -1 || tex1 == -1 || phongProgram == -1)
	{
		cout << "\nCan't load texture(s)" << endl;
		printf(strerror(errno));
		return false;
	}

	//Tell OpenGL how to set shader input (how the data in the VBO is organized)
	GLint posAttrib = glGetAttribLocation(phongProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0); //Attribute, vals/attrib., type, normalized?, stride, offset
	glEnableVertexAttribArray(posAttrib);

	GLint texAttrib = glGetAttribLocation(phongProgram, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	GLint normAttrib = glGetAttribLocation(phongProgram, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	glBindVertexArray(0); //Unbind the vao in case we want to create a new one

	/////////////////////////////////
	//BUILD OBJ VAO
	/////////////////////////////////
	glGenVertexArrays(1, &obj_vao);
	glBindVertexArray(obj_vao);

	/////////////////////////////////
	//BUILD OBJ VBOs + IBO
	/////////////////////////////////
	glGenBuffers(3, obj_vbos);

	//1.1 POSITIONS --> obj_vbos[0]
	glBindBuffer(GL_ARRAY_BUFFER, obj_vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, obj_attrib.vertices.size() * sizeof(float), &obj_attrib.vertices.at(0), GL_STATIC_DRAW);

	//1.2 setup position attributes --> need to set now while obj_vbos[0] is bound
	GLint obj_posAttrib = glGetAttribLocation(phongProgram, "position");
	glVertexAttribPointer(obj_posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(obj_posAttrib);

	//2.1 NORMALS --> obj_vbos[1]
	if (obj_attrib.normals.size() == 0)
	{
		//fill with (0,0,0) normal vectors
		for (int i = 0; i < total_obj_triangles*3; i++)	//same number of normals as vertices
		{
			obj_attrib.normals.push_back(0);
			obj_attrib.normals.push_back(0);
			obj_attrib.normals.push_back(0);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj_vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, obj_attrib.normals.size() * sizeof(float), &obj_attrib.normals.at(0), GL_STATIC_DRAW);

	//2.2 setup normal attributes
	GLint obj_normAttrib = glGetAttribLocation(phongProgram, "inNormal");
	glVertexAttribPointer(obj_normAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(obj_normAttrib);

	//3.1 TEXCOORDS --> obj_vbos[2]
	if (obj_attrib.texcoords.size() == 0)
	{
		//fill with (-1, -1) text coords --> no texture
		for (int i = 0; i < total_obj_triangles*3; i++)	//same number of texcoords as vertices
		{
			obj_attrib.texcoords.push_back(-1);
			obj_attrib.texcoords.push_back(-1);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj_vbos[2]);
	glBufferData(GL_ARRAY_BUFFER, obj_attrib.texcoords.size() * sizeof(float), &obj_attrib.texcoords.at(0), GL_STATIC_DRAW);

	//3.2 setup texcoord attributes
	GLint obj_texAttrib = glGetAttribLocation(phongProgram, "inTexcoord");
	glVertexAttribPointer(obj_texAttrib, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glEnableVertexAttribArray(obj_texAttrib);

	//4. INDICES --> obj_ibo
	glGenBuffers(1, obj_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj_ibo[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj_shapes.at(0).mesh.indices.size() * sizeof(int), &obj_shapes.at(0).mesh.indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	return true;
}

//loops through WObj array and draws each
//also draws floor
void World::draw(Camera * cam)
{
	glClearColor(.2f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(phongProgram); //Set the active shader (only one can be used at a time)

	//vertex shader uniforms
	GLint uniView = glGetUniformLocation(phongProgram, "view");
	GLint uniProj = glGetUniformLocation(phongProgram, "proj");
	GLint uniTexID = glGetUniformLocation(phongProgram, "texID");

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
	glUniform1i(glGetUniformLocation(phongProgram, "tex0"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex1);
	glUniform1i(glGetUniformLocation(phongProgram, "tex1"), 1);

	glBindVertexArray(model_vao);
	glBindBuffer(GL_ARRAY_BUFFER, model_vbo[0]); //Set the model_vbo as the active VBO
	glUniform1i(uniTexID, 1);

	floor->draw(phongProgram);

	//draw obj cylinder
	glBindVertexArray(obj_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj_ibo[0]);
	glUniform1i(uniTexID, -1);

	obj->draw(phongProgram);
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
  cout << "\nLoading " << filename << "......"<< endl;

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

	for (int s = 0; s < shapes.size(); s++)
	{
		printf("-->shapes[%i] : %s\n", s, shapes.at(s).name.c_str());
		printf("----> # of indices = %d\n", (int)shapes.at(s).mesh.indices.size());
	}

  return true;
}
