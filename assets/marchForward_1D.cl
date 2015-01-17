//#define USEDOUBLE

#ifdef USEDOUBLE
#define ARRAY_T double2
#define SCALAR_T double
#else
#define ARRAY_T float2
#define SCALAR_T float
#endif

__kernel void marchForward_1D(__global const ARRAY_T* uAtN1,
	__global ARRAY_T* uAtN2,
	SCALAR_T dx,
	SCALAR_T dt)
{
	/* get_global_id(0) returns the ID of the thread in execution.
	Since we fix the boundary conditions, we only queue up (numVerticesX - 2) work items,
	making the first work item (number 0) for the second vertex (number 1)
	--> Add one
	*/
	const int j = (get_global_id(0) + 1);

	__private ARRAY_T uAtN1_j = uAtN1[j];
	__private ARRAY_T uAtN1_jp1 = uAtN1[j + 1];
	__private ARRAY_T uAtN1_jm1 = uAtN1[j - 1];

	// Central differencing requires one point in front and one in back
	uAtN2[j].y = uAtN1_j.y
		- dt / (2.0*dx) * uAtN1_j.y * (uAtN1_jp1.y - uAtN1_jm1.y)
		+ dt / (dx*dx) * (uAtN1_jp1.y - 2.0 * uAtN1_j.y + uAtN1_jm1.y);

	//u(j, n + 1) = u(j, n) &
	//	& -dt / (2.0D0*dx) * u(j, n) * (u(j + 1, n) - u(j - 1, n)) &
	//	& +dt / (dx*dx) * (u(j + 1, n) - 2.0D0 * u(j, n) + u(j - 1, n))
}
