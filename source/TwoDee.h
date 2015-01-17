#pragma once

#define __CL_ENABLE_EXCEPTIONS

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
#if defined (WIN32)
#include <GL/wglew.h>
#endif
#if defined (__linux__)
#include <GL/glxew.h>
#endif

// GLFW
#include <GLFW/glfw3.h>

// OpenCL
#if defined (WIN32) || defined (__linux__)
#include <CL/cl.hpp>
#include <CL/cl_gl.h>
#elif defined (__APPLE__)
#include <OpenGL/OpenGL.h>
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl.h>
#include <OpenCL/cl_gl_ext.h>
#endif

#include <vector>

//#define USEDOUBLE

#ifdef USEDOUBLE
#define GL_DATA_TYPE GLdouble
#define CL_DATA_TYPE cl_double
#define GL_VERTEX_DATA_TYPE GL_DOUBLE
#else
#define GL_DATA_TYPE GLfloat
#define CL_DATA_TYPE cl_float
#define GL_VERTEX_DATA_TYPE GL_FLOAT
#endif

class OneDee
{
public:
	OneDee(GLsizei numVerticesX, CL_DATA_TYPE R, CL_DATA_TYPE dx, CL_DATA_TYPE dt);

	~OneDee();

	enum kernelArguments
	{
		KERNELARG_U_AT_N1,
		KERNELARG_U_AT_N2,
		KERNELARG_DX,
		KERNELARG_DT
	};

	// Read a shader/kernel source from a file
	// store the source in a std::string
	void ReadSource(const char *fname, std::string &buffer);

	// Compile a shader
	GLuint LoadAndCompileShader(const char *fname, GLenum shaderType);

	// Create a program from two shaders
	GLuint CreateGLProgram(const char *path_vert_shader, const char *path_frag_shader);

	static void KeyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GL_TRUE);
	}
	static void ErrorCallback(int error, const char* description)
	{
		fputs(description, stderr);
	}
	void CreateGlWindow(void);

	void Loop(void);

	void InitializeGL();

	void InitializeCL();

	void InitializeSharedData();

	void ExecuteKernel();

	void UpdateGL();

	void UpdateCL();


	void UpdateDisplay();

	GLFWwindow* window;

	cl::Context context;
	cl::CommandQueue commandQueue;
	cl::Kernel kernel;
	cl::Event kernelEvent;

	//cl_platform_id platform;
	//cl_device_id device;
	////cl_context context;
	//cl_context_properties properties[7];
	//cl_program program;
	//cl_command_queue queue;
	//cl_kernel kernel;

	GLuint vao_A, vbo_A;
	GLuint vao_B, vbo_B;
	
	cl::Memory sharedVbo_A;
	cl::Memory sharedVbo_B;

	std::vector<cl::Memory> sharedVbo;

	bool computeFromAToB;

	GLsizei numVerticesX;
	CL_DATA_TYPE R, dx, dt;

	int windowWidth;
	int windowHeight;

	unsigned int iterationCount;
	bool drawFrame;
	cl::Event kernel_event;
	cl::NDRange globalSize;
};

