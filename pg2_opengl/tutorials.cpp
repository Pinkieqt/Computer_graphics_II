#include "pch.h"
#include "tutorials.h"
#include "utils.h"

/* OpenGL check state */
bool check_gl( const GLenum error )
{
	if ( error != GL_NO_ERROR )
	{
		//const GLubyte * error_str;
		//error_str = gluErrorString( error );
		//printf( "OpenGL error: %s\n", error_str );

		return false;
	}

	return true;
}

/* glfw callback */
void glfw_callback( const int error, const char * description )
{
	printf( "GLFW Error (%d): %s\n", error, description );
}

/* OpenGL messaging callback */
void GLAPIENTRY gl_callback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * user_param )
{
	printf( "GL %s type = 0x%x, severity = 0x%x, message = %s\n",
		( type == GL_DEBUG_TYPE_ERROR ? "Error" : "Message" ),
		type, severity, message );
}

/* invoked when window is resized */
void framebuffer_resize_callback( GLFWwindow * window, int width, int height )
{
	glViewport( 0, 0, width, height );
}

/* load shader code from text file */
char * LoadShader( const char * file_name )
{
	FILE * file = fopen( file_name, "rt" );

	if ( file == NULL )
	{
		printf( "IO error: File '%s' not found.\n", file_name );

		return NULL;
	}

	size_t file_size = static_cast< size_t >( GetFileSize64( file_name ) );
	char * shader = NULL;

	if ( file_size < 1 )
	{
		printf( "Shader error: File '%s' is empty.\n", file_name );
	}
	else
	{
		/* v glShaderSource nezad�v�me v posledn�m parametru d�lku,
		tak�e �et�zec mus� b�t null terminated, proto +1 a reset na 0*/
		shader = new char[file_size + 1];
		memset( shader, 0, sizeof( *shader ) * ( file_size + 1 ) );

		size_t bytes = 0; // po�et ji� na�ten�ch byt�

		do
		{
			bytes += fread( shader, sizeof( char ), file_size, file );
		} while ( !feof( file ) && ( bytes < file_size ) );

		if ( !feof( file ) && ( bytes != file_size ) )
		{
			printf( "IO error: Unexpected end of file '%s' encountered.\n", file_name );
		}
	}

	fclose( file );
	file = NULL;

	return shader;
}

/* check shader for completeness */
GLint CheckShader( const GLenum shader )
{
	GLint status = 0;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

	printf( "Shader compilation %s.\n", ( status == GL_TRUE ) ? "was successful" : "FAILED" );

	if ( status == GL_FALSE )
	{
		int info_length = 0;
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &info_length );
		char * info_log = new char[info_length];
		memset( info_log, 0, sizeof( *info_log ) * info_length );
		glGetShaderInfoLog( shader, info_length, &info_length, info_log );

		printf( "Error log: %s\n", info_log );

		SAFE_DELETE_ARRAY( info_log );
	}

	return status;
}

