#include "TwoDee.h"

#include <stdio.h>
#include <fstream>
#include <iostream>

#define VERT_SHADER ("assets/vert.shader")
#define FRAG_SHADER ("assets/frag.shader")
#define KERNEL_FILE ("assets/marchForward_1D.cl")
#define KERNEL_FUNC ("marchForward_1D")

OneDee::OneDee(GLsizei numVerticesX, CL_DATA_TYPE R, CL_DATA_TYPE dx, CL_DATA_TYPE dt)
{
	this->computeFromAToB = true;

	this->numVerticesX = numVerticesX;
	this->dx = dx;
	this->dt = dt;
	this->R = R;

	this->windowHeight = 480;
	this->windowWidth = 640;

	this->iterationCount = 0;
	this->drawFrame = true;
}

OneDee::~OneDee()
{

}

void OneDee::CreateGlWindow()
{
	// Tell glfw how to output error messages
	glfwSetErrorCallback(this->ErrorCallback);

	// Init glfw
	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Make a glfw window
	window = glfwCreateWindow(this->windowWidth, this->windowHeight, "Simple example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	// Tell glfw what to use to process key presses
	glfwSetKeyCallback(window, this->KeyPressCallback);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW! I'm out!" << std::endl;
		glfwTerminate();
		exit(-1);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void OneDee::InitializeSharedData()
{
	// Compile shader program to color the points
	GLuint shaderProgram = this->CreateGLProgram(VERT_SHADER, FRAG_SHADER);

	// Get the location of the attributes that enters in the vertex shader
	GLint position_attribute = glGetAttribLocation(shaderProgram, "position");

	// Create array of points along line and initialize to a function
	GL_DATA_TYPE *vertices_position = new GL_DATA_TYPE[this->numVerticesX * 2];
	GLint yOffset = this->numVerticesX;
	for (size_t j = 0; j < this->numVerticesX; ++j)
	{
		// X
		vertices_position[2 * j + 0] = this->dx* (GL_DATA_TYPE)j;
		// Y
		vertices_position[2 * j + 1] = -this->R * sin(vertices_position[2 * j + 0] );
		//vertices_position[2 * j + 1] = (vertices_position[2 * j + 0] < 3.14 ? 1 : -1);
	}

	//============================= A

	// Create a Vertex Array Object that tells OpenGL how to draw the points
	GLint numVertexArrayObjects = 1;
	glGenVertexArrays(numVertexArrayObjects, &this->vao_A);
	glBindVertexArray(this->vao_A);

	// Create a Vector Buffer Object that will store the vertices on video memory
	GLint numVertexBufferObjects = 1;
	glGenBuffers(numVertexBufferObjects, &this->vbo_A);

	// Allocate space and upload the data from CPU to GPU
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_A);
	glBufferData(GL_ARRAY_BUFFER, this->numVerticesX * 2 * sizeof(GL_DATA_TYPE), vertices_position, GL_DYNAMIC_DRAW);

	// Specify how the data for position can be accessed
	GLsizei stride = 0;
	glVertexAttribPointer(position_attribute, 2, GL_VERTEX_DATA_TYPE, GL_FALSE, stride, 0);

	// Enable the attribute
	glEnableVertexAttribArray(position_attribute);

	// Enable point size
	glEnable(GL_PROGRAM_POINT_SIZE);

	// Create CL memory objects from GL VBO
	try
	{
		cl_int err;
		this->sharedVbo_A = cl::BufferGL(this->context, CL_MEM_READ_WRITE, this->vbo_A, &err);
	}
	catch (cl::Error e) {
		std::cout << e.what() << ": Error code "
			<< e.err() << std::endl;
	}

	//============================= B

	// Create a Vertex Array Object that tells OpenGL how to draw the points
	glGenVertexArrays(numVertexArrayObjects, &this->vao_B);
	glBindVertexArray(this->vao_B);

	// Create a Vector Buffer Object that will store the vertices on video memory
	glGenBuffers(numVertexBufferObjects, &this->vbo_B);

	// Allocate space and upload the data from CPU to GPU
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_B);
	glBufferData(GL_ARRAY_BUFFER, this->numVerticesX * 2 * sizeof(GL_DATA_TYPE), vertices_position, GL_DYNAMIC_DRAW);

	// Specify how the data for position can be accessed
	glVertexAttribPointer(position_attribute, 2, GL_VERTEX_DATA_TYPE, GL_FALSE, 0, 0);

	// Enable the attribute
	glEnableVertexAttribArray(position_attribute);

	// Enable point size
	glEnable(GL_PROGRAM_POINT_SIZE);

	// Create CL memory objects from GL VBO
	try
	{
		cl_int err;
		this->sharedVbo_B = cl::BufferGL(this->context, CL_MEM_READ_WRITE, this->vbo_B, &err);
	}
	catch (cl::Error e) {
		std::cout << e.what() << ": Error code "
			<< e.err() << std::endl;
	}

	// Done with local data, delete it
	delete vertices_position;

	this->globalSize = (size_t)(this->numVerticesX - 2); // We aren't doing computation on boundaries
	this->sharedVbo.push_back(this->sharedVbo_A);
	this->sharedVbo.push_back(this->sharedVbo_B);
};

