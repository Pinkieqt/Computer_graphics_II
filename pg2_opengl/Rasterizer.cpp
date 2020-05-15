#include "pch.h"
#include "Rasterizer.h"
#include "objloader.h"
#include "utils.h"
#include "matrix4x4.h"
#include "matrix3x3.h"
#include "glutils.h"
#include "mymath.h"
#include "tutorials.h"
#include "texture.h"

using namespace std;

Rasterizer::Rasterizer() {};

Rasterizer::Rasterizer(const int width, const int height, const float fov_y, const Vector3 view_from, const Vector3 view_at, Vector3 light)
{
	this->light_position = light;
	this->width_ = width;
	this->height_ = height;
	camera_ = Camera(width, height, fov_y, view_from, view_at);
}

// device inicialization
int Rasterizer::InitDevice()
{
	glfwSetErrorCallback(glfw_callback);

	if (!glfwInit())
	{
		return(EXIT_FAILURE);
	}
	//change glfw version for my gpu
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

	window_ = glfwCreateWindow(width_, height_, "PG2 OpenGL", nullptr, nullptr);
	if (!window_)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);
	glfwMakeContextCurrent(window_);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		if (!gladLoadGL())
		{
			return EXIT_FAILURE;
		}
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_callback, nullptr);

	printf("OpenGL %s, ", glGetString(GL_VERSION));
	printf("%s", glGetString(GL_RENDERER));
	printf(" (%s)\n", glGetString(GL_VENDOR));
	printf("GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	check_gl(glad_glGetError());

	glEnable(GL_MULTISAMPLE);

	// map from the range of NDC coordinates <-1.0, 1.0>^2 to <0, width> x <0, height>
	glViewport(0, 0, width_, height_);
	// GL_LOWER_LEFT (OpenGL) or GL_UPPER_LEFT (DirectX, Windows) and GL_NEGATIVE_ONE_TO_ONE or GL_ZERO_TO_ONE
	glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);

	return S_OK;
}

//2. load obj and scene
int Rasterizer::LoadSceneAndObject(const char* fileName) {
	const int surfaces_count = LoadOBJ(fileName, surfaces_, materials_);
	no_triangles_ = 0;

	for (Surface* surface : surfaces_)
	{
		no_triangles_ += surface->no_triangles();
	}

	//Inicializase bufferů
	const int numOfVertices = no_triangles_ * 3;
	Vertex* vertices = new Vertex[numOfVertices];

	const int vertex_stride = sizeof(Vertex);

	int k = 0;
	for (Surface* surface : surfaces_)
	{
		for (int i = 0; i < surface->no_triangles(); i++)
		{
			Triangle & triangle = surface->get_triangle(i);
			for (int j = 0; j < 3; j++)
			{
				Vertex v = triangle.vertex(j);
				v.material_index = surface->get_material()->materialIndex;
				Material * tmp = surface->get_material();
				vertices[k] = v;
				k++;
			}
		}
	}

	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);

	glGenBuffers(1, &vbo_); // generate vertex buffer object (one of OpenGL objects) and get the unique ID corresponding to that buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo_); // bind the newly created buffer to the GL_ARRAY_BUFFER target
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numOfVertices, vertices, GL_STATIC_DRAW); // copies the previously defined vertex data into the buffer's memory

	//vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	//vertex normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	//vertex color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(offsetof(Vertex, color)));
	glEnableVertexAttribArray(2);
	//vertex texture_coords
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(offsetof(Vertex, texture_coords)));
	glEnableVertexAttribArray(3);
	//vertex tangent
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(offsetof(Vertex, tangent)));
	glEnableVertexAttribArray(4);
	//vertex material_index
	glVertexAttribIPointer(5, 1, GL_INT, vertex_stride, (void*)(offsetof(Vertex, material_index)));
	glEnableVertexAttribArray(5);

	delete[] vertices;
	return S_OK;
}

