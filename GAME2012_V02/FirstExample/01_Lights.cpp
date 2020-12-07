
///////////////////////////////////////////////////////////////////////
//
// 01_JustAmbient.cpp
//
///////////////////////////////////////////////////////////////////////

using namespace std;

#include <cstdlib>
#include <ctime>
#include "vgl.h"
#include "LoadShaders.h"
#include "Light.h"
#include "Shape.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define MOVESPEED 0.3f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,1,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)
#define XYZ_AXIS glm::vec3(0,0,0)

enum keyMasks {
	KEY_FORWARD =  0b00000001,		// 0x01 or 1 or 01
	KEY_BACKWARD = 0b00000010,		// 0x02 or 2 or 02
	KEY_LEFT = 0b00000100,		
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000
	// Any other keys you want to add.
};

// IDs.
GLuint vao, ibo, points_vbo, colors_vbo, uv_vbo, normals_vbo, modelID, viewID, projID;
GLuint program;

// Matrices.
glm::mat4 View, Projection;

// Our bitflags. 1 byte for up to 8 keys.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;

// Texture variables.
GLuint txt[14];
GLint width, height, bitDepth;

// Light variables.
AmbientLight aLight(glm::vec3(255.0f / 255.0f, 238.0f / 255.0f, 204.0f/255.0f),	// Ambient colour.
	0.7f);							// Ambient strength.

DirectionalLight dLight(glm::vec3(-1.0f, 0.0f, -0.5f), // Direction.
	glm::vec3(1.0f, 1.0f, 0.25f),  // Diffuse colour.
	0.0f);						  // Diffuse strength.