/* Initialize OpenCl processing */
void OneDee::InitializeCL()
{
	// C++ version
	try {
		/* Identify a platform */
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		std::cerr << "Number of OpenCL implementations (platforms) is: " << platforms.size() << std::endl;

		std::string platformVendor;
		platforms[0].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor);
		std::cerr << "Selecting platforms[0] by: " << platformVendor << "\n";

		/* Create OpenCL context properties */
		cl_context_properties properties[] = {
			CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
			CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
			CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
			0 };

		/* Select a specific context */
		this->context = cl::Context(CL_DEVICE_TYPE_GPU, properties);

		/* Access all devices in that context */
		std::vector<cl::Device> devices;
		devices = this->context.getInfo<CL_CONTEXT_DEVICES>();
		std::string deviceName;
		for (size_t i = 0; i < devices.size(); ++i)
		{
			deviceName = devices[i].getInfo<CL_DEVICE_NAME>();
			std::cout << "Device: "
				<< deviceName.c_str()
				<< std::endl;
		}

		/* Create program from file */
		std::string programString;
		this->ReadSource(KERNEL_FILE, programString);
		cl::Program::Sources source(1,
			std::make_pair(programString.c_str(),
			programString.length() + 1));
		cl::Program program(context, source);

		/* Build the program for all devices in the program's context */
		cl_int err = program.build(devices);

		/* Create kernel */
		this->kernel = cl::Kernel(program, KERNEL_FUNC, &err);

		/* Set kernel arguments that are constant throughout computation */
		this->kernel.setArg(KERNELARG_DX, this->dx);
		this->kernel.setArg(KERNELARG_DT, this->dt);

		/* Create a command queue for the first device in the context */
		this->commandQueue = cl::CommandQueue(context, devices[0], 0, &err);

	}
	catch (cl::Error e) {
		std::cout << e.what() << ": Error code "
			<< e.err() << std::endl;
	}
}

void OneDee::UpdateGL()
{
	// Draw every N iterations
	if (this->drawFrame)
	{
		if (this->computeFromAToB)
		{
			glBindVertexArray(this->vao_A);
		}
		else
		{
			glBindVertexArray(this->vao_B);
		}

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_POINTS, 0, this->numVerticesX);
		// Swap front and back buffers
		glfwSwapBuffers(window);

		glfwPollEvents();
	}
}

void OneDee::UpdateCL()
{
	try
	{
		if (this->drawFrame)
		{
			/* Complete OpenGL processing */
			glFlush();

			// If we drew the last frame, we need to reacquire the gl objects
			this->commandQueue.enqueueAcquireGLObjects(&sharedVbo, NULL, NULL);
		}

		/* Swap the values of u we are using*/
		if (!this->computeFromAToB)
		{
			this->kernel.setArg(KERNELARG_U_AT_N1, this->sharedVbo_A);
			this->kernel.setArg(KERNELARG_U_AT_N2, this->sharedVbo_B);
		}
		else
		{
			this->kernel.setArg(KERNELARG_U_AT_N1, this->sharedVbo_B);
			this->kernel.setArg(KERNELARG_U_AT_N2, this->sharedVbo_A);
		}

		//  Switch which array we are using next time
		this->computeFromAToB = !this->computeFromAToB;

		/* Execute the kernel */
		cl::NDRange offset = (size_t)0;
		cl::NDRange localSize = (size_t)64;
		
		cl_int err = this->commandQueue.enqueueNDRangeKernel(
			this->kernel,
			offset,
			this->globalSize,
			localSize,
			NULL,
			&this->kernelEvent);

		++this->iterationCount;

		// Determine if we are going to draw the next frame
		this->drawFrame =  (this->iterationCount % 50000 == 0);

		if (this->drawFrame)
		{
			// Stall local application until kernel is complete
			this->kernelEvent.wait();

			// Need to release the gl objects to draw with them
			this->commandQueue.enqueueReleaseGLObjects(&sharedVbo);
		}

	}
	catch (cl::Error e) {
		std::cout << e.what() << ": Error code "
			<< e.err() << std::endl;
	}
}

void OneDee::Loop()
{
	this->CreateGlWindow();

	this->InitializeCL();

	// Initialize the data to be rendered
	this->InitializeSharedData();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		this->UpdateGL();

		this->UpdateCL();

		double time_s = this->iterationCount*this->dt;
		if (time_s >= 2.0 )
		//if (this->iterationCount > 20)
		{
			return;
		}
	}

	// Kill window
	glfwTerminate();
}

// Read a shader/kernel source from a file
// store the source in a std::vector<char>
void OneDee::ReadSource(const char *fname, std::string &buffer) {
	std::ifstream in(fname);
	if (in.is_open()) {

		std::string myBuffer(
			std::istreambuf_iterator<char>(in),
			(std::istreambuf_iterator<char>()));

		in.close();

		// Copy
		buffer = myBuffer;
	}
	else {
		std::cerr << "Unable to open " << fname << " I'm out!" << std::endl;
		exit(-1);
	}

}

// Compile a shader
GLuint OneDee::LoadAndCompileShader(const char *fname, GLenum shaderType) {
	// Load a shader from an external file
	std::string buffer;
	this->ReadSource(fname, buffer);
	const char *src = buffer.c_str();

	// Compile the shader
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	// Check the result of the compilation
	GLint test;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &test);
	if (!test) {
		std::cerr << "Shader compilation failed with this message:" << std::endl;
		std::vector<char> compilation_log(512);
		glGetShaderInfoLog(shader, (GLsizei)compilation_log.size(), NULL, &compilation_log[0]);
		std::cerr << &compilation_log[0] << std::endl;
		glfwTerminate();
		exit(-1);
	}
	return shader;
}

// Create a program from two shaders
GLuint OneDee::CreateGLProgram(const char *path_vert_shader, const char *path_frag_shader) {
	// Load and compile the vertex and fragment shaders
	GLuint vertexShader = this->LoadAndCompileShader(path_vert_shader, GL_VERTEX_SHADER);
	GLuint fragmentShader = this->LoadAndCompileShader(path_frag_shader, GL_FRAGMENT_SHADER);

	// Attach the above shader to a program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Flag the shaders for deletion
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Link and use the program
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	return shaderProgram;
}