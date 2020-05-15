#include "pch.h"
#include "tutorials.h"
#include "Rasterizer.h"
#include "mymath.h"

int main()
{
	printf( "PG2 OpenGL, (c)2019 Tomas Fabian\n\n" );

	Rasterizer rasterizer;
	enum model { avenger, piece };
	enum shader { normal, pbr, shadow };

	bool includeEnvMap = true;
	bool includeShadows = false;

	//change model and shader here
	model m = avenger;
	shader s = shadow;
	std::string shader = "normal_shader"; //default shader


	switch (s) {
	case normal:
		shader = "normal_shader";
		break;

	case pbr:
		shader = "pbr";
		break;
	case shadow:
		includeShadows = true;
		shader = "pbr_shadow";
		break;
	}


	switch (m) {
	case avenger:
		includeEnvMap = true;
		rasterizer = Rasterizer(640, 480, deg2rad(45.0), Vector3(190, -103, 186), Vector3(0, 0, 30), Vector3(0, 1, 350));
		rasterizer.InitDevice();
		rasterizer.InitBuffers(shader);
		rasterizer.LoadSceneAndObject("../../data/6887_allied_avenger_gi2.obj");
		break;
	case piece:
		rasterizer = Rasterizer(640, 480, deg2rad(45.0), Vector3(25.19, -2.99, 15.99), Vector3(0, 0, 0), Vector3(-380.004791, 387.605255, -115.599396)); //mine close up
		rasterizer.InitDevice();
		rasterizer.InitBuffers(shader);
		rasterizer.LoadSceneAndObject("../../data/piece_02.obj");
		break;
	}

	//BRDF map
	rasterizer.InitGGXIntegrMap("../../data/brdf_integration_map_ct_ggx.png");
	
	//Irradiance
	rasterizer.InitIrradianceMap("../../data/lebombo_irradiance_map.exr");

	//Enviromental map
	if(includeEnvMap)
		rasterizer.InitEnvMaps({"../../data/lebombo_prefiltered_env_map_001_2048.exr",
								"../../data/lebombo_prefiltered_env_map_010_1024.exr",
								"../../data/lebombo_prefiltered_env_map_100_512.exr",
								"../../data/lebombo_prefiltered_env_map_250_256.exr",
								"../../data/lebombo_prefiltered_env_map_500_128.exr",
								"../../data/lebombo_prefiltered_env_map_750_64.exr",
								"../../data/lebombo_prefiltered_env_map_999_32.exr" });

	//Shadows
	if(includeShadows)
		rasterizer.InitShadowDepthBuffer();


	rasterizer.InitMaterials();

	rasterizer.RenderFrame(false, includeShadows);

	return 1;
}
