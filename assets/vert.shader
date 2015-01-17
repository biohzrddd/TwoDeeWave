#version 400

//#define USEDOUBLE

#ifdef USEDOUBLE
in dvec4 position;
#else
in vec4 position;
#endif
out vec4 out_position;

void main() {
	vec4 newPos = vec4(position.x, position.y, position.z, position.w);
	newPos.x *= 0.318; // 2 / 6.28
	newPos.x -= 1;
	gl_Position = newPos;
	gl_PointSize = 2.0;
	out_position = newPos;
}