//3. shaders
void Rasterizer::InitBuffers(std::string shader) {
	if (shader == "normal_shader")
		obtainMVN = true;
	else obtainMVN = false;

	//vertex shader
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const char * vertex_shader_source = LoadShader((shader + ".vert").c_str());
	glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
	glCompileShader(vertex_shader);
	SAFE_DELETE_ARRAY(vertex_shader_source);
	CheckShader(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const char * fragment_shader_source = LoadShader((shader + ".frag").c_str());
	glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
	glCompileShader(fragment_shader);
	SAFE_DELETE_ARRAY(fragment_shader_source);
	CheckShader(fragment_shader);

	shader_program_ = glCreateProgram();
	glAttachShader(shader_program_, vertex_shader);
	glAttachShader(shader_program_, fragment_shader);
	glLinkProgram(shader_program_);
	glUseProgram(shader_program_);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

//4. materials
int Rasterizer::InitMaterials() {
#pragma pack( push, 1 ) // 1 B alignment
	struct GLMaterial
	{
		Color3f diffuse; // 3 * 4 B
		GLbyte pad0[4]; // + 4 B = 16 B
		GLuint64 tex_diffuse_handle{ 0 }; // 1 * 8 B
		GLbyte pad1[8]; // + 8 B = 16 B
		Color3f rma; // 3 * 4 B
		GLbyte pad2[4]; // + 4 B = 16 B
		GLuint64 tex_rma_handle{ 0 }; // 1 * 8 B
		GLbyte pad3[8]; // + 8 B = 16 B
		Color3f normal; // 3 * 4 B
		GLbyte pad4[4]; // + 4 B = 16 B
		GLuint64 tex_normal_handle{ 0 }; // 1 * 8 B
		GLbyte pad5[8]; // + 8 B = 16 B
	};
#pragma pack( pop )

	GLMaterial * gl_materials = new GLMaterial[materials_.size()];

	int m = 0;
	for (const auto & material : materials_) {

		Texture3u * tex_diffuse = material->texture(Material::kDiffuseMapSlot);
		if (tex_diffuse) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, tex_diffuse->width(), tex_diffuse->height(), tex_diffuse->data());
			Color3f color = Color3f({ 1, 1, 1 }); //barva pod texturami
			gl_materials[m].diffuse = material->diffuse_; // white diffuse color
		}
		else {
			GLuint id = 0;
			GLubyte data[] = { 255, 255, 255, 255 };
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, 1, 1, data); // white texture
			gl_materials[m].diffuse = material->diffuse_;
		}

		//Přidat vlastnosti - ior, roughness, metalicness
		gl_materials[m].rma = Color3f({ material->roughness_, material->metallicness, material->ior });

		//bump mapa
		Texture3u * tex_normal = material->texture(Material::kNormalMapSlot);
		if (tex_normal) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_normal_handle, tex_normal->width(), tex_normal->height(), tex_normal->data());
		}

		//Texture3u * rma = material->texture(Material::kRMAMapSlot);
		//if (rma) {
		//	GLuint id = 0;
		//	CreateBindlessTexture(id, gl_materials[m].tex_rma_handle, rma->width(), rma->height(), rma->data());
		//}
		//else {
		//	GLuint id = 0;
		//	GLubyte data[] = { 255, 255, 255, 255 };
		//	CreateBindlessTexture(id, gl_materials[m].tex_rma_handle, 1, 1, data); // white texture
		//}

		m++;
	}

	GLuint ssbo_materials = 0;
	glGenBuffers(1, &ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_materials);
	const GLsizeiptr gl_materials_size = sizeof(GLMaterial) * materials_.size();
	glBufferData(GL_SHADER_STORAGE_BUFFER, gl_materials_size, gl_materials, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	SAFE_DELETE_ARRAY(gl_materials);

	return S_OK;
}


