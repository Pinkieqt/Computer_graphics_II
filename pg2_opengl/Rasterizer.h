#pragma once

#include "surface.h"
#include "camera.h"

class Rasterizer
{
public:
	Rasterizer();
	Rasterizer(const int width, const int height, const float fov_y, const Vector3 view_from, const Vector3 view_at, Vector3 light);

	//functions
	int InitDevice();
	int LoadSceneAndObject(const char * fileName);
	void InitBuffers(std::string shader);
	int InitMaterials();
	void InitIrradianceMap(const char * path);
	void InitEnvMaps(std::vector<const char*> paths);
	void InitGGXIntegrMap(const char * path);
	int RenderFrame(bool rotate, bool includeShadows);

	//Shadow mapping
	int InitShadowDepthBuffer();

	void move();

	void genMipMap();

private: 
	bool obtainMVN;
	int width_;
	int height_;
	Camera camera_;
	GLFWwindow* window_;
	int no_triangles_;
	std::vector<Surface *> surfaces_;
	std::vector<Material *> materials_;

	Vector3 light_position;

	GLuint vao_{ 0 };
	GLuint vbo_{ 0 }; 
	GLuint shader_program_;

	//Irradiance
	GLuint irradianceMap{ 0 };
	GLuint brdfMap{ 0 };
	GLuint envMap{ 0 };

	//Shadow mapping
	int shadow_width_{ 1024 }; // shadow map resolution
	int shadow_height_{ shadow_width_ };
	GLuint fbo_shadow_map_{ 0 }; // shadow mapping FBO
	GLuint tex_shadow_map_{ 0 }; // shadow map texture
	GLuint shadow_program_{ 0 }; // collection of shadow mapping shaders

};