/* create a window and initialize OpenGL context */
int tutorial_1( const int width, const int height )
{
	glfwSetErrorCallback( glfw_callback );

	if ( !glfwInit() )
	{
		return( EXIT_FAILURE );
	}

	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	glfwWindowHint( GLFW_SAMPLES, 8 );
	glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
	glfwWindowHint( GLFW_DOUBLEBUFFER, GL_TRUE );	

	GLFWwindow * window = glfwCreateWindow( width, height, "PG2 OpenGL", nullptr, nullptr );
	if ( !window )
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwSetFramebufferSizeCallback( window, framebuffer_resize_callback );
	glfwMakeContextCurrent( window );

	if ( !gladLoadGLLoader( ( GLADloadproc )glfwGetProcAddress ) )
	{
		if ( !gladLoadGL() )
		{
			return EXIT_FAILURE;
		}
	}
	
	glEnable( GL_DEBUG_OUTPUT );
	glDebugMessageCallback( gl_callback, nullptr );
	
	printf( "OpenGL %s, ", glGetString( GL_VERSION ) );
	printf( "%s", glGetString( GL_RENDERER ) );
	printf( " (%s)\n", glGetString( GL_VENDOR ) );
	printf( "GLSL %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );	

	check_gl();

	glEnable( GL_MULTISAMPLE );
	
	// map from the range of NDC coordinates <-1.0, 1.0>^2 to <0, width> x <0, height>
	glViewport( 0, 0, width, height );
	// GL_LOWER_LEFT (OpenGL) or GL_UPPER_LEFT (DirectX, Windows) and GL_NEGATIVE_ONE_TO_ONE or GL_ZERO_TO_ONE
	glClipControl( GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE );

	// setup vertex buffer as AoS (array of structures)
	GLfloat vertices[] =
	{
		-0.9f, 0.9f, 0.0f,  0.0f, 1.0f, // vertex 0 : p0.x, p0.y, p0.z, t0.u, t0.v
		0.9f, 0.9f, 0.0f,   1.0f, 1.0f, // vertex 1 : p1.x, p1.y, p1.z, t1.u, t1.v
		0.0f, -0.9f, 0.0f,  0.5f, 0.0f  // vertex 2 : p2.x, p2.y, p2.z, t2.u, t2.v
	};
	const int no_vertices = 3;
	const int vertex_stride = sizeof( vertices ) / no_vertices;	
	// optional index array
	unsigned int indices[] =
	{  
		0, 1, 2
	};

	GLuint vao = 0;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
	GLuint vbo = 0;
	glGenBuffers( 1, &vbo ); // generate vertex buffer object (one of OpenGL objects) and get the unique ID corresponding to that buffer
	glBindBuffer( GL_ARRAY_BUFFER, vbo ); // bind the newly created buffer to the GL_ARRAY_BUFFER target
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW ); // copies the previously defined vertex data into the buffer's memory
	// vertex position
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, vertex_stride, 0 );
	glEnableVertexAttribArray( 0 );
	// vertex texture coordinates
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, vertex_stride, ( void* )( sizeof( float ) * 3 ) );
	glEnableVertexAttribArray( 1 );

	
	GLuint ebo = 0; // optional buffer of indices
	glGenBuffers( 1, &ebo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
	const char * vertex_shader_source = LoadShader( "basic_shader.vert" );
	glShaderSource( vertex_shader, 1, &vertex_shader_source, nullptr );
	glCompileShader( vertex_shader );
	SAFE_DELETE_ARRAY( vertex_shader_source );
	CheckShader( vertex_shader );
	
	GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
	const char * fragment_shader_source = LoadShader( "basic_shader.frag" );
	glShaderSource( fragment_shader, 1, &fragment_shader_source, nullptr );
	glCompileShader( fragment_shader );
	SAFE_DELETE_ARRAY( fragment_shader_source );
	CheckShader( fragment_shader );

	GLuint shader_program = glCreateProgram();
	glAttachShader( shader_program, vertex_shader );
	glAttachShader( shader_program, fragment_shader );
	glLinkProgram( shader_program );
	// TODO check linking
	glUseProgram( shader_program );
	
	glPointSize( 10.0f );	
	glLineWidth( 2.0f );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	
	// main loop
	while ( !glfwWindowShouldClose( window ) )
	{		
		glClearColor( 0.2f, 0.3f, 0.3f, 1.0f ); // state setting function
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT ); // state using function

		glBindVertexArray( vao );
		//glDrawArrays( GL_TRIANGLES, 0, 3 );
		glDrawArrays( GL_POINTS, 0, 3 );
		glDrawArrays( GL_LINE_LOOP, 0, 3 );
		glDrawElements( GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0 ); // optional - render from an index buffer

		glfwSwapBuffers( window );
		glfwPollEvents();
	}

	glDeleteShader( vertex_shader );
	glDeleteShader( fragment_shader );
	glDeleteProgram( shader_program );

	glDeleteBuffers( 1, &vbo );
	glDeleteVertexArrays( 1, &vao );

	glfwTerminate();

	return EXIT_SUCCESS;
}
