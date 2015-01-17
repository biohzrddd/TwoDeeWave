
/* Project Goals

ALL
-Get a window open

1-D
-Get a line of points plotted
-Implement the 1-D Burger's Equation
--Port over existing fortran
---Make it opencl-ified
--Use variable discretization

2-D
-Get a grid of triangles plotted
-Assign colors to each vertex based on initialization
-Copy the triangle mesh three times - one for u, v, and w
-Create the shared context
-Implement the 2-d Burger's Equations
--Port over existing fortran
---Make it opencl-ified
--Use variable discretization
-Time update rates

-Click on a point and change/increase value
-Initialize with a drawing

*/
#include <chrono>
#include <iostream>
#include <ctime>
#include <conio.h>

#include "TwoDee.h"
int main(void)
{
	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();


	CL_DATA_TYPE R = 1.0f;
	
	// Fill a full period (0 to 2*pi) with vertices
	cl_int numVerticesX = (cl_int)(pow<int, int>(2, 12) + 2); //  (2 ^ 13)
	CL_DATA_TYPE dx = (CL_DATA_TYPE)(2.0*3.141592653589793) / (CL_DATA_TYPE)numVerticesX;
	CL_DATA_TYPE dt = dx * dx * 0.5f;

	std::cout << "dx: " << dx << std::endl;
	std::cout << "dt: " << dt << std::endl;
	std::cout << "Nx: " << numVerticesX << std::endl;

	OneDee thing(numVerticesX, R, dx, dt);

	thing.Loop();

	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	std::cout << "Overall elapsed time: " << elapsed_seconds.count() << "s\n";

	std::cout << "Press the any key to continue" << std::endl;
	_getch();
	exit(true);
}