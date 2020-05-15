#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require

#define PI 3.14159265359

// ### Maps
uniform sampler2D irradianceMap;
uniform sampler2D envMap;
uniform sampler2D brdfMap;
uniform int envMap_roughness;

// ### Input from vertex shader
in vec3 position; //position
in vec3 normal;	//normal
in vec2 texcoord;	//texcoord
flat in int material_index;
in vec3 light;
in vec3 camPos;

const vec3 lightColour = vec3(1, 1, 1);

struct Material {
	vec3 diffuse; // (1,1,1) or albedo
	uint64_t tex_diffuse; // albedo texture
	vec3 rma; // (1,1,1) or (roughness, metalness, 1)
	uint64_t tex_rma; // rma texture
	vec3 normal; // (1,1,1) or (0,0,1)
	uint64_t tex_normal; // bump texture
};

layout (std430, binding = 0) readonly buffer Materials {
	Material materials[];
};

////////////////////////////////////
// Functions from Learnopengl
////////////////////////////////////
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
////////////////////////////////////
////////////////////////////////////

out vec4 FragColor;

void main( void )
{
	Material material = materials[material_index];

	//Albedo color
	vec3 albedo = material.diffuse.rgb * texture(sampler2D(material.tex_diffuse), texcoord).rgb;

	//Roughness, metallness and IOR
	vec3 rma = vec3(material.rma.r, material.rma.g, 1);
	float roughness = rma.r;
	float metalness = rma.g;
	float IOR = rma.b;

	//preparation
	vec3 n = normalize(normal);  //NORMALIZE!! 
	vec3 W0 = normalize(camPos - position); 
	vec3 Wi = reflect(-W0, n);
    vec3 L = normalize(light - position);
    vec3 H = normalize(L + W0);
	float cos0 = max(dot(n, W0), 0.0);

	//fresnel
    float HV = max(dot(H, W0), 0.0);
    vec3 F0 = vec3(pow((1 - IOR) / (1 + IOR), 2.0)); //0.034 plastic +-
    vec3 F  = F0 + (1.0 - F0) * pow(1.0 - HV, 5.0); 

    // Cook-Torrance brdf
    float NDF = DistributionGGX(n, H, roughness);   
    float G = GeometrySmith(n, W0, L, roughness);      
	 
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS * (1.0 - metalness);

	//Irradiance map, Enviromental maps and BRDF png map
	vec2 uv = SampleSphericalMap(n);
	vec3 irradiance = texture(irradianceMap, uv).rgb;

	//PrefEnvMap(Wi, roughness)
	uv = SampleSphericalMap(Wi);
	vec3 env = texture(envMap, uv, roughness * envMap_roughness).rgb;

	//(s, b) = BRDFIntMap(N_V, a)
	uv = vec2(cos0, roughness);
	vec2 brdf = texture(brdfMap, uv).rg;

	//Final color - complete prb ibl shader
	vec3 Lo = kD * ((albedo/PI) * irradiance) + (kS * brdf.x + brdf.y) * env;

	//Gamma correct - learnopengl
    Lo = Lo / (Lo + vec3(1.0));
    Lo = pow(Lo, vec3(1.0/2.2)); 

	//output color
	FragColor = vec4(Lo, 1.0f);
}
