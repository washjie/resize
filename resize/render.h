
#pragma once

#include <iostream>
#include <cmath>
#include <vector>

#include <opencv2/opencv.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"

void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

class MyGL
{
public:
	MyGL(int col, int row)
	{
		grid_col = col;
		grid_row = row;
	}
	void render();
	void initWindow();
	void loadOpenGL();
	
	void setVertexArrayData();
	void loadAndCreateTexture();
	
	//post processing
	void processInput(GLFWwindow* window);

	//pre processing
	cv::Mat readTexPic(char * filename);
	void setVAO(float x, float y, float z, float r, float g, float b, float tx, float ty);
	void setEBO();
private:
	int grid_col, grid_row;

	GLuint VBO, VAO, EBO, texture;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	std::vector<GLuint> grid_indices;
	cv::Mat texture_pic;
	GLFWwindow* window;
};

void MyGL::render()
{
	initWindow();
	
	loadOpenGL();
	
	//init shader
	Shader ourShader("vertex_shader", "fragment_shader");

	setVertexArrayData();
	loadAndCreateTexture();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glLineWidth(1.0f);

	//render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// bind Texture
		glBindTexture(GL_TEXTURE_2D, texture);

		// render container
		ourShader.use();
		glBindVertexArray(VAO);
		
		glDrawElements(GL_TRIANGLES, 6 * grid_col*grid_row , GL_UNSIGNED_INT, 0);
		//glDrawElements(GL_LINE, 6 * grid_col*grid_row, GL_UNSIGNED_INT, 0);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
}

void MyGL::initWindow()
{
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize glfw" << std::endl;
	}
	

	//glfwWindowHint(GLFW_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_ANY_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(texture_pic.cols, texture_pic.rows, "LearnOpenGl", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "window creation failure." << std::endl;
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
}

void MyGL::loadOpenGL()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return;
	}
}


void MyGL::setVertexArrayData()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices.front(), GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices.front(), GL_STATIC_DRAW);

	//position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(vertices[0]), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(vertices[0]), (void*)(3 * sizeof(vertices[0])));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(vertices[0]), (void*)(6 * sizeof(vertices[0])));
	glEnableVertexAttribArray(2);

}

void MyGL::loadAndCreateTexture()
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps

	if (texture_pic.data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_pic.cols, texture_pic.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_pic.data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cerr << "Failed to load texture" << std::endl;
	}
}

cv::Mat MyGL::readTexPic(char * filename)
{
	//read in texture picture
	texture_pic = cv::imread(filename, CV_LOAD_IMAGE_COLOR);//one for significance map, one for original map
	cv::Mat output = texture_pic.clone();
	if (!texture_pic.data)
	{
		std::cerr << "Error: Loading Image Failed." << std::endl;
		return output;
	}
	cv::flip(texture_pic, texture_pic, 0);
	cv::cvtColor(texture_pic, texture_pic, CV_BGR2RGB);

	return output;
}

void MyGL::setVAO(float x, float y, float z, float r, float g, float b, float tx, float ty)
{
	vertices.push_back(x);
	vertices.push_back(y);
	vertices.push_back(z);
	vertices.push_back(r);
	vertices.push_back(g);
	vertices.push_back(b);
	vertices.push_back(tx);
	vertices.push_back(ty);
}

void MyGL::setEBO()
{
	int vertex_pre_row = grid_col + 1;
	for (int i = 0; i < grid_row; ++i)
	{
		for (int j = 0; j < grid_col; ++j)
		{
			//first tri
			indices.push_back(i*vertex_pre_row + j);
			indices.push_back(i*vertex_pre_row + j + 1);
			indices.push_back((i + 1)*vertex_pre_row + j);

			//second tri
			indices.push_back(i*vertex_pre_row + j + 1);
			indices.push_back((i + 1)*vertex_pre_row + j);
			indices.push_back((i + 1)*vertex_pre_row + j + 1);
		}
	}
	
	//for (int i = 0; i < indices.size()/3; i++)
	//{
	//	std::cout << indices[i * 3] << ", " << indices[i * 3 + 1] << ", " << indices[i * 3 + 2] << std::endl;
	//}
	
}

static void error_callback(int error, const char* description)
{
	puts(description);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void MyGL::processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

	//implementing dragging vertex
	if (state == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		xpos /= (double)width; xpos -= 0.5; xpos *= 2;
		ypos /= (double)height; ypos = 0.5 - ypos; ypos *= 2;

		
		std::cout << xpos << ", " << ypos << std::endl;

		for (int i = 0; i < vertices.size(); i += 5)
		{
			float dist = std::sqrt(std::pow(vertices[i] - xpos, 2) + std::pow(vertices[i + 1] - ypos, 2));
			
			GLfloat pos[2];
			if (dist < 0.03)
			{
				//std::cout << "vertex " << i / 5 << " pos :" <<
				//	vertices[i] << ", " << vertices[i + 1] <<
				//	" distance from cursor :" <<
				//	(sqrt(std::pow(vertices[i] - xpos, 2) + std::pow(vertices[i + 1] - ypos, 2))) << std::endl;

				pos[0] = xpos;
				//vertices[i] = xpos;
				pos[1] = ypos;
				//vertices[i+1] = ypos;
				std::cout << "vertex " << i / 5 << std::endl;
				glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vertices[0]), sizeof(pos), pos);
				vertices[i] = pos[0];
				vertices[i + 1] = pos[1];
			}
			
		}
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
		
}

