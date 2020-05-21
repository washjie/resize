
//saliency.jpg  butterfly.png k500_20.ppm 
#pragma warning(disable: 4996)

#include <iostream>
#include <vector>
#include <map>

#include "MyMesh.h"
#include "Significant.h"
#include "render.h"
//--------------------- cplex

#define IL_STD

#include<ilcplex\ilocplex.h>
#include<ilconcert\iloexpression.h>

//grid mesh : 10 (height) * 16 (width)
int grid_row = 20, grid_col = 50;//number of quad per row or col (not the number of vertex)
int vertex_per_row = grid_col + 1;
int vertex_per_col = grid_row + 1;
float h_r = (float)100/700, w_r = (float)200/1024;//ratio for height and width

int main(int argc, char *argv[])
{
	
	cv::Mat sigmap = significance(argv[1], argv[2]);

	// create my mesh structure
	// generate vertices position and texture coordinate
	MyMesh mesh;
	
	for (int i = 0; i < grid_row + 1; ++i)
	{
		for (int j = 0; j < grid_col + 1; ++j)
		{
			float x = (float)j * (2 / (float)grid_col) -1;
			float y = (float)i * (2 / (float)grid_row) -1;
			
			float tex_x = (float)j * (1 / (float)grid_col);
			float tex_y = (float)i * (1 / (float)grid_row);
			
			mesh.add_vertex(x, y, tex_x, tex_y);
		}
	}
	
	std::map<float, std::vector< face > > Patch;
	
	for (int i = 0; i < grid_row; ++i)
	{
		for (int j = 0; j < grid_col; ++j)
		{

			//claculate the four vertices handle for each face
			int bot_left = i*vertex_per_row + j;
			int bot_right = i*vertex_per_row + j + 1;
			int top_right = (i + 1)*vertex_per_row + j + 1;
			int top_left = (i + 1)*vertex_per_row + j;

			//calculate the S[i]:
			//we use a map to calculate numbers of each valuse's appearance
			//then choose the highest
			
			int x0 = mesh.vertex_handle[top_left].tex_x * sigmap.cols;
			int y0 = (1 - mesh.vertex_handle[top_left].tex_y) *sigmap.rows;

			int x1 = mesh.vertex_handle[bot_right].tex_x * sigmap.cols;
			int y1 = (1 - mesh.vertex_handle[bot_right].tex_y) * sigmap.rows;

			std::map<float, int> s;
			for (int y = y0; y < y1; ++y)
				for (int x = x0; x < x1; ++x)
					s[sigmap.at<float>(y, x)]++;

			float si, si_max = 0;
			for (auto it = s.begin(); it != s.end(); ++it)
				if (si_max < it->second)
					si = it->first;

			face f(bot_left, bot_right, top_right, top_left);
			
			mesh.add_face(f);
			Patch[si].push_back(f);
		}
	}
	
	//cplex
	std::cout << "Optimizing..." << std::endl;
	try
	{
		IloEnv environment;

		IloModel model(environment);
		IloNumVarArray variables(environment);
		IloRangeArray constraints(environment);
		IloExpr expressions(environment);

		for (int i = 0; i < mesh.vertex_handle.size(); ++i)
		{
			variables.add(IloNumVar(environment, -IloInfinity, IloInfinity)); //x
			variables.add(IloNumVar(environment, -IloInfinity, IloInfinity)); //y		
		}
		
		
		int vertice_size = mesh.vertex_handle.size();
		for (auto it = Patch.begin(); it != Patch.end(); ++it)
		{
			// first we need to calculate the center edge
			// 4 vertices for each face(quad): w-z, z-y, y-x, x-w
			// center edge: first face(quad)'s : Vertex(w) - Vertex(z)
			// significance is the patch key e.g. Patch[0.1]...
			
			float si = it->first;
			
			int cidx1 = it->second[0].w;
			int cidx2 = it->second[0].z;

			float Cx = mesh.vertex_handle[cidx1].x - mesh.vertex_handle[cidx2].x;
			float Cy = mesh.vertex_handle[cidx1].y - mesh.vertex_handle[cidx2].y;
			// C[2][2] = { {Cx,  Cy},
			//			   {Cy, -Cx} };
			
			//calculate C inverse
			
			float det = 1 / (Cx * -Cx - Cy * Cy);
			
			float inC[2][2] = { { -Cx*det , -Cy*det },
								{ -Cy*det ,  Cx*det } };
			
			
			//Dtf
			for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)//each face
			{
				int vList[5] = { jt->w, jt->z ,jt->y ,jt->x ,jt->w };
				
				for (int i = 0; i < 4; ++i)
				{
					//calculate _S, _R

					float ex = mesh.vertex_handle[vList[i]].x - mesh.vertex_handle[vList[i + 1]].x;
					float ey = mesh.vertex_handle[vList[i]].y - mesh.vertex_handle[vList[i + 1]].y;

					float S = (inC[0][0] * ex + inC[0][1] * ey);
					float R = (inC[1][0] * ex + inC[1][1] * ey);

					//Dst
					expressions += 0.8 * si * IloPower(variables[vList[i]] - variables[vList[i + 1]] -
						(S * (variables[cidx1] - variables[cidx2]) + R*(variables[vertice_size + cidx1] - variables[vertice_size + cidx2])), 2);
					expressions += 0.8 * si * IloPower(variables[vertice_size + vList[i]] - variables[vertice_size + vList[i + 1]] -
						(-R * (variables[cidx1] - variables[cidx2]) + S*(variables[vertice_size + cidx1] - variables[vertice_size + cidx2])), 2);

					//Dlt
					expressions += 0.2 * (1 - si) * IloPower(variables[vList[i]] - variables[vList[i + 1]] -
						(h_r * S * (variables[cidx1] - variables[cidx2]) + h_r * R * (variables[vertice_size + cidx1] - variables[vertice_size + cidx2])), 2);
					expressions += 0.2 * (1 - si) * IloPower(variables[vertice_size + vList[i]] - variables[vertice_size + vList[i + 1]] -
						(w_r *-R * (variables[cidx1] - variables[cidx2]) + w_r * S * (variables[vertice_size + cidx1] - variables[vertice_size + cidx2])), 2);
				}

				//Dor
				expressions += IloPower(variables[vertice_size + jt->y] - variables[vertice_size + jt->x], 2);
				expressions += IloPower(variables[vertice_size + jt->w] - variables[vertice_size + jt->z], 2);
				expressions += IloPower(variables[jt->x] - variables[jt->w], 2);
				expressions += IloPower(variables[jt->z] - variables[jt->y], 2);
			}
		}
		
		
		model.add(IloMinimize(environment, expressions));
		
		//boundary constraint
		for (int i = 0; i < vertice_size; i++)
		{
			if (mesh.vertex_handle[i].x == -1)
				constraints.add(variables[i] == -w_r);	
			if (mesh.vertex_handle[i].x == 1)
				constraints.add(variables[i] == w_r);
			if (mesh.vertex_handle[i].y == 1)
				constraints.add(variables[vertice_size + i] == h_r);
			if (mesh.vertex_handle[i].y == -1)
				constraints.add(variables[vertice_size + i] == -h_r);
		}		
			
		model.add(constraints);

		IloCplex cplex(model);

		if (!cplex.solve())
		{
			environment.error() << "Failed to optimize LP" << std::endl;
			return -1;
		}
		std::cout << "result" << std::endl;
		
		IloNumArray values(environment);
		IloNumArray result(environment);

		cplex.getValues(result, variables);
		/*
		environment.out() << "Solution status = " << cplex.getStatus() << std::endl;
		environment.out() << "Solution value  = " << cplex.getObjValue() << std::endl;
		cplex.getValues(values, variables);
		environment.out() << "Values          = " << values << std::endl;
		cplex.getSlacks(values, constraints);
		environment.out() << "Slacks          = " << values << std::endl;
		cplex.getDuals(values, constraints);
		environment.out() << "Duals           = " << values << std::endl;
		cplex.getReducedCosts(values, variables);
		environment.out() << "Reduced Costs   = " << values << std::endl;
		*/
		//cplex.exportModel("qpex1.lp");
		

		std::cout << "rendering ..." << std::endl;
		//---------------------------------------------
		//OpenGL rendering
		
		MyGL my_render(grid_col, grid_row);

		cv::Mat comp = my_render.readTexPic(argv[3]);

		my_render.setEBO();

		//store vertice info to glVAO (position and texture)
		for (int i = 0; i < vertice_size; ++i)
		{
			my_render.setVAO(
				result[i], result[vertice_size+i], 0,		//position cord
				1.0f, 0.0f, 0.0f,							//colors
				mesh.vertex_handle[i].tex_x, mesh.vertex_handle[i].tex_y	//texture cord
			);
		}
		cv::namedWindow("linear", CV_WINDOW_NORMAL);
		cv::resizeWindow("linear", comp.cols*w_r, comp.rows*h_r);
		cv::imshow("linear", comp);

		my_render.render();
		
	}
	catch (IloAlgorithm::CannotExtractException &e) {
		
		IloExtractableArray &failed = e.getExtractables();
		std::cerr << "Failed to extract:" << std::endl;
		for (IloInt i = 0; i < failed.getSize(); ++i)
			std::cerr << "\t" << failed[i] << std::endl;
	}
	catch (IloException& e)
	{
		std::cerr << "IloException: " << e << std::endl;
	}
	catch (std::exception& e)
	{
		std::cerr << "Standard exception: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Some other exception!" << std::endl;
	}
	
	/*
	//create my render class

	MyGL my_render(grid_col, grid_row);
	my_render.readTexPic(argv[3]);
	my_render.setEBO();

	//store vertice info to glVAO (position and texture)
	
	
	for (auto it = mesh.vertex_handle.begin(); it != mesh.vertex_handle.end(); ++it)
	{
		// std::cout << mesh.point(*v_it) << ": "
		//  << mesh.texcoord2D(*v_it) << std::endl;
		my_render.setVAO(
			it->x, it->y, 0,		//position cord
			it->tex_x, it->tex_y	//texture cord
		);
	}

	my_render.render();
	*/
	

	return 0;
}



