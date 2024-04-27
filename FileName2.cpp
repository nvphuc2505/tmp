#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <vector>

#include <assert.h>
#include <assimp/Importer.hpp>		
#include <assimp/scene.h>			
#include <assimp/postprocess.h>	

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/normalize_dot.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

int g_gl_width = 1280;
int g_gl_height = 780;

GLuint framebuffer;
GLuint framebufferTexPos;
GLuint framebufferTexNor;

GLuint modelVAO = 0;
int modelPointCount = 0;

GLuint sphereVAO;
int spherePointCount;

GLuint modelShaderProgramme;
GLuint modelProjMatrixLocation;
GLuint modelModelMatrixLocation;
GLuint modelViewMatrixLocation;

GLint sphereShaderProgramme;
GLuint sphereProjMatrixLocation;
GLuint sphereViewMatrixLocation;
GLuint sphereModelMatrixLocation;
GLuint sphereIDLocation;
GLuint sphereISLocation;
GLuint sphereIPLocation;
GLuint sphereTPosLocation;
GLuint sphereTNorLocation;

// ================================ FUCNTION IMPLEMENT ================================
void checkShaderCompilation(GLuint shader) {
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<char> errorLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		std::cerr << "Shader compilation failed: " << &errorLog[0] << std::endl;
	}
}
void checkProgramLinking(GLuint program) {
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		GLint maxLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<char> errorLog(maxLength);
		glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);

		std::cerr << "Program linking failed: " << &errorLog[0] << std::endl;
	}
}
bool initial_framebuffer()
{
	glGenFramebuffers(1, &framebuffer);

	glGenTextures(1, &framebufferTexPos);
	glBindTexture(GL_TEXTURE_2D, framebufferTexPos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, g_gl_width, g_gl_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &framebufferTexNor);
	glBindTexture(GL_TEXTURE_2D, framebufferTexNor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, g_gl_width, g_gl_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLuint depthTex = 0;
	glGenTextures(1, &depthTex);
	glActiveTexture(GL_TEXTURE0); 
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, g_gl_width, g_gl_height, 0, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexPos, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, framebufferTexNor, 0);

	GLenum drawBuffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (GL_FRAMEBUFFER_COMPLETE != status) {
		std::cerr << "ERROR: incomplete framebuffer\n";
		return false;
	}

	return true;
}
bool load_sphere_file(const char* fileName, GLuint& sphereVAO, int& pointCount)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate);

	if (!scene)
	{
		std::cerr << "ERROR: reading mesh" << fileName << '\n';
		return false;
	}

	std::cout << "There are " << scene->mNumAnimations << " animations\n";
	std::cout << "There are " << scene->mNumCameras << " cameras\n";
	std::cout << "There are " << scene->mNumLights << " lights\n";
	std::cout << "There are " << scene->mNumMaterials << " materials\n";
	std::cout << "There are " << scene->mNumMeshes << " meshes\n";
	std::cout << "There are " << scene->mNumTextures << " textures\n";

	const aiMesh* mesh = scene->mMeshes[0];
	pointCount = mesh->mNumVertices;
	std::cout << "There are " << pointCount << " vertices in mesh[0]\n";

	GLfloat* points = nullptr;
	GLfloat* normals = nullptr;
	GLfloat* texcoords = nullptr;

	if (mesh->HasPositions())
	{
		points = new GLfloat[sizeof(GLfloat) * 3 * pointCount];
		for (int i = 0; i < pointCount; i++)
		{
			aiVector3D* vp = &(mesh->mVertices[i]);
			points[i * 3 + 0] = vp->x;
			points[i * 3 + 1] = vp->y;
			points[i * 3 + 2] = vp->z;
		}
	}
	if (mesh->HasNormals())
	{
		normals = new GLfloat[sizeof(GLfloat) * 3 * pointCount];
		for (int i = 0; i < pointCount; i++)
		{
			aiVector3D* vn = &(mesh->mNormals[i]);
			normals[i * 3 + 0] = vn->x;
			normals[i * 3 + 1] = vn->y;
			normals[i * 3 + 2] = vn->z;
		}
	}
	if (mesh->HasTextureCoords(0))
	{
		texcoords = new GLfloat[sizeof(GLfloat) * 2 * pointCount];
		for (int i = 0; i < pointCount; i++)
		{
			aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
			texcoords[i * 2 + 0] = vt->x;
			texcoords[i * 2 + 1] = vt->y;
		}
	}

	glGenVertexArrays(1, &sphereVAO);
	glBindVertexArray(sphereVAO);

	// VAO
	if (mesh->HasPositions())
	{
		GLuint spherePointsVBO = 0;
		glGenBuffers(1, &spherePointsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, spherePointsVBO);
		glBufferData(GL_ARRAY_BUFFER, 3.0f * pointCount * sizeof(GLfloat), points, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
	}

	delete[] points;
	delete[] normals;
	delete[] texcoords;

	std::cout << "MESH LOADED\n";

	importer.FreeScene();
	return true;
}
bool load_model_file(const char* fileName, GLuint& vao, int& pointCount)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate);

	if (!scene)
	{
		std::cerr << "ERROR: reading mesh" << fileName << '\n';
		return false;
	}

	std::cout << "There are " << scene->mNumAnimations << " animations\n";
	std::cout << "There are " << scene->mNumCameras << " cameras\n";
	std::cout << "There are " << scene->mNumLights << " lights\n";
	std::cout << "There are " << scene->mNumMaterials << " materials\n";
	std::cout << "There are " << scene->mNumMeshes << " meshes\n";
	std::cout << "There are " << scene->mNumTextures << " textures\n";

	const aiMesh* mesh = scene->mMeshes[0];
	pointCount = mesh->mNumVertices;
	std::cout << "There are " << pointCount << " vertices in mesh[0]\n";

	GLfloat* points = nullptr;
	GLfloat* normals = nullptr;
	GLfloat* texcoords = nullptr;

	if (mesh->HasPositions())
	{
		points = new GLfloat[sizeof(GLfloat) * 3 * pointCount];
		for (int i = 0; i < pointCount; i++)
		{
			aiVector3D* vp = &(mesh->mVertices[i]);
			points[i * 3 + 0] = vp->x;
			points[i * 3 + 1] = vp->y;
			points[i * 3 + 2] = vp->z;
		}
	}
	if (mesh->HasNormals())
	{
		normals = new GLfloat[sizeof(GLfloat) * 3 * pointCount];
		for (int i = 0; i < pointCount; i++)
		{
			aiVector3D* vn = &(mesh->mNormals[i]);
			normals[i * 3 + 0] = vn->x;
			normals[i * 3 + 1] = vn->y;
			normals[i * 3 + 2] = vn->z;
		}
	}
	if (mesh->HasTextureCoords(0))
	{
		texcoords = new GLfloat[sizeof(GLfloat) * 2 * pointCount];
		for (int i = 0; i < pointCount; i++)
		{
			aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
			texcoords[i * 2 + 0] = vt->x;
			texcoords[i * 2 + 1] = vt->y;
		}
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// VAO
	if (mesh->HasPositions())
	{
		GLuint pointsVBO = 0;
		glGenBuffers(1, &pointsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
		glBufferData(GL_ARRAY_BUFFER, 3.0f * pointCount * sizeof(GLfloat), points, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
		delete[] points;
	}
	if (mesh->HasNormals())
	{
		GLuint normalsVBO = 0;
		glGenBuffers(1, &normalsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
		glBufferData(GL_ARRAY_BUFFER, 3.0f * pointCount * sizeof(GLfloat), normals, GL_STATIC_DRAW);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);
		delete[] normals;
	}
	
	delete[] texcoords;

	std::cout << "MESH LOADED\n";

	importer.FreeScene();
	return true;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(g_gl_width, g_gl_height, "Shadowing", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();

	// ======================================================================================

	initial_framebuffer();
	load_model_file("plane.obj", modelVAO, modelPointCount);
	
	const char* first_pass_vertex_shader =
		"#version 330 core\n"

		"layout(location = 0) in vec3 first_vp;"
		"layout(location = 1) in vec3 first_vn;"

		"uniform mat4 first_proj;"
		"uniform mat4 first_view;"
		"uniform mat4 first_model;"

		"out vec3 p_eye;"
		"out vec3 n_eye;"

		"void main()"
		"{"
			"p_eye = (first_view * first_model * vec4(first_vp, 1.0f)).xyz;"
			"n_eye = (first_view * first_model * vec4(first_vn, 0.0f)).xyz;"
			"gl_Position = first_proj * vec4(p_eye, 1.0f);"
		"}";
	const char* first_pass_fragment_shader =
		"#version 330 core\n"

		"in vec3 p_eye;"
		"in vec3 n_eye;"

		"layout(location = 0) out vec3 def_p;"
		"layout(location = 1) out vec3 def_n;"

		"void main()"
		"{"
			"def_p = p_eye;"
			"def_n = n_eye;"
		"}";

	GLuint first_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(first_vs, 1, &first_pass_vertex_shader, NULL);
	glCompileShader(first_vs);
	checkShaderCompilation(first_vs);
	GLuint first_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(first_fs, 1, &first_pass_fragment_shader, NULL);
	glCompileShader(first_fs);
	checkShaderCompilation(first_fs);

	modelShaderProgramme = glCreateProgram();
	glAttachShader(modelShaderProgramme, first_vs);
	glAttachShader(modelShaderProgramme, first_fs);
	glBindAttribLocation(modelShaderProgramme, 0, "first_vp");
	glBindAttribLocation(modelShaderProgramme, 1, "first_vn");
	glLinkProgram(modelShaderProgramme);
	checkProgramLinking(modelShaderProgramme);

	glBindFragDataLocation(modelShaderProgramme, 0, "def_p");
	glBindFragDataLocation(modelShaderProgramme, 1, "def_n");

	modelModelMatrixLocation = glGetUniformLocation(modelShaderProgramme, "first_model");
	modelProjMatrixLocation = glGetUniformLocation(modelShaderProgramme, "first_proj");
	modelViewMatrixLocation = glGetUniformLocation(modelShaderProgramme, "first_view");

	// Matrix TMP
	glm::mat4 proj = glm::perspective(glm::radians(67.0f), (float)g_gl_width / (float)g_gl_height, 0.1f, 1000.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 30.0f, 30.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(200.0f, 1.0f, 200.0f));
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));



	load_sphere_file("1sphere.obj", sphereVAO, spherePointCount);
	const char* second_vertex_shader =
		"#version 330 core\n"

		"layout (location = 0) in vec3 vp;"

		"uniform mat4 second_proj;"
		"uniform mat4 second_view;"
		"uniform mat4 second_model;"

		"void main()"
		"{"
			"gl_Position = second_proj * second_view * second_model * vec4(vp, 1.0f);"
		"}";
	const char* second_fragment_shader =
		"#version 330 core\n"

		"uniform mat4 view;"
		"uniform sampler2D p_tex;"
		"uniform sampler2D n_tex;"
		"uniform vec3 ls;"
		"uniform vec3 ld;"
		"uniform vec3 lp;"

		"out vec4 frag_colour;"

		"vec3 kd = vec3(1.0f, 1.0f, 1.0f);"
		"vec3 ks = vec3(1.0f, 1.0f, 1.0f);"
		"float specular_exponent = 100.0f;"

		"vec3 phong(in vec3 p_eye, in vec3 n_eye)"
		"{"
			"vec3 light_position_eye = vec3(view * vec4(lp, 1.0f));"
			"vec3 dist_to_light_eye = light_position_eye - p_eye;"
			"vec3 direction_to_light_eye = normalize(dist_to_light_eye);"

			"float dot_prod = max(dot(direction_to_light_eye, n_eye), 0.0f);"
			"vec3 Id = ld * kd * dot_prod;"

			"vec3 reflection_eye = reflect(-direction_to_light_eye, n_eye);"
			"vec3 surface_to_viewer_eye = normalize(-p_eye);"
			"float dot_prod_specular = dot(reflection_eye, surface_to_viewer_eye);"
			"dot_prod_specular = max(dot_prod_specular, 0.0);"
			"float specular_factor = pow(dot_prod_specular, specular_exponent);"
			"vec3 Is = ls * ks * specular_factor;"

			"float dist_2d = distance(light_position_eye, p_eye) / 5.0f;"
			"float atten_factor = max(0.0f, 1.0f - dist_2d);"

			" return (Id + Is) * atten_factor;"
		"}"

		"void main()"
		"{"
			"vec2 st;"
			"st.s = gl_FragCoord.x / 800.0f;"
			"st.t = gl_FragCoord.y / 600.0f;"

			"vec4 p_texel = texture(p_tex, st);"

			"if (p_texel.z > -0.0001)"
			"{"
				"discard;"
			"}"

			"vec4 n_texel = texture (n_tex, st);"
			
			"frag_colour.rgb = phong(p_texel.rgb, normalize(n_texel.rgb));"
			"frag_colour.a = 1.0f;"
		"}";

	GLuint second_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(second_vs, 1, &second_vertex_shader, NULL);
	glCompileShader(second_vs);
	checkShaderCompilation(second_vs);

	GLuint second_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(second_fs, 1, &second_fragment_shader, NULL);
	glCompileShader(second_fs);
	checkShaderCompilation(second_fs);

	sphereShaderProgramme = glCreateProgram();
	glAttachShader(sphereShaderProgramme, second_vs);
	glAttachShader(sphereShaderProgramme, second_fs);
	glBindAttribLocation(sphereShaderProgramme, 0, "vp");
	glLinkProgram(sphereShaderProgramme);
	checkProgramLinking(sphereShaderProgramme);

	sphereViewMatrixLocation = glGetUniformLocation(sphereShaderProgramme, "second_view");
	sphereProjMatrixLocation = glGetUniformLocation(sphereShaderProgramme, "second_proj");
	sphereModelMatrixLocation = glGetUniformLocation(sphereShaderProgramme, "second_model");
	sphereIDLocation = glGetUniformLocation(sphereShaderProgramme, "ld");
	sphereISLocation = glGetUniformLocation(sphereShaderProgramme, "ls");
	sphereIPLocation = glGetUniformLocation(sphereShaderProgramme, "lp");
	sphereTPosLocation = glGetUniformLocation(sphereShaderProgramme, "p_tex");
	sphereTNorLocation = glGetUniformLocation(sphereShaderProgramme, "n_tex");

	glUseProgram(sphereShaderProgramme);
	glUniform1i(sphereTPosLocation, 0);
	glUniform1i(sphereTNorLocation, 1);


	glm::vec3 lp[64];
	glm::vec3 ls[64];
	glm::vec3 ld[64];

	for (int i = 0; i < 64; i++)
	{
		float x = -sinf((float)i * 0.5f) * 5.0f;
		float y = cosf((float)i * 0.5f) * 5.0f;
		float z = (float)-i * 2.0f + 10.0f; 
		lp[i] = glm::vec3(x, y, z);
	}

	glm::mat4 lM[64];
	const float radius = 5.0f;

	int redi = 0;
	int bluei = 1;
	int greeni = 2;

	for (int i = 0; i < 64; i++) {
		lM[i] = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, radius));
		lM[i] = glm::translate(lM[i], lp[i]);

		ld[i] = glm::vec3((float)((redi + 1) / 3), (float)((greeni + 1) / 3), (float)((bluei + 1) / 3));
		ls[i] = glm::vec3(1.0f, 1.0f, 1.0f);

		redi = (int) (redi + 1) % 3;
		bluei = (int) (bluei + 1) % 3;
		greeni = (int) (greeni + 1) % 3;
	}

	glViewport(0, 0, g_gl_width, g_gl_height);
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK);    // cull back face
	glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise

	while (!glfwWindowShouldClose(window))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);

		glUseProgram(modelShaderProgramme);
		glBindVertexArray(modelVAO);

		glUniformMatrix4fv(modelModelMatrixLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(modelProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(proj));
		glUniformMatrix4fv(modelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(view));

		glDrawArrays(GL_TRIANGLES, 0, modelPointCount);

	// ===================================================================================

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glClearColor(0.2f, 0.2f, 0.2f, 0.0f); // added ambient light here
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE); // addition each time

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebufferTexPos);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, framebufferTexNor);

		glUseProgram(sphereShaderProgramme);

		glBindVertexArray(sphereVAO);

		glUniformMatrix4fv(sphereProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(proj));
		glUniformMatrix4fv(sphereViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(view));

		for (int i = 0; i < 64; i++)
		{
			glUniformMatrix4fv(sphereModelMatrixLocation, 1, GL_FALSE, glm::value_ptr(lM[i]));
			glUniform3f(sphereIDLocation, ld[i].x, ld[i].y, ld[i].z);
			glUniform3f(sphereISLocation, ls[i].x, ls[i].y, ls[i].z);
			glUniform3f(sphereIPLocation, lp[i].x, lp[i].y, lp[i].z);

			glDrawArrays(GL_TRIANGLES, 0, spherePointCount);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}