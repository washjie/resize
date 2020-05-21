#pragma once

#include <vector>
#include <iostream>


struct vertex
{
	vertex(float x, float y, float tex_x, float tex_y)
	{
		this->x = x;
		this->y = y;
		this->tex_x = tex_x;
		this->tex_y = tex_y;
	}
	float x, y, tex_x, tex_y;
};

/*
/ My quad infomation is as follow:
/
/	x				y
/	-----------------
/	|				|
/	|	  quad		|
/	|				|
/	|				|
/	-----------------
/	w				z
/
*/

struct face
{
	face(int w, int z, int y, int x)
	{
		this->w = w;
		this->z = z;
		this->x = x;
		this->y = y;
	}
	int w, x, y, z;
};

class MyMesh
{
public:

	void add_vertex(float x, float y, float tex_x, float tex_y)
	{
		vertex_handle.push_back(vertex(x, y, tex_x, tex_y));
	}

	void add_face(face f)
	{
		face_handle.push_back(f);
	}
	std::vector< vertex > vertex_handle;
	std::vector< face > face_handle;
private:
	
	
};