PointLight pLights[10] = {  { glm::vec3(6.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
						    { glm::vec3(54.5f, 2.5f, -5.0f), 5.5f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(55.0f, 10.1f, -10.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(36.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(16.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(54.0f, 0.1f, -21.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(6.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(6.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(6.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f },
							{ glm::vec3(6.0f, 0.1f, -4.0f), 5.0f, glm::vec3(1.0f, 0.8f, 0.6f), 5.0f }};

SpotLight sLight(glm::vec3(23.7001f, 69.2495f, - 25.1001f),	// Position.
	glm::vec3(1.0f, 1.0f, 1.0f),	// Diffuse colour.
	1.0f,							// Diffuse strength.
	glm::vec3(0.0f, -1.0f, 0.0f),  // Direction.
	15.0f);

void timer(int);

void resetView()
{
	position = glm::vec3(60.0f, 3.0f, -23.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = -5.0f;
	yaw = -180.0f;
	// View will now get set only in transformObject
}

// Shapes. Recommend putting in a map
Cube g_cube;
Prism g_prism(14);
//Plane g_plane;
Grid g_grid(55,15); // New UV scale parameter. Works with texture now.
Grid g_ground(44, 16); // New UV scale parameter. Works with texture now.



void init(void)
{
	srand((unsigned)time(NULL));
	//Specifying the name of vertex and fragment shaders.
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	//Loading and compiling shaders
	program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	modelID = glGetUniformLocation(program, "model");
	projID = glGetUniformLocation(program, "projection");
	viewID = glGetUniformLocation(program, "view");

	// Projection matrix : 45∞ Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	resetView();

	// Image loading.
	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load("id1_bricks.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[0]);
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	image = stbi_load("T_Tiles_D.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[7]);
	glBindTexture(GL_TEXTURE_2D, txt[7]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	image = stbi_load("T_Top_Wall.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[8]);
	glBindTexture(GL_TEXTURE_2D, txt[8]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);


	image = stbi_load("Wood.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[9]);
	glBindTexture(GL_TEXTURE_2D, txt[9]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	image = stbi_load("id6_roof_tall.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[10]);
	glBindTexture(GL_TEXTURE_2D, txt[10]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	image = stbi_load("T_abovewater_D.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[3]);
	glBindTexture(GL_TEXTURE_2D, txt[3]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	image = stbi_load("T_Top_Wall.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[4]);
	glBindTexture(GL_TEXTURE_2D, txt[4]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	// Second texture. Blank one.
	
	unsigned char* image2 = stbi_load("T_water_D.png", &width, &height, &bitDepth, 0);
	if (!image2) cout << "Unable to load file!" << endl;
	
	glGenTextures(1, &txt[1]);
	glBindTexture(GL_TEXTURE_2D, txt[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image2);
	
	image2 = stbi_load("T_GroundPlane_Pavement.png", &width, &height, &bitDepth, 0);
	if (!image2) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[5]);
	glBindTexture(GL_TEXTURE_2D, txt[5]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image2);

	unsigned char* image3 = stbi_load("T_leaves_D.png", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[2]);
	glBindTexture(GL_TEXTURE_2D, txt[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image3);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image3);

	 image3 = stbi_load("id3_roof.png", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;

	glGenTextures(1, &txt[6]);
	glBindTexture(GL_TEXTURE_2D, txt[6]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image3);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image3);


	glUniform1i(glGetUniformLocation(program, "texture0"), 0);

	// Setting ambient Light.
	glUniform3f(glGetUniformLocation(program, "aLight.ambientColour"), aLight.ambientColour.x, aLight.ambientColour.y, aLight.ambientColour.z);
	glUniform1f(glGetUniformLocation(program, "aLight.ambientStrength"), aLight.ambientStrength);

	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColour"), dLight.diffuseColour.x, dLight.diffuseColour.y, dLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

	// Setting point lights.
	{
		glUniform3f(glGetUniformLocation(program, "pLights[0].base.diffuseColour"), pLights[0].diffuseColour.x, pLights[0].diffuseColour.y, pLights[0].diffuseColour.z);
		glUniform1f(glGetUniformLocation(program, "pLights[0].base.diffuseStrength"), pLights[0].diffuseStrength);
		glUniform3f(glGetUniformLocation(program, "pLights[0].position"), pLights[0].position.x, pLights[0].position.y, pLights[0].position.z);
		glUniform1f(glGetUniformLocation(program, "pLights[0].constant"), pLights[0].constant);
		glUniform1f(glGetUniformLocation(program, "pLights[0].linear"), pLights[0].linear);
		glUniform1f(glGetUniformLocation(program, "pLights[0].exponent"), pLights[0].exponent);

		glUniform3f(glGetUniformLocation(program, "pLights[1].base.diffuseColour"), pLights[1].diffuseColour.x, pLights[1].diffuseColour.y, pLights[1].diffuseColour.z);
		glUniform1f(glGetUniformLocation(program, "pLights[1].base.diffuseStrength"), pLights[1].diffuseStrength);
		glUniform3f(glGetUniformLocation(program, "pLights[1].position"), pLights[1].position.x, pLights[1].position.y, pLights[1].position.z);
		glUniform1f(glGetUniformLocation(program, "pLights[1].constant"), pLights[1].constant);
		glUniform1f(glGetUniformLocation(program, "pLights[1].linear"), pLights[1].linear);
		glUniform1f(glGetUniformLocation(program, "pLights[1].exponent"), pLights[1].exponent);

		glUniform3f(glGetUniformLocation(program, "pLights[2].base.diffuseColour"), pLights[2].diffuseColour.x, pLights[2].diffuseColour.y, pLights[2].diffuseColour.z);
		glUniform1f(glGetUniformLocation(program, "pLights[2].base.diffuseStrength"), pLights[2].diffuseStrength);
		glUniform3f(glGetUniformLocation(program, "pLights[2].position"), pLights[2].position.x, pLights[2].position.y, pLights[2].position.z);
		glUniform1f(glGetUniformLocation(program, "pLights[2].constant"), pLights[2].constant);
		glUniform1f(glGetUniformLocation(program, "pLights[2].linear"), pLights[2].linear);
		glUniform1f(glGetUniformLocation(program, "pLights[2].exponent"), pLights[2].exponent);

		glUniform3f(glGetUniformLocation(program, "pLights[3].base.diffuseColour"), pLights[3].diffuseColour.x, pLights[3].diffuseColour.y, pLights[3].diffuseColour.z);
		glUniform1f(glGetUniformLocation(program, "pLights[3].base.diffuseStrength"), pLights[3].diffuseStrength);
		glUniform3f(glGetUniformLocation(program, "pLights[3].position"), pLights[3].position.x, pLights[3].position.y, pLights[3].position.z);
		glUniform1f(glGetUniformLocation(program, "pLights[3].constant"), pLights[3].constant);
		glUniform1f(glGetUniformLocation(program, "pLights[3].linear"), pLights[3].linear);
		glUniform1f(glGetUniformLocation(program, "pLights[3].exponent"), pLights[3].exponent);

		glUniform3f(glGetUniformLocation(program, "pLights[4].base.diffuseColour"), pLights[4].diffuseColour.x, pLights[4].diffuseColour.y, pLights[4].diffuseColour.z);
		glUniform1f(glGetUniformLocation(program, "pLights[4].base.diffuseStrength"), pLights[4].diffuseStrength);
		glUniform3f(glGetUniformLocation(program, "pLights[4].position"), pLights[4].position.x, pLights[4].position.y, pLights[4].position.z);
		glUniform1f(glGetUniformLocation(program, "pLights[4].constant"), pLights[4].constant);
		glUniform1f(glGetUniformLocation(program, "pLights[4].linear"), pLights[4].linear);
		glUniform1f(glGetUniformLocation(program, "pLights[4].exponent"), pLights[4].exponent);

		glUniform3f(glGetUniformLocation(program, "pLights[5].base.diffuseColour"), pLights[5].diffuseColour.x, pLights[5].diffuseColour.y, pLights[5].diffuseColour.z);
		glUniform1f(glGetUniformLocation(program, "pLights[5].base.diffuseStrength"), pLights[5].diffuseStrength);
		glUniform3f(glGetUniformLocation(program, "pLights[5].position"), pLights[5].position.x, pLights[5].position.y, pLights[5].position.z);
		glUniform1f(glGetUniformLocation(program, "pLights[5].constant"), pLights[5].constant);
		glUniform1f(glGetUniformLocation(program, "pLights[5].linear"), pLights[5].linear);
		glUniform1f(glGetUniformLocation(program, "pLights[5].exponent"), pLights[5].exponent);
	}



	// Setting spot light.
	glUniform3f(glGetUniformLocation(program, "sLight.base.diffuseColour"), sLight.diffuseColour.x, sLight.diffuseColour.y, sLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "sLight.base.diffuseStrength"), sLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	glUniform3f(glGetUniformLocation(program, "sLight.direction"), sLight.direction.x, sLight.direction.y, sLight.direction.z);
	glUniform1f(glGetUniformLocation(program, "sLight.edge"), sLight.edgeRad);

	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

		ibo = 0;
		glGenBuffers(1, &ibo);
	
		points_vbo = 0;
		glGenBuffers(1, &points_vbo);

		colors_vbo = 0;
		glGenBuffers(1, &colors_vbo);

		uv_vbo = 0;
		glGenBuffers(1, &uv_vbo);

		normals_vbo = 0;
		glGenBuffers(1, &normals_vbo);

	glBindVertexArray(0); // Can optionally unbind the vertex array to avoid modification.

	// Change shape data.
	g_prism.SetMat(0.1, 16);
	g_grid.SetMat(0.0, 16);

	// Enable depth test and blend.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	// Enable face culling.
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	glClearColor(13.0f / 255.0f, 26.0f / 255.0f, 38.0f / 255.0f, 255.0f / 255.0f);
	
	timer(0); 
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
	glUniform3f(glGetUniformLocation(program, "eyePosition"), position.x, position.y, position.z);
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);
	
	calculateView();
	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}

void DrawCastleWalls()
{
	//Woods
	glBindTexture(GL_TEXTURE_2D, txt[9]);
	for (int i = 0; i < 8; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(5.5f, 0.1f, 0.5f), Z_AXIS, -20.0f, glm::vec3(48.0f, 2.0f, -24.0f- 0.5 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 0; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 4 * i, 6.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 0; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.5f + 4 * i, 7.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 2 * i, 4.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 2 * i, 2.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[3]);
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 2 * i, 0.0f, -2.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 2 * i, 4.0f, -46.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 4 * i, 6.0f, -46.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.5f + 4 * i, 7.0f, -46.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 2 * i, 2.0f, -46.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[3]);
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(4.0f + 2 * i, 0.0f, -46.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	/*glBindTexture(GL_TEXTURE_2D, txt[3]);
	for (int i = 0; i < 45; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 0.15f), Y_AXIS, 90.0f, glm::vec3(3.85f, 0.0f, -1.0f-i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}*/
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 1; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 1.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(4.0f , 6.0f, -2.0f-4 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 1; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(4.0f , 7.0f, -2.5f - 4 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(4.0f, 4.0f, -2.0f - 2 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(4.0f, 2.0f, -2.0f - 2 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[3]);
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(4.0f, 0.0f, -2.0f - 2 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 4.0f, -2.0f - 2 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 2.0f, -2.0f - 2 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[3]);
	for (int i = 0; i < 22; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 0.0f, -2.0f - 2 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 1; i < 5; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 1.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 6.0f, -2.0f - 4 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 7; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(2.0f, 1.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 6.0f, -2.0f - 4 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	for (int i = 1; i < 5; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 7.0f, -2.5f - 4 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 7; i < 10; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(47.0f, 7.0f, -2.5f - 4 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
}
//---------------------------------------------------------------------
//
// display
//
void DrawMaze()
{
	{
		glBindTexture(GL_TEXTURE_2D, txt[2]);
		//1-1 wall
		for (int i = 0; i < 10; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(7.0f + 1.5 * i, 0.0f, -5.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 10; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(7.0f + 1.5 * i, 1.5f, -5.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 12; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(23.5f + 1.5 * i, 0.0f, -5.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 12; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(23.5f + 1.5 * i, 1.5f, -5.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//1-2 wall
		for (int i = 0; i < 23; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(7.0f + 1.5 * i, 0.0f, -42.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 23; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(7.0f + 1.5 * i, 1.5f, -42.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//1-3 wall
		for (int i = 0; i < 25; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(7.0f, 0.0f, -5.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 25; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(7.0f, 1.5f, -5.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//1-4 wall
		for (int i = 0; i < 25; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(41.0f, 0.0f, -5.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 25; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(41.0f, 1.5f, -5.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	{	//2-1 wall
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(9.5f + 1.5 * i, 0.0f, -7.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(9.5f + 1.5 * i, 1.5f, -7.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//2-2
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(9.5f + 1.5 * i, 0.0f, -40.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(9.5f + 1.5 * i, 1.5f, -40.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//2-3
		for (int i = 0; i < 10; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(9.5f, 0.0f, -7.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 10; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(9.5f, 1.5f, -7.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 11; i < 22; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(9.5f, 0.0f, -7.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 11; i < 22; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(9.5f, 1.5f, -7.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//2-4
		for (int i = 0; i < 22; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(38.0f, 0.0f, -7.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 22; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(38.0f, 1.5f, -7.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	{
		//3-1
		for (int i = 0; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(12.0f + 1.5 * i, 0.0f, -9.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(12.0f + 1.5 * i, 1.5f, -9.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//3-2
		for (int i = 0; i < 9; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(12.0f + 1.5 * i, 0.0f, -38.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 9; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(12.0f + 1.5 * i, 1.5f, -38.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 10; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(12.0f + 1.5 * i, 0.0f, -38.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 10; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(12.0f + 1.5 * i, 1.5f, -38.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//3-3
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(12.0f, 0.0f, -9.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(12.0f, 1.5f, -9.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//3-4
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(35.5f, 0.0f, -9.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 19; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(35.5f, 1.5f, -9.5f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	{
		//4-1
		for (int i = 0; i < 12; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(15.0f + 1.5 * i, 0.0f, -12.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 12; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(15.0f + 1.5 * i, 1.5f, -12.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//4-2
		for (int i = 0; i < 12; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(15.0f + 1.5 * i, 0.0f, -36.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 12; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(15.0f + 1.5 * i, 1.5f, -36.5f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//4-3
		for (int i = 0; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(15.0f, 0.0f, -12.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(15.0f, 1.5f, -12.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		//4-4
		for (int i = 0; i < 9; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(32.5f, 0.0f, -12.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 0; i < 9; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(32.5f, 1.5f, -12.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 10; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(32.5f, 0.0f, -12.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
		for (int i = 10; i < 16; i++)
		{
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(32.5f, 1.5f, -12.0f - 1.5 * i));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}


}

void DrawRoom()
{
	//1
	glBindTexture(GL_TEXTURE_2D, txt[7]);
	for (int i = 0; i < 2; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 0.9f, -14.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 3; i < 8; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 0.9f, -14.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[8]);
	for (int i = 0; i < 2; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 2.4f, -14.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 3; i < 8; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 2.4f, -14.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	//2
	glBindTexture(GL_TEXTURE_2D, txt[7]);
	for (int i = 0; i < 5; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 0.9f, -34.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 6; i < 8; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 0.9f, -34.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[8]);
	for (int i = 0; i < 5; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 2.4f, -34.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 6; i < 8; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), X_AXIS, 0.0f, glm::vec3(18.0f + 1.5 * i, 2.4f, -34.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	//3
	glBindTexture(GL_TEXTURE_2D, txt[7]);
	for (int i = 0; i < 13; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(18.0f, 0.9f, -14.5f - 1.5 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[8]);
	for (int i = 0; i < 13; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(18.0f, 2.4f, -14.5f - 1.5 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	//4
	glBindTexture(GL_TEXTURE_2D, txt[7]);
	for (int i = 0; i < 13; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(29.5f, 0.9f, -14.5f - 1.5 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, txt[8]);
	for (int i = 0; i < 13; i++)
	{
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.5f, 1.5f, 0.5f), Y_AXIS, 90.0f, glm::vec3(29.5f, 2.4f, -14.5f - 1.5 * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}


}

void DrawSideWalk()
{
	glBindTexture(GL_TEXTURE_2D, txt[6]);
	for (int i = 0; i < 19; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), X_AXIS, 0.0f, glm::vec3(1.0f + 3 * i, -0.5f, 6.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 19; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), X_AXIS, 0.0f, glm::vec3(1.0f + 3 * i, -0.5f, 4.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 19; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), X_AXIS, 0.0f, glm::vec3(1.0f + 3 * i, -0.5f, 1.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 18; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), Y_AXIS, 90.0f, glm::vec3(53.0f , -0.5f, 1.5f - 3.0f * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 18; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 3.0f), Y_AXIS, 90.0f, glm::vec3(55.5f, -0.5f, 1.5f - 3.0f * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 21; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), Y_AXIS, 90.0f, glm::vec3(-1.5f, -0.5f, 6.5f - 3.0f * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 21; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 3.0f), Y_AXIS, 90.0f, glm::vec3(-4.5f, -0.5f, 6.5f - 3.0f * i));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 18; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), X_AXIS, 0.0f, glm::vec3(1.0f + 3 * i, -0.5f, -52.5f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < 19; i++) {
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(3.0f, 1.0f, 2.5f), X_AXIS, 0.0f, glm::vec3(1.0f + 3 * i, -0.5f, -55.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}

}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	// Draw all shapes.

	/*glBindTexture(GL_TEXTURE_2D, alexTx);
	g_plane.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 10.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_plane.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	glBindTexture(GL_TEXTURE_2D, txt[1]);
	g_grid.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(1.0f, 0.0f, 4.0f));
	glDrawElements(GL_TRIANGLES, g_grid.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, txt[5]);
	g_ground.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(4.0f, 0.9f, -2.0f));
	glDrawElements(GL_TRIANGLES, g_grid.NumIndices(), GL_UNSIGNED_SHORT, 0);
	
	
	glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);
	cout << sLight.position.x << " " << sLight.position.y << " " << sLight.position.z << endl;
	DrawCastleWalls();
	DrawMaze();
	DrawRoom();
	DrawSideWalk();
	glBindTexture(GL_TEXTURE_2D, txt[0]);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.75f, 10.0f, 3.75f), X_AXIS, 0.0f, glm::vec3(3.0f, 0.0f, -4.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.75f, 10.0f, 3.75f), X_AXIS, 0.0f, glm::vec3(3.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.75f, 10.0f, 3.75f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -4.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.75f, 10.0f, 3.75f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0); // Done writing.
	glutSwapBuffers(); // Now for a potentially smoother render.
}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	else if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	else if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position.y += MOVESPEED;
	else if (keys & KEY_DOWN)
		position.y -= MOVESPEED;
}

void timer(int) { // essentially our update()
	parseKeys();
	glutPostRedisplay();
	glutTimerFunc(1000/FPS, timer, 0); // 60 FPS or 16.67ms.
}

//---------------------------------------------------------------------
//
// keyDown
//
void keyDown(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD; break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT; break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT; break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP; break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN; break;
	case 'i':
		sLight.position.z -= 0.1; break;
	case 'j':
		sLight.position.x -= 0.1; break;
	case 'k':
		sLight.position.z += 0.1; break;
	case 'l':
		sLight.position.x += 0.1; break;
	case 'p':
		sLight.position.y += 0.1; break;
	case ';':
		sLight.position.y -= 0.1; break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; break;
	case 's':
		keys &= ~KEY_BACKWARD; break;
	case 'a':
		keys &= ~KEY_LEFT; break;
	case 'd':
		keys &= ~KEY_RIGHT; break;
	case 'r':
		keys &= ~KEY_UP; break;
	case 'f':
		keys &= ~KEY_DOWN; break;
	case ' ':
		resetView();
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		keys &= ~KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		keys &= ~KEY_BACKWARD;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch += (GLfloat)((y - lastY) * TURNSPEED);
		yaw -= (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	/*glDeleteTextures(1, &alexTx);
	glDeleteTextures(1, &blankTx);*/
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("GAME2012 - Week 7");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp); // New function for third example.
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.
	
	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	glutMainLoop();

	return 0;
}
