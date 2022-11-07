#pragma once
#ifndef SHAPE_H
#define SHAPE_H

#include <memory>
#include <vector>
#include <math.h>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;

class Shape
{
public:
	Shape();
	virtual ~Shape();
	void loadObj(const std::string &filename, std::vector<float> &pos, std::vector<float> &nor, std::vector<float> &tex, bool loadNor = true, bool loadTex = true);
	void loadMesh(const std::string &meshName);
	void loadBlend(const std::vector<std::string>& blendName);
	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	virtual void init();
	virtual void draw(float t);
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	
protected:
	std::string meshFilename;
	std::string textureFilename;
	std::vector<std::string> blendFilename;
	std::shared_ptr<Program> prog;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<float> basePosBuf;
	std::vector<float> baseNorBuf;
	std::vector< std::vector<float> > blendPosBuf;
	std::vector< std::vector<float> > blendNorBuf;
	std::vector< std::vector<float> > posDelta;
	std::vector< std::vector<float> > norDelta;
	int numBlend;
	GLuint posBufID;
	GLuint norBufID;
	GLuint texBufID;
};

#endif
