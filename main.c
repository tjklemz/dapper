#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <cstring>

#define GLSL(src) "#version 150 core\n" #src

// Shader sources
const GLchar* vertexSource = GLSL(
	in vec2 position;
	in vec3 color;
	in vec2 texcoord;
	out vec3 Color;
	out vec2 Texcoord;

	void main() {
		Color = color;
		Texcoord = texcoord;
		gl_Position = vec4(position, 0.0, 1.0);
	}
);

const GLchar* fragmentSource = GLSL(
	in vec3 Color;
	in vec2 Texcoord;
	out vec4 outColor;
	uniform sampler2D tex;

	void main() {
		outColor = texture(tex, Texcoord);
	}
);

GLuint tex;

#define WIDTH 100
#define HEIGHT 100
#define CANVAS_DIMS (WIDTH*HEIGHT*3)

static GLfloat * canvas = NULL;
static GLFWwindow * window;

static void init()
{
	canvas = new GLfloat[WIDTH*HEIGHT*3];

	std::cout << "Canvas: " << canvas << std::endl;

	for(int i = 0; i < WIDTH*HEIGHT*3; ++i) {
		canvas[i] = 0.5f;
	}

	std::cout << "first val: " << canvas[0] << std::endl;

	for(int i = 0; i < 50*3; i+=3) {
		canvas[i+1] = 1.0f;
	}
}

static void updateCanvas()
{
	glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGB, GL_FLOAT, canvas);
}

static void draw(float xpos, float ypos)
{
	float color[] = { 1.0f, 1.0f, 1.0f };

	int x = fmin(xpos, WIDTH-1);
	x = x > 0 ? x : 0;
	int y = fmin(ypos, HEIGHT-1);
	y = y > 0 ? y : 0;

	int i = 3*(WIDTH*y + x);

	canvas[i+0] = color[0];
	canvas[i+1] = color[1];
	canvas[i+2] = color[2]; 

	updateCanvas();
}

static void destroy() {
	delete [] canvas;
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static bool isDrawing = false;

static void onMouseButton(GLFWwindow * window, int button, int action, int mods)
{
	isDrawing = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
	if(isDrawing)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		draw(xpos, ypos);
	}
}

static void onMouseMove(GLFWwindow * window, double xpos, double ypos)
{
	if(isDrawing)
		draw(xpos, ypos);
}

int main(void)
{
	glfwSetErrorCallback(error_callback);

	init();

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(800, 600, "Simple example", NULL, NULL);
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	glewInit();

	// Create Vertex Array Object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLfloat vertices[] = {
        //  Position   Color             Texcoords
        -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
        -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create an element array
    GLuint ebo;
    glGenBuffers(1, &ebo);

    GLuint elements[] = {
        0, 1, 2,
        2, 3, 0
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), 0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));

    // Load texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, canvas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

	//glBindTexture(GL_TEXTURE_2D, 0);

	glfwSetKeyCallback(window, onKey);
	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetCursorPosCallback(window, onMouseMove);

	while(!glfwWindowShouldClose(window))
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//glActiveTexture(GL_TEXTURE0);
  		//glBindTexture(GL_TEXTURE_2D, tex);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteTextures(1, &tex);

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);

    glDeleteVertexArrays(1, &vao);

	glfwDestroyWindow(window);
	glfwTerminate();

	destroy();

	exit(EXIT_SUCCESS);
}
