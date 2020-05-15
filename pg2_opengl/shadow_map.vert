#version 450 core
// vertex attributes
layout ( location = 0 ) in vec4 in_position_ms;

// uniform variables
// Projection (P_l)*Light (V_l)*Model (M) matrix

uniform mat4 mlp;

void main( void )
{
	//gl_Position = mlp * vec3(in_position_ms.x, in_position_ms.y, in_position_ms.z);
	gl_Position = mlp * in_position_ms;
}