void Rasterizer::InitIrradianceMap(const char * path) {
	Texture3f* bitmap = new Texture3f(path);

	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, bitmap->width(), bitmap->height(), 0, GL_RGB, GL_FLOAT, bitmap->data());

	genMipMap();
}

void Rasterizer::InitEnvMaps(std::vector<const char*> paths) {

	glGenTextures(1, &envMap);
	glBindTexture(GL_TEXTURE_2D, envMap);

	int roughness = 0;
	for (auto path : paths) {
		Texture3f* bitmap = new Texture3f(path);
		glTexImage2D(GL_TEXTURE_2D, roughness, GL_RGB32F, bitmap->width(), bitmap->height(), 0, GL_RGB, GL_FLOAT, bitmap->data());
		roughness++;
	}
	genMipMap();
	SetInt(shader_program_, roughness, "envMap_roughness");
}

void Rasterizer::InitGGXIntegrMap(const char * path) {
	Texture3f* bitmap = new Texture3f(path);

	glGenTextures(1, &brdfMap);
	glBindTexture(GL_TEXTURE_2D, brdfMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, bitmap->width(), bitmap->height(), 0, GL_RGB, GL_FLOAT, bitmap->data());

	genMipMap();
}


//5. renderování frejmu
//PDF 1 - stránka 79 !!!
int Rasterizer::RenderFrame(bool rotate, bool includeShadows) {
	glBindVertexArray(vao_);

	int loc;
	loc = glGetUniformLocation(shader_program_, "irradianceMap");
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(shader_program_, "envMap");
	glUniform1i(loc, 1);
	loc = glGetUniformLocation(shader_program_, "brdfMap");
	glUniform1i(loc, 2);

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, envMap);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, brdfMap);
	
	if (includeShadows) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, tex_shadow_map_);
		SetSampler(shader_program_, 3, "shadow_map");
	}
	
	float speedOfRotation = deg2rad(45);
	while (!glfwWindowShouldClose(window_))
	{

		//Poloha modelu v 4x4 matici
		//PDF 1 - stránka 9
		//cosf, -sinf, sinf, cosf -> rotace proti směru hodinových ručiček
		Matrix4x4 model;
		model.set(0, 0, cosf(speedOfRotation));
		model.set(0, 1, -sinf(speedOfRotation));
		model.set(1, 0, sinf(speedOfRotation));
		model.set(1, 1, cosf(speedOfRotation));
		//speedOfRotation += 0.0009;
		if (rotate) {
			speedOfRotation += 0.0009;
		}


		const GLint lightPos = glGetUniformLocation(shader_program_, "lightPos");
		glUniform3fv(lightPos, 1, light_position.data);
		const GLint viewFrom = glGetUniformLocation(shader_program_, "viewFrom");
		glUniform3fv(viewFrom, 1, camera_.view_from_.data);

		//Moving camera and light from user inputs
		move();
		camera_.Update();

		Matrix4x4 mlp = camera_.BuildMLPMatrix(light_position);
		mlp = mlp * model;

		if (includeShadows) {
			// --- first pass ---
			// set the shadow shader program and the viewport to match the size of the depth map
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			// set up the light source through the MLP matrix
			glUseProgram(shadow_program_);
			glViewport(0, 0, shadow_width_, shadow_height_);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow_map_);
			glClear(GL_DEPTH_BUFFER_BIT);

			SetMatrix4x4(shadow_program_, mlp.data(), "mlp");

			// draw the scene
			glBindVertexArray(vao_);
			glDrawArrays(GL_TRIANGLES, 0, no_triangles_ * 3);
			glBindVertexArray(0);

			// set back the main shader program and the viewport
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, camera_.width_, camera_.height_);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(shader_program_);
		}


		//barva pozadí - background color
		glClearColor(0.f, 0.f, 0.f, 1.0f); // state setting function
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // state using function

		Matrix4x4 mvp = camera_.projectionMatrix * camera_.viewMatrix * model;
		SetMatrix4x4(shader_program_, mvp.data(), "mvp");

		if (obtainMVN) {
			Matrix4x4 mvn = model * camera_.viewMatrix;
			SetMatrix4x4(shader_program_, mvn.data(), "mvn");
		}

		if (includeShadows) {
			mlp = camera_.BuildMLPMatrix(light_position);
			mlp = mlp * model;
			SetMatrix4x4(shader_program_, mlp.data(), "mlp");
		}

		glBindVertexArray(vao_);
		glDrawArrays(GL_TRIANGLES, 0, no_triangles_ * 3);
		glBindVertexArray(0);

		glfwSwapBuffers(window_);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vao_);
	glDeleteBuffers(1, &vbo_);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glfwTerminate();


	return S_OK;
}


