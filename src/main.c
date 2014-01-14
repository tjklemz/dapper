#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define GLSL(src) "#version 150 core\n" #src

// Shader sources
const GLchar* vertexSource = GLSL(
	uniform mat4 projection;
	uniform mat4 transform;
	in vec2 position;
	in vec3 color;
	in vec2 texcoord;
	out vec3 Color;
	out vec2 Texcoord;

	void main() {
		Color = color;
		Texcoord = texcoord;
		gl_Position = projection*transform*vec4(position, 0.0, 1.0);
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

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define WIDTH 6000
#define HEIGHT 4000
#define R_COMP 0
#define G_COMP 1
#define B_COMP 2
#define A_COMP 3
#define COLOR_COMPS 3
#define CANVAS_DIMS (WIDTH*HEIGHT*COLOR_COMPS)

struct Point {
	float x, y;
};
typedef struct Point Point;

struct Size {
	float width, height;
};
typedef struct Size Size;

struct Rect {
	Point origin;
	Size size;
};
typedef struct Rect Rect;

struct Color {
	float r,g,b,a;
};
typedef struct Color Color;

static Rect canvasRect = {0, 0, WIDTH, HEIGHT};

static GLfloat * canvas = NULL;
static GLFWwindow * window;
static GLuint projectionLoc, transformLoc;
static float scaleAmt = 1.0f;
static GLfloat matrix[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

static bool hasDrawingToolSelected = false;
static bool isDrawing = false;
static bool hasMoveToolSelected = false;
static bool isMoving = false;


static Rect makeRect(Point p1, Point p2)
{
	float x1 = fmin(p1.x, p2.x);
	float y1 = fmin(p1.y, p2.y);
	float x2 = fmax(p1.x, p2.x);
	float y2 = fmax(p1.y, p2.y);
	float w = fabs(x2-x1);
	w = fmax(w, 1);
	float h = fabs(y2-y1);
	h = fmax(h, 1);
	return (Rect){x1, y1, w, h};
}

static inline float nextPow2(float val)
{
	return fmax(2, powf(2, ceilf(log2f(val))));
}

static Rect makeRegion(Point p1, Point p2)
{
	Rect r = makeRect(p1, p2);
	float val = fmax(r.size.width, r.size.height);
	r.size.height = r.size.width = nextPow2(val);
	return r;
}

static Rect makeRegionBounded(Point p1, Point p2)
{
	Rect r = makeRegion(p1, p2);

	float extra_w = (r.size.width + r.origin.x) - canvasRect.size.width;
	extra_w = fmax(extra_w, 0);
	float extra_h = (r.size.height + r.origin.y) - canvasRect.size.height;
	extra_h = fmax(extra_h, 0);

	r.origin.x -= extra_w;
	r.origin.y -= extra_h;

	return r;
}

static inline int canvasIndex(float x, float y)
{
    int i = (int)(COLOR_COMPS)*(int)(WIDTH*y + x);
    //printf("(canvas index) x: %f y: %f i: %d\n", x, y, i);
	return i;
}

static void scale(GLfloat * matrix, float scale)
{
	matrix[0] = scale;
	matrix[5] = scale;
	matrix[10] = scale;
}

static void move(GLfloat * matrix, float x, float y)
{
	matrix[12] = x;
	matrix[13] = y;
}

static void identity(GLfloat * matrix)
{
	GLfloat I[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	memcpy(matrix, I, sizeof(GLfloat)*16);
}

static void init()
{
	canvas = malloc(sizeof(GLfloat)*CANVAS_DIMS);

	for(int i = 0; i < CANVAS_DIMS; ++i) {
		canvas[i] = 0.5f;
	}

	//setup scale amount
	float ratioWidth = WINDOW_WIDTH > WIDTH ? (float)WIDTH/WINDOW_WIDTH : (float)WINDOW_WIDTH/WIDTH;
	float ratioHeight = WINDOW_HEIGHT > HEIGHT ? (float)HEIGHT/WINDOW_HEIGHT : (float)WINDOW_HEIGHT/HEIGHT;
	float ratio = ratioHeight < ratioWidth ? ratioHeight : ratioWidth;
	scaleAmt = 0.8*ratio;
}

static void updateCanvas(Rect r)
{
	int i = canvasIndex(r.origin.x, r.origin.y);
	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, canvasRect.size.width);
	glTexSubImage2D(GL_TEXTURE_2D, 0, r.origin.x, r.origin.y, r.size.width, r.size.height, GL_RGB, GL_FLOAT, &canvas[i]);
}

static Point screenToCanvas(Point screenPoint)
{
	double s = 1.0/scaleAmt;
	float canvasX = (int)(s*(screenPoint.x - canvasRect.origin.x));
	float canvasY = (int)(s*(screenPoint.y - canvasRect.origin.y));
	return (Point){canvasX, canvasY};
}

static Point screenToCanvasBounded(Point screenPoint)
{
	Point p = screenToCanvas(screenPoint);

	float canvasX = p.x < WIDTH ? p.x : WIDTH-1;
	canvasX = canvasX > 0 ? canvasX : 0;
	float canvasY = p.y < HEIGHT ? p.y : HEIGHT-1;
	canvasY = canvasY > 0 ? canvasY : 0;

	return (Point){canvasX, canvasY};
}

static bool isInCanvas(float xpos, float ypos)
{
	Point p = screenToCanvas((Point){xpos, ypos});
	return p.x > 0 && p.x < WIDTH && p.y > 0 && p.y < HEIGHT;
}

static void placePoint(Point p, Color c)
{
	int i = canvasIndex(p.x, p.y);
	canvas[i+R_COMP] = c.r;
	canvas[i+G_COMP] = c.g;
	canvas[i+B_COMP] = c.b;
	//canvas[i+A_COMP] = c.a;
	//TODO: figure out how to handle Alpha
}

static void placeRect(Rect r, Color c)
{
	for(int x = r.origin.x; x < r.origin.x + r.size.width; ++x) {
		for(int y = r.origin.y; y < r.origin.y + r.size.height; ++y) {
			placePoint((Point){x, y}, c);
		}
	}
}

static float firstDrawX = -1.0f;
static float firstDrawY = -1.0f;

static void draw(float xpos, float ypos)
{
	Color color = { 1.0f, 1.0f, 1.0f, 1.0f};

	Point oldPoint = screenToCanvasBounded((Point){firstDrawX, firstDrawY});
	Point newPoint = screenToCanvasBounded((Point){xpos, ypos});

	Rect dirtyRegion = makeRegionBounded(oldPoint, newPoint);
	//placeRect(dirtyRegion, color);
	placePoint(newPoint, color);
	updateCanvas(dirtyRegion);
	//printf("dirtyRegion: %f %f %f %f\n", dirtyRegion.origin.x, dirtyRegion.origin.y, dirtyRegion.size.width, dirtyRegion.size.height);
	//updateCanvas((Rect){newPoint.x, newPoint.y, 10, 10});
	//updateCanvas((Rect){0, 0, WIDTH, HEIGHT});

	firstDrawX = xpos;
	firstDrawY = ypos;
}

static void beginDraw(float xpos, float ypos)
{
	firstDrawX = xpos;
	firstDrawY = ypos;

	if((isDrawing = isInCanvas(xpos, ypos)))
		draw(xpos, ypos);
}

static void endDraw(float xpos, float ypos)
{
	isDrawing = false;
}

static float firstX = -1.0f;
static float firstY = -1.0f;

static void moveCanvas(float xpos, float ypos)
{
	canvasRect.origin.x += xpos - firstX;
	canvasRect.origin.y += ypos - firstY;
	firstX = xpos;
	firstY = ypos;
	move(matrix, canvasRect.origin.x, canvasRect.origin.y);
	glUniformMatrix4fv(transformLoc, 1, false, matrix);
}

static void beginMoveCanvas(float xpos, float ypos)
{
	isMoving = hasMoveToolSelected;
	firstX = xpos;
	firstY = ypos;
}

static void endMoveCanvas(float xpos, float ypos)
{
	isMoving = false;
}

static void destroy() {
	free(canvas);
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if(key == GLFW_KEY_SPACE)
		hasMoveToolSelected = (action != GLFW_RELEASE);
}

static void onMouseButton(GLFWwindow * window, int button, int action, int mods)
{
	if(button == GLFW_MOUSE_BUTTON_LEFT)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		hasDrawingToolSelected = !hasMoveToolSelected;

		if(action == GLFW_PRESS) {
			if(hasMoveToolSelected) {
				beginMoveCanvas(xpos, ypos);
			} else if(hasDrawingToolSelected) {
				beginDraw(xpos, ypos);
			}
		} else if(action == GLFW_RELEASE) {
			if(isMoving) {
				endMoveCanvas(xpos, ypos);
			} else if(isDrawing) {
				endDraw(xpos, ypos);
			}
		}
	} 
	else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		scaleAmt *= (mods & GLFW_MOD_SHIFT ? 0.8 : 1.2);
		scale(matrix, scaleAmt);
		glUniformMatrix4fv(transformLoc, 1, false, matrix);
	}
}

static void onMouseMove(GLFWwindow * window, double xpos, double ypos)
{
	if(isDrawing)
		draw(xpos, ypos);
	else if(isMoving)
		moveCanvas(xpos, ypos);
}

int main(void)
{
	glfwSetErrorCallback(error_callback);

	init();

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Drawing App", NULL, NULL);
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
        0,  0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
        WIDTH, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
        WIDTH, HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
        0, HEIGHT, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, canvas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

	projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    {
        GLfloat left = 0.0f;
        GLfloat right = WINDOW_WIDTH;
        GLfloat bottom = WINDOW_HEIGHT;
        GLfloat top = 0.0f;
        GLfloat zNear = -1.0f;
        GLfloat zFar = 1.0f;
        GLfloat ortho[16] = {2.0f / (right-left), 0, 0, 0,
                            0, 2.0f / (top-bottom), 0, 0,
                            0, 0, -2.0f / (zFar - zNear), 0,
                            -(right+left)/(right-left), -(top+bottom)/(top-bottom), -(zFar+zNear)/(zFar-zNear), 1};
        glUniformMatrix4fv(projectionLoc, 1, false, ortho);
    }

    transformLoc = glGetUniformLocation(shaderProgram, "transform");
    scale(matrix, scaleAmt);
    glUniformMatrix4fv(transformLoc, 1, false, matrix);

	glBindTexture(GL_TEXTURE_2D, 0);

	glfwSetKeyCallback(window, onKey);
	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetCursorPosCallback(window, onMouseMove);

	while(!glfwWindowShouldClose(window))
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
  		glBindTexture(GL_TEXTURE_2D, tex);

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
