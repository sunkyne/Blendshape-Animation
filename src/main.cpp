#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLSL.h"
#include "Program.h"
#include "Camera.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "Texture.h"

using namespace std;

// Stores information in data/input.txt
class DataInput
{
public:
	vector<string> textureData;
	vector< vector<string> > meshData;
	vector< vector<string> > blendData;
};

DataInput dataInput;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the shaders are loaded from
string DATA_DIR = ""; // where the data are loaded from
bool keyToggles[256] = {false};

shared_ptr<Camera> camera = NULL;
vector< shared_ptr<Shape> > shapes;
map< string, shared_ptr<Texture> > textureMap;
shared_ptr<Program> prog = NULL;
double t, t0;

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
	switch(key) {
	}
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved(xmouse, ymouse);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = mods & GLFW_MOD_SHIFT;
		bool ctrl  = mods & GLFW_MOD_CONTROL;
		bool alt   = mods & GLFW_MOD_ALT;
		camera->mouseClicked(xmouse, ymouse, shift, ctrl, alt);
	}
}

void init()
{
	keyToggles[(unsigned)'c'] = true;
	
	camera = make_shared<Camera>();
	
	// Create shapes
	for(const auto &mesh : dataInput.meshData) {
		auto shape = make_shared<Shape>();
		shapes.push_back(shape);
		shape->loadMesh(DATA_DIR + mesh[0]);

		if (mesh[0].find("head") != std::string::npos) {
			std::vector<std::string> blends;
			for (const auto& blend : dataInput.blendData) {
				blends.push_back(DATA_DIR + blend[0]);
			}
			shape->loadBlend(blends);
		}

		shape->setTextureFilename(mesh[1]);
	}
	
	// GLSL programs
	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "phong_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
	prog->setVerbose(true);
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	for(auto shape : shapes) {
		shape->init();
	}
	
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addAttribute("aTex");
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addUniform("ka");
	prog->addUniform("ks");
	prog->addUniform("s");
	prog->addUniform("kdTex");
	
	// Bind the texture to unit 1.
	int unit = 1;
	prog->bind();
	glUniform1i(prog->getUniform("kdTex"), unit);
	prog->unbind();
	
	for(const auto &filename : dataInput.textureData) {
		auto textureKd = make_shared<Texture>();
		textureMap[filename] = textureKd;
		textureKd->setFilename(DATA_DIR + filename);
		textureKd->init();
		textureKd->setUnit(unit); // Bind to unit 1
		textureKd->setWrapModes(GL_REPEAT, GL_REPEAT);
	}
	
	// Initialize time.
	glfwSetTime(0.0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void render()
{
	// Update time.
	double t1 = glfwGetTime();
	float dt = (t1 - t0);
	if(keyToggles[(unsigned)' ']) {
		t += dt;
	}
	t0 = t1;

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Use the window size for camera.
	glfwGetWindowSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	
	// Apply global transform
	MV->pushMatrix();
	MV->translate(0.0, -18.5, 0.0);
	
	// Draw shapes
	prog->bind();
	for(const auto &shape : shapes) {
		MV->pushMatrix();
		textureMap[shape->getTextureFilename()]->bind(prog->getUniform("kdTex"));
		glLineWidth(1.0f); // for wireframe
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniform3f(prog->getUniform("ka"), 0.1f, 0.1f, 0.1f);
		glUniform3f(prog->getUniform("ks"), 0.1f, 0.1f, 0.1f);
		glUniform1f(prog->getUniform("s"), 200.0f);
		shape->setProgram(prog);
		shape->draw(t);
		MV->popMatrix();
	}
	prog->unbind();
	
	// Undo global transform
	MV->popMatrix();

	// Pop matrix stacks.
	MV->popMatrix();
	P->popMatrix();

	GLSL::checkError(GET_FILE_LINE);
}

void loadDataInputFile()
{
	string filename = DATA_DIR + "input.txt";
	ifstream in;
	in.open(filename);
	bool facs_check = false;
	if(!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;
	
	string line;
	while(1) {
		getline(in, line);
		if(in.eof()) {
			break;
		}
		if(line.empty()) {
			continue;
		}
		// Skip comments
		if(line.at(0) == '#') {
			continue;
		}
		// Parse lines
		string key, value;
		stringstream ss(line);
		// key
		ss >> key;
		if(key.compare("TEXTURE") == 0) {
			ss >> value;
			dataInput.textureData.push_back(value);
		} else if(key.compare("MESH") == 0) {
			vector<string> mesh;
			ss >> value;
			mesh.push_back(value); // obj
			ss >> value;
			mesh.push_back(value); // texture
			dataInput.meshData.push_back(mesh);
		}else if (key.compare("BLEND") == 0) {
			if (facs_check) {
				continue;
			}
			vector<string> blend;
			ss >> value;
			blend.push_back(value);
			ss >> value;
			blend.push_back(value);
			dataInput.blendData.push_back(blend);
		} else if (key.compare("FACS") == 0) {
			facs_check = true;
			vector<string> facs;
			ss >> value;
			facs.push_back(value);
			ss >> value;
			facs.push_back(value);
			dataInput.blendData.push_back(facs);
		} else {
			cout << "Unkown key word: " << key << endl;
		}
	}
	in.close();
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		cout << "Usage: A3 <SHADER DIR> <DATA DIR>" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	DATA_DIR = argv[2] + string("/");
	loadDataInputFile();
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "Kyne Sun", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		if(!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			// Render scene.
			render();
			// Swap front and back buffers.
			glfwSwapBuffers(window);
		}
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
