#version 450 core
layout ( location = 0 ) in vec4 in_position_ms; //x, y, z, 1.0f
layout ( location = 1 ) in vec3 in_normal_ms;
layout ( location = 2 ) in vec3 in_color;
layout ( location = 3 ) in vec2 in_texcoord;
layout ( location = 4 ) in vec3 in_tangent;
layout ( location = 5 ) in int in_material_index;

uniform mat4 mvp; //model view projection
uniform mat4 mvn; //model view normal

uniform vec3 lightPos;
uniform vec3 viewFrom;

//Output variables
out vec3 unified_normal_es;
out vec2 texcoord;
out float normalLightDot;
flat out int material_index;

//shadow mapping
// uniform variables
// Projection (P_l)*Light (V_l)*Model (M) matrix
//uniform mat4 mlp;


void main( void )
{
	//model space -> clip space
	gl_Position =  mvp * vec4(in_position_ms.x, in_position_ms.y, in_position_ms.z , 1.0f);

	//PDF strana 118
	//normal vector transformations
	unified_normal_es = normalize((mvn * vec4(in_normal_ms.x, in_normal_ms.y, in_normal_ms.z, 0.0f)).xyz);
	vec4 hit_es = mvn * vec4(in_position_ms.x, in_position_ms.y, in_position_ms.z , 1.0f);
	vec3 omega_i_es = normalize( hit_es.xyz / hit_es.w );
	if ( dot( unified_normal_es, omega_i_es ) > 0.0f )
	{
		unified_normal_es *= -1.0f;
	}
	
	//texture coordinates 
	texcoord = vec2( in_texcoord.x, 1.0f - in_texcoord.y );
	material_index = in_material_index;

	vec3 vectorToLight_MS = normalize(lightPos - in_position_ms.xyz);
	vec3 vectorToLight_ES = normalize((mvn * vec4(vectorToLight_MS.x, vectorToLight_MS.y, vectorToLight_MS.z, 0.0f)).xyz);

	normalLightDot = dot(unified_normal_es, vectorToLight_ES.xyz);

}