//PDF 129 - 142
int Rasterizer::InitShadowDepthBuffer()
{
	glGenTextures(1, &tex_shadow_map_); // texture to hold the depth values from the light's perspective
	glBindTexture(GL_TEXTURE_2D, tex_shadow_map_);
	// GL_DEPTH_COMPONENT ... each element is a single depth value. The GL converts it to floating point, multiplies by the signed scale
	// factor GL_DEPTH_SCALE, adds the signed bias GL_DEPTH_BIAS, and clamps to the range [0, 1] – this will be important later
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_width_, shadow_height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // areas outside the light's frustum will be lit
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenFramebuffers(1, &fbo_shadow_map_); // new frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow_map_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_shadow_map_, 0); // attach the texture as depth
	glDrawBuffer(GL_NONE); // we dont need any color buffer during the first pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind the default framebuffer back

	GLuint shadow_vertex = glCreateShader(GL_VERTEX_SHADER);
	const char * vertex_shader_source = LoadShader("shadow_map.vert");
	glShaderSource(shadow_vertex, 1, &vertex_shader_source, nullptr);
	glCompileShader(shadow_vertex);
	SAFE_DELETE_ARRAY(vertex_shader_source);
	CheckShader(shadow_vertex);

	GLuint shadow_fragment = glCreateShader(GL_FRAGMENT_SHADER);
	const char * fragment_shader_source = LoadShader("shadow_map.frag");
	glShaderSource(shadow_fragment, 1, &fragment_shader_source, nullptr);
	glCompileShader(shadow_fragment);
	SAFE_DELETE_ARRAY(fragment_shader_source);
	CheckShader(shadow_fragment);

	shadow_program_ = glCreateProgram();
	glAttachShader(shadow_program_, shadow_vertex);
	glAttachShader(shadow_program_, shadow_fragment);
	glLinkProgram(shadow_program_);
	glUseProgram(shadow_program_);

	glUseProgram(shader_program_);
	return 1;
}



void Rasterizer::move() {
	if (glfwGetKey(window_, GLFW_KEY_D) > 0)
	{
		light_position.x += 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_A) > 0)
	{
		light_position.x -= 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_W) > 0)
	{
		light_position.z += 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_S) > 0)
	{
		light_position.z -= 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_Q) > 0)
	{
		light_position.y += 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_E) > 0)
	{
		light_position.y -= 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_KP_6) > 0)
	{
		camera_.view_from_.x += 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_KP_4) > 0)
	{
		camera_.view_from_.x -= 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_KP_8) > 0)
	{
		camera_.view_from_.z += 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_KP_5) > 0)
	{
		camera_.view_from_.z -= 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_KP_7) > 0)
	{
		camera_.view_from_.y += 0.2;
	}
	if (glfwGetKey(window_, GLFW_KEY_KP_9) > 0)
	{
		camera_.view_from_.y -= 0.2;
	}

	printf("\rCamera: %f, %f, %f ... Light: %f, %f, %f ", camera_.view_from_.x, camera_.view_from_.y, camera_.view_from_.z, light_position.x, light_position.y, light_position.z);

}

void Rasterizer::genMipMap() {
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
}