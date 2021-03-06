#version 450 core
layout (location = 0) in vec4 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 3) in vec2 in_texcoord;
layout (location = 4) in vec3 in_tangent;
layout (location = 5) in int materialIdx;

out vec3 position; //position
out vec2 texcoord;	//texcoord
out vec3 normal;	//normal
flat out int material_index;

out vec3 light;
out vec3 camPos;

out vec3 position_lcs; // this is our point a (or b) in lcs

// uniform variables
// Projection (P_l)*Light (V_l)*Model (M) matrix
uniform mat4 mlp;
//input
uniform mat4 mvp; // View Projection
uniform vec3 lightPos;
uniform vec3 viewFrom;

void main( void )
{
	gl_Position = mvp * in_position; // model-space -> clip-space
		
	vec3 N = normalize(in_normal);
	vec3 T = normalize(in_tangent);

	position = vec3(in_position.x, in_position.y, in_position.z); 
	texcoord =  vec2(in_texcoord.x, 1.0 - in_texcoord.y);
	normal = N;
	material_index = materialIdx;

	light = lightPos;
	camPos = viewFrom;

	vec4 tmp = mlp * vec4(position.x, position.y, position.z, 1.0f);
	position_lcs = vec3(tmp.x, tmp.y, tmp.z) / tmp.w;
}
