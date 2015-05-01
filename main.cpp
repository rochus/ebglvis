#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "shader.hpp"
#include "glutil.hpp"

#define rotate_model 1
#define animate_cam 0


template<typename T>
T deg2rad(T d) {
	return static_cast<T>(d * M_PI / 180.0);
}


struct Event {
	GLfloat x, y, t, p;
};


#define N_EVENTS  1000
#define dvs_size  128

std::vector<Event> events;

// vertex buffer object for the events we wish to render
GLuint vbo_events= 0;
GLuint vao = 0;

// shaders and program
shader *fs;
shader *vs;
program *p;

// model view matrices
//
glm::mat4 model = glm::mat4(1.0f);

glm::mat4 view = glm::lookAt(
		glm::vec3(1.2f, 1.1f, 1.4f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));


struct Camera {
	glm::vec3 pos;
	glm::vec3 target;
	glm::vec3 up;
};

glm::mat4 projection = glm::perspective(
		deg2rad(100.0f),
		4.0f / 3.0f,
		0.1f, 100.0f);

void
generate_buffers() {
	glGenBuffers(1, &vbo_events);
	// glBindBuffers is called in the render function (as we stream data to
	// the GPU)
	GL_CHECK_ERROR();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_events);
	// glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	GL_CHECK_ERROR();
}


static void
glfw_error_callback(int error, const char *description) {
	std::cerr << "E: " << description << " (" << error << ")" << std::endl;
}


static void
glfw_key_callback(GLFWwindow *win, int key, int /*scancode*/, int action, int /*mode*/) {
	if (action != GLFW_PRESS) return;

	switch (key) {
	case GLFW_KEY_ESCAPE:
	case GLFW_KEY_Q:
		glfwSetWindowShouldClose(win, GL_TRUE);
		break;

	default:
		break;
	}
}


int
rand_range(int min, int max) {
	return min + (rand() % (int)(max - min + 1));
}


/*
 * TODO: receive more data from the DVS
 */
void
update_data(float t, float dt) {
	// increase age -> decrease position
	for (auto &e: events) {
		e.t -= dt;
	}

	// storing event like this will have them sorted automatically

	// remove events that are out of reach (t=0 meanse position = -1.0)
	events.erase(std::remove_if(
				events.begin(),
				events.end(),
				[](Event e){ return e.t < -1.0f; }),
			events.end());

	for (size_t i = 0; i < N_EVENTS; i++) {
		Event e = {
			2.0f * (float)rand_range(0, dvs_size-1) / (float)dvs_size - 1.0f,
			2.0f * (float)rand_range(0, dvs_size-1) / (float)dvs_size - 1.0f,
			1.0,
			(float)rand_range(0, 1)
		};
		events.push_back(e);
	}
}


void
destroy_data() {
	events.clear();
}


GLFWwindow*
gl_setup() {
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) {
		std::cerr << "E: Could not init GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	// create a window
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	GLFWwindow *window = glfwCreateWindow(640, 480, "particles", NULL, NULL);
	if (!window) {
		std::cerr << "E: Could not create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	// make the window's context current
	glfwMakeContextCurrent(window);

	// capture keys
	glfwSetKeyCallback(window, glfw_key_callback);
	GL_CHECK_ERROR();

	// start GLEW stuff
	glewExperimental = GL_TRUE;
	glewInit();
	GL_CHECK_ERROR();

	// print openGL information
	gl_print_info();

	glEnable(GL_PROGRAM_POINT_SIZE);

	// tell OpenGL only to draw onto a pixel if the shape is closer to the viewer

	return window;
}


void
gl_destroy(GLFWwindow *window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}


void
render_points() {
	// load data to GPU
	glEnableVertexAttribArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_events);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Event) * events.size(), &events[0], GL_STREAM_DRAW);

	// disable depth sorting to get blending right
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	// draw
	glDrawArrays(GL_POINTS, 0, events.size());

	// re-enable depth sorting for everything else
	glDepthMask(GL_TRUE);
}

void
render_loop(GLFWwindow *window) {


	// get IDs to access the data
	GLuint modelId = p->getUniformLocation("model");
	GLuint viewId = p->getUniformLocation("view");
	GLuint projId = p->getUniformLocation("projection");

	float t = 0.0f;
	float dt = 0.02f;
	float x = 0.0f;
	while (!glfwWindowShouldClose(window)) {

		// fetch new data
		t += dt;
		update_data(t, dt);


		// simple demo: moving the object
		/*
		glm::mat4 translation(1.0f);
		translation[2][3] = 0.1;
		model = translation * model;
		*/
#if rotate_model
		model = glm::rotate(model, deg2rad(0.10f), glm::vec3(0, 1, 0));
#endif

#if animate_cam
		// simple demo: moving the camera
		glm::vec3 cpos(1.1 * sin(x), 1.1 * cos(x), 1.3f);
		view = glm::lookAt(
				cpos,
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));

		x += 0.015f;
		if (x > M_PI)
			x -= 2.0f*M_PI;
#endif

		glViewport(0, 0, 640, 480);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

		p->use();

		// set params to shaders
		glUniformMatrix4fv(modelId, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewId, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projId, 1, GL_FALSE, glm::value_ptr(projection));

		render_points();

		//textured_fullscreen_quad();

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
}


int
main(int, char *[]) {
	auto window = gl_setup();
	generate_buffers();

	vs = new shader(GL_VERTEX_SHADER);
	vs->load_from_file("shaders/vertex.glsl");

	fs = new shader(GL_FRAGMENT_SHADER);
	fs->load_from_file("shaders/fragment.glsl");

	p = new program();

	fs->compile();
	vs->compile();
	p->link(2, vs, fs);

	render_loop(window);
	gl_destroy(window);
	destroy_data();

	delete p;
	delete vs;
	delete fs;
	return EXIT_SUCCESS;
}
