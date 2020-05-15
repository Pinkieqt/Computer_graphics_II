#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

in vec3 unified_normal_es;
in vec2 texcoord;

in float normalLightDot;

flat in int material_index;

struct Material
{
	vec3 diffuse; // (1,1,1) or albedo
	sampler2D tex_diffuse;
	//uint64_t tex_diffuse; // albedo texture
	vec3 rma; // (1,1,1) or (roughness, metalness, 1)
	uint64_t tex_rma; // rma texture
	vec3 normal; // (1,1,1) or (0,0,1)
	uint64_t tex_normal; // bump texture
};

layout ( std430, binding = 0 ) readonly buffer Materials
{
	Material materials[]; // only the last member can be unsized array
};


out vec4 FragColor;

void main( void )
{
	//Normalový
	FragColor = vec4((unified_normal_es.x + 1) * 0.5, (unified_normal_es.y + 1) * 0.5, (unified_normal_es.z + 1) * 0.5, 1.0f );

	

	//S material texturama
	//FragColor = vec4(materials[material_index].diffuse.rgb * texture(materials[material_index].tex_diffuse, texcoord).rgb, 1.0f ); // * normalLightDot

}
