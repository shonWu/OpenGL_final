#include "../Externals/Include/Include.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_ORI 4
#define MENU_NORMALCOLOR 5
#define MENU_ABSTRACT 6
#define MENU_WATERCOLOR 7
#define MENU_BLOOM 8
#define MENU_PIXELIZATION 9
#define MENU_SINEWAVE 10
#define MENU_MAGNIFIER 11
#define MENU_FOG 12
#define SHADOW_MAP_SIZE 4096

#define M_PI 3.14159

float window_width = 600;
float window_height = 600;

float timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

bool ori_scene = true;

int current_x = 0, current_y = 0;
bool move_bar = false;
float current_bar = 300;

bool magnifier = false;
bool move_mag = false;
bool resize_mag = false;
int mag_rad = 100;
int mag_x = 300, mag_y = 300;

int useFog = 0;

using namespace glm;
using namespace std;

GLuint water_program;
GLuint water_quad_vao;
GLuint water_quad_vbo;

GLuint reflection_fbo;
GLuint reflection_colorTex;
GLuint reflection_rbo;

GLuint refraction_fbo;
GLuint refraction_colorTex;
GLuint refraction_rbo;

//vector<float> plane_normal{-4000.0f , 160000.0f , -4000.0f , -19200000.0f};
//vector<float> plane_normal{0.0f , 1000000.0f , -25000.0f , -122500000.0f};
vector<float> plane_normal{ 0.0f , 4000000.0f , -100000.0f , -500000000.0f };
vector<float> refraction_plane_normal{ 0.0f , -4000000.0f , 100000.0f , 500000000.0f };
float wave_speed = 0.03f;
float move_factor = 0.0f;

vector<float> water_quad
{
	-1000.0f , 100.0f , -1000.0f,
	-1000.0f , 150.0f , 1000.0f ,
	1000.0f , 150.0f , 1000.0f ,
	1000.0f , 100.0f , -1000.0f
};

GLuint DudvTex;

struct Shape
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
};

struct Material
{
	GLuint diffuse_tex;
};

// Vector of meshes and materials
vector<Material> v_material;
vector<Shape> v_shape;

mat4 model_scale_matrix;
mat4 model_rotation_matrix;

mat4 model_matrix;
mat4 view_matrix;
mat4 proj_matrix;
mat4 reflection_view_matrix;


vec3 eye_position = vec3(1.83546, 187.097, 161.775);
vec3 eye_look = vec3(-25.9237, -301.265, -1113.46);  //x前面 y上面 z右邊
vec3 reflected_eye_pos;
vec3 reflected_eye_look;

GLint um4p;
GLint um4mv;
GLint tex;
GLint nor_color;

GLint mode;
GLint time;
GLint mouse_x;
GLint mouse_y;
GLint bar_x;

GLint uni_mag_rad;
GLint uni_mag_center_x;
GLint uni_mag_center_y;

GLint texture_location1;
GLint texture_location2;

GLuint program;			

// For window Frame Buffer Object
GLuint vao2;
GLuint fbo;
GLuint depthrbo;
GLuint program2;
GLuint window_vertex_buffer;
GLuint fboDataTexture;

GLuint noise_tex;

//For Skybox
struct
{	
	GLuint skybox_prog;
	GLuint skybox_vao;
	GLuint tex_envmap;
	GLint inv_vp_matrix;
	GLint eye;
} skybox;

struct
{
	GLuint skybox_prog;
	GLuint skybox_vao;
	GLuint tex_envmap;
	GLint inv_vp_matrix;
	GLint eye;
} graySkyBox;
//Blinn-Phong
struct
{
	//mat4 model_matrix;
	struct
	{
		GLint mv_matrix;
		GLint proj_matrix;
		GLint m_matrix;
		GLint view_matrix;
		GLuint plane_normal;
	}model;
	struct
	{
		GLuint cubemap_tex;
		//for shadow
		GLuint shadow_tex;
		GLint  shadow_matrix;
		GLint flag_shadow;
		GLuint useFog;
	}shadow;
	struct
	{
		GLuint mvp_matrix;
		GLuint reflectTex;
		GLuint refractTex;
		GLuint DudvTex;
		GLuint plane_normal;
		GLuint move_factor;
		GLuint useFog;
	}water;
}uniforms;


//for depth map
GLint light_mvp;
GLuint depthProg;

//for depth map FBO
struct
{
	GLuint fbo;
	GLuint depthMap;
} shadowBuffer;

//Light Space
mat4 light_proj_matrix;
mat4 light_view_matrix;
mat4 light_vp_matrix;

mat4 scale_bias_matrix;
mat4 shadow_sbpv_matrix;


// GUI
TwBar *bar;
int effect_mode = 0;
int useNormal = 0;



static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};


char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
    _TextureData(void) :
        width(0),
        height(0),
        data(0)
    {
    }

    int width;
    int height;
    unsigned char* data;
} TextureData;
// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadImage(const char* const Filepath, bool flip)
{
	TextureData texture;
	int n;
	stbi_set_flip_vertically_on_load(flip);
	stbi_uc *data = stbi_load(Filepath, &texture.width, &texture.height, &n, 4);
	if (data != NULL)
	{
		texture.data = new unsigned char[texture.width * texture.height * 4 * sizeof(unsigned char)];
		memcpy(texture.data, data, texture.width * texture.height * 4 * sizeof(unsigned char));
		stbi_image_free(data);
	}
	return texture;
}

vec3 mirror_point(float a, float b, float c, float d, float x1, float y1, float z1)
{
	float k = (-a * x1 - b * y1 - c * z1 - d) / (float)(a * a + b * b + c * c);
	float x2 = a * k + x1;
	float y2 = b * k + y1;
	float z2 = c * k + z1;
	float x3 = 2 * x2 - x1;
	float y3 = 2 * y2 - y1;
	float z3 = 2 * z2 - z1;
	return vec3(x3, y3, z3);
	//std::cout << std::fixed;
	//cout << " x3 = " << x3;
	//cout << " y3 = " << y3;
	//cout << " z3 = " << z3;
}

void shaderLog(GLuint shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar* errorLog = new GLchar[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		printf("%s\n", errorLog);
		delete[] errorLog;
	}
}

GLuint createShader(char* vertexShader, char* fragmentShader) {
	GLuint shader = glCreateProgram();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

	char** vertexShaderSource_sky = loadShaderSource(vertexShader);
	char** fragmentShaderSource_sky = loadShaderSource(fragmentShader);

	glShaderSource(vs, 1, vertexShaderSource_sky, NULL);
	glShaderSource(fs, 1, fragmentShaderSource_sky, NULL);

	freeShaderSource(vertexShaderSource_sky);
	freeShaderSource(fragmentShaderSource_sky);

	glCompileShader(vs);
	glCompileShader(fs);

	shaderLog(vs);
	shaderLog(fs);

	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	glUseProgram(shader);
	return shader;
}

void TW_CALL SetShowFogCB(const void *value, void *clientData)
{
	useFog = *(const int *)value;
}
void TW_CALL GetShowFogCB(void *value, void *clientData)
{
	*(int *)value = useFog;
}


void TW_CALL SetShowNormalCB(const void *value, void *clientData)
{
	useNormal = *(const int *)value;
}
void TW_CALL GetShowNormalCB(void *value, void *clientData)
{
	*(int *)value = useNormal;
}



void setupGUI()
{
	// Initialize AntTweakBar

#ifdef _MSC_VER
	TwInit(TW_OPENGL, NULL);
#else
	TwInit(TW_OPENGL_CORE, NULL);
#endif
	TwGLUTModifiersFunc(glutGetModifiers);
	bar = TwNewBar("Properties");
	TwDefine("Properties fontsize='3' size='180 50' position='0 0' color='0 0 0' alpha=128 valueswidth=fit");  // http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:twbarparamsyntax
	TwAddVarCB(bar, "Show Fog", TW_TYPE_BOOL32, SetShowFogCB, GetShowFogCB, NULL, "");
	//TwAddVarCB(bar, "Show Normal", TW_TYPE_BOOL32, SetShowNormalCB, GetShowNormalCB, NULL, "");
	
	{
		TwEnumVal effect[8] = { 
			{ MENU_ORI, "Origin" },{ MENU_NORMALCOLOR, "Normal Color" },
			{ MENU_ABSTRACT, "Abstract" },{ MENU_WATERCOLOR, "Water Color" },
			{ MENU_BLOOM, "Bloom Effect" },{ MENU_PIXELIZATION, "Pixelation" },
			{ MENU_SINEWAVE, "Sine Wave Distortion"}, { MENU_MAGNIFIER, "Magnifier"}
		};
		TwType type = TwDefineEnum("Effect", effect, 8);
		TwAddVarRW(bar, "Effect", type, &effect_mode, "");
	}

}

const char* obj_file = "SFMC_main.obj";

void My_LoadScene()
{
	const aiScene *scene = aiImportFile(obj_file, aiProcessPreset_TargetRealtime_MaxQuality);
	if (!scene)
	{
		printf(aiGetErrorString());
		//fprintf(stderr, "Could not load file '%s'\n", obj_file);
		exit(1);
		//return ;
	}

	printf("Load material...\n");
	cout << "scene->mNumMaterials: " << scene->mNumMaterials << endl; // check the number of Materials
	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial *material = scene->mMaterials[i];
		Material mat;
		aiString texturePath;

		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS) 
		{
			cout << "load " << texturePath.C_Str() << endl;
			TextureData tdata = loadImage(texturePath.C_Str(), true);
			// load width, height and data from texturePath.C_Str();+		positions	{ size=14 }	std::vector<float,std::allocator<float> >

			glGenTextures(1, &mat.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, mat.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else  
		{
			cout << "load material fail" << endl;
			// load some default image as default_diffuse_tex
			TextureData tdata = loadImage("KAMEN-stup.JPG",true);
			// load width, height and data from texturePath.C_Str();
			glGenTextures(1, &mat.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, mat.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		// save material…
		v_material.push_back(mat);
	}

	vector<float> positions;
	vector<float> normals;
	vector<float> texcoords;
	vector<unsigned int> indices;

	printf("Load meshes...\n");
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[i];
		Shape shape;
		glGenVertexArrays(1, &shape.vao);
		glBindVertexArray(shape.vao);

		positions.clear();
		normals.clear();
		texcoords.clear();
		indices.clear();

		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{	
			positions.push_back(mesh->mVertices[v].x);
			positions.push_back(mesh->mVertices[v].y);
			positions.push_back(mesh->mVertices[v].z);
			//normals.push_back(mesh->mNormals[v].x);  // 這裡是NULLPTR，檢查檔案以後發現SFMC_main.obj沒有vn
			//normals.push_back(mesh->mNormals[v].y);  // 但是將這裡註解，會跑出assert的error
			//normals.push_back(mesh->mNormals[v].z);
			// texcoords.push_back(mesh->mTextureCoords[0][v].x);
			// texcoords.push_back(mesh->mTextureCoords[0][v].y);

			if (mesh->HasNormals()) {
				normals.push_back(mesh->mNormals[v].x); 
				normals.push_back(mesh->mNormals[v].y); 
				normals.push_back(mesh->mNormals[v].z);
			}

			if (mesh->HasTextureCoords(0)) {
				texcoords.push_back(mesh->mTextureCoords[0][v].x);
				texcoords.push_back(mesh->mTextureCoords[0][v].y);
			}
			else
			{
				texcoords.push_back(0);
				texcoords.push_back(0);
			}
			
			// mesh->mVertices[v][0~2] => position
			// mesh->mNormals[v][0~2] => normal
			// mesh->mTextureCoords[0][v][0~1] => texcoord
		}
		//printf("mesh->mNumFaces:",mesh->mNumFaces);
		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			indices.push_back(mesh->mFaces[f].mIndices[0]);
			indices.push_back(mesh->mFaces[f].mIndices[1]);
			indices.push_back(mesh->mFaces[f].mIndices[2]);
			// mesh->mFaces[f].mIndices[0~2] => index
		}
		// create 3 vbos to hold data
		//printf("position : %lu\n normal : %lu\n texCoord : %lu\n NumVertices : %lu\n indices : %lu\n NumFaces : %lu\n\n" , positions.size(), normals.size(), texcoords.size() , indices.size() , mesh->mNumVertices , mesh->mNumFaces * 3);
		glGenBuffers(1, &shape.vbo_position);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, positions.size()*sizeof(float), &positions[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &shape.vbo_normal);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(float), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
		
		glGenBuffers(1, &shape.vbo_texcoord);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size()*sizeof(float), &texcoords[0], GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		// create 1 ibo to hold data
		glGenBuffers(1, &shape.ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces*3;		
		// save shape…
		v_shape.push_back(shape);
	}
	aiReleaseImport(scene);
}


//Initialize post-process frame buffer object
void My_InitFBO()
{
	program2 = createShader("vertex2.vs.glsl", "fragment2.fs.glsl");

	glUseProgram(program2);

	mode = glGetUniformLocation(program2, "mode");
	time = glGetUniformLocation(program2, "time");
	mouse_x = glGetUniformLocation(program2, "mouse_x");
	mouse_y = glGetUniformLocation(program2, "mouse_y");
	bar_x = glGetUniformLocation(program2, "bar_x");

	uni_mag_rad = glGetUniformLocation(program2, "mag_rad");
	uni_mag_center_x = glGetUniformLocation(program2, "mag_center_x");
	uni_mag_center_y = glGetUniformLocation(program2, "mag_center_y");

	// Set the sampler2D uniform value to 0 (Texture Image Unit 0)
	texture_location1 = glGetUniformLocation(program2, "tex");
	// Set the sampler2D uniform value to 1 (Texture Image Unit 1)
	//texture_location2 = glGetUniformLocation(program2, "noise_tex");

	glGenVertexArrays(1, &vao2);
	glBindVertexArray(vao2);

	glGenBuffers(1, &window_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_positions), window_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	//TextureData noisedata = loadImage("noisy-texture.png");
	/*
	glGenTextures(1, &noise_tex);
	glBindTexture(GL_TEXTURE_2D, noise_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, noisedata.width, noisedata.height, 0, GL_RGB, GL_UNSIGNED_BYTE, noisedata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	*/
	glGenFramebuffers(1, &fbo);
}

// Load the texture for skybox
void LoadSkybox()
{
	vector<TextureData> sky_tex(6);
	sky_tex[0] = loadImage("cubemaps/Skybox_right.png", true);
	sky_tex[1] = loadImage("cubemaps/Skybox_left.png", true);
	sky_tex[2] = loadImage("cubemaps/Skybox_top.png", true);
	sky_tex[3] = loadImage("cubemaps/Skybox_down.png", true);
	sky_tex[4] = loadImage("cubemaps/Skybox_front.png", true);
	sky_tex[5] = loadImage("cubemaps/Skybox_back.png", true);
	glGenTextures(1, &skybox.tex_envmap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.tex_envmap);
	for (int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, sky_tex[i].width, sky_tex[i].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, sky_tex[i].data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	sky_tex.clear();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void LoadGraySkybox()
{
	vector<TextureData> sky_tex(6);
	sky_tex[0] = loadImage("Skybox_BlueNebular_Textures/BlueNebular_right.jpg", true);
	sky_tex[1] = loadImage("Skybox_BlueNebular_Textures/BlueNebular_left.jpg", true);
	sky_tex[2] = loadImage("Skybox_BlueNebular_Textures/BlueNebular_top.jpg", true);
	sky_tex[3] = loadImage("Skybox_BlueNebular_Textures/BlueNebular_bottom.jpg", true);
	sky_tex[4] = loadImage("Skybox_BlueNebular_Textures/BlueNebular_front.jpg", true);
	sky_tex[5] = loadImage("Skybox_BlueNebular_Textures/BlueNebular_back.jpg", true);
	glGenTextures(1, &graySkyBox.tex_envmap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, graySkyBox.tex_envmap);
	for (int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, sky_tex[i].width, sky_tex[i].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, sky_tex[i].data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	sky_tex.clear();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}


// Initialize skybox
void Init_Skybox()
{
	skybox.skybox_prog = createShader("vertex_sky.vs.glsl", "fragment_sky.fs.glsl");

	glUseProgram(skybox.skybox_prog);

	skybox.inv_vp_matrix = glGetUniformLocation(skybox.skybox_prog, "inv_vp_matrix");
	skybox.eye = glGetUniformLocation(skybox.skybox_prog, "eye");
	//tex_cubemap = glGetUniformLocation(skybox_prog, "tex_cubemap");

	LoadSkybox();
	LoadGraySkybox();

	glGenVertexArrays(1, &skybox.skybox_vao);
	glGenVertexArrays(1, &graySkyBox.skybox_vao);
}

void Init_depth() {
	// ----- Begin Initialize Depth Shader Program -----
	depthProg = createShader("depth.vs.glsl", "depth.fs.glsl");
	light_mvp = glGetUniformLocation(depthProg, "mvp");
	// ----- End Initialize Depth Shader Program -----
}

void Init_ShadowFBO() {
	// ----- Begin Initialize Shadow Framebuffer Object -----
	glGenFramebuffers(1, &shadowBuffer.fbo);

	glGenTextures(1, &shadowBuffer.depthMap);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowBuffer.depthMap, 0);
	// ----- End Initialize Shadow Framebuffer Object -----
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void My_InitWater()
{
	water_program = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	char** vs_source = loadShaderSource("water_vs.glsl");
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** fs_source = loadShaderSource("water_fs.glsl");
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);
	glAttachShader(water_program, vs);
	glAttachShader(water_program, fs);
	glLinkProgram(water_program);
	uniforms.water.mvp_matrix = glGetUniformLocation(water_program, "mvp");
	uniforms.water.reflectTex = glGetUniformLocation(water_program, "reflectionTexture");
	uniforms.water.refractTex = glGetUniformLocation(water_program, "refractionTexture");
	uniforms.water.DudvTex = glGetUniformLocation(water_program, "dudvTex");
	uniforms.water.plane_normal = glGetUniformLocation(water_program, "plane_normal");
	uniforms.water.move_factor = glGetUniformLocation(water_program, "move_factor");

	uniforms.water.useFog = glGetUniformLocation(water_program, "useFog");

	glGenVertexArrays(1, &water_quad_vao);
	glBindVertexArray(water_quad_vao);
	glGenBuffers(1, &water_quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, water_quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, water_quad.size() * sizeof(float), water_quad.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void My_InitReflection()
{
	glGenFramebuffers(1, &reflection_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reflection_fbo);

	glGenRenderbuffers(1, &reflection_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, reflection_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, window_width, window_height);

	glGenTextures(1, &reflection_colorTex);
	glBindTexture(GL_TEXTURE_2D, reflection_colorTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reflection_fbo);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflection_rbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflection_colorTex, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}


void My_InitRefraction()
{
	glGenFramebuffers(1, &refraction_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, refraction_fbo);

	glGenRenderbuffers(1, &refraction_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, refraction_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 600, 600);

	glGenTextures(1, &refraction_colorTex);
	glBindTexture(GL_TEXTURE_2D, refraction_colorTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 600, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, refraction_fbo);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, refraction_rbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refraction_colorTex, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void My_LoadDudv()
{
	glGenTextures(1, &DudvTex);
	glBindTexture(GL_TEXTURE_2D, DudvTex);

	TextureData data = loadImage("dudv4.jpg", false);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data.width, data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void My_Init()
{
    glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	My_LoadDudv();
	My_InitWater();
	My_InitReflection();
	My_InitRefraction();
	// Create Shader Program
	program = createShader("vertex.vs.glsl", "fragment.fs.glsl");


	uniforms.model.proj_matrix = glGetUniformLocation(program, "proj_matrix");
	uniforms.model.mv_matrix = glGetUniformLocation(program, "mv_matrix");

	uniforms.model.m_matrix = glGetUniformLocation(program, "m_matrix");
	uniforms.model.view_matrix = glGetUniformLocation(program, "view_matrix");

	uniforms.model.plane_normal = glGetUniformLocation(program, "plane_normal");


	tex = glGetUniformLocation(program, "tex");
	uniforms.shadow.shadow_tex = glGetUniformLocation(program, "shadow_tex");
	uniforms.shadow.shadow_matrix = glGetUniformLocation(program, "shadow_matrix");
	uniforms.shadow.useFog = glGetUniformLocation(program, "useFog");

	nor_color = glGetUniformLocation(program, "normal_flag");

	glUseProgram(program);

	My_LoadScene();
	printf("Load model finish\n");

	Init_Skybox();
	printf("Load skybox finish\n");


	Init_depth();

	Init_ShadowFBO();

	My_InitFBO();
}


void skybox_display()
{
	mat4 inv_vp_matrix = inverse(proj_matrix * view_matrix);

	// Draw skybox
	
	glUseProgram(skybox.skybox_prog);
	
	if (useFog == 0) {
		glBindVertexArray(skybox.skybox_vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.tex_envmap);
	}
	else {
		glBindVertexArray(graySkyBox.skybox_vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, graySkyBox.tex_envmap);
	}

	glUniformMatrix4fv(skybox.inv_vp_matrix, 1, GL_FALSE, &inv_vp_matrix[0][0]);
	glUniform3fv(skybox.eye, 1, &eye_look[0]);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);
}


void model_display()
{
	//Scene Rendering
	glUseProgram(program);

	glEnable(GL_CLIP_PLANE0);
	model_rotation_matrix = rotate(mat4(), (float)deg2rad(270), vec3(1.0, 0.0, 0.0));
	//model_matrix = model_rotation_matrix * model_matrix;
	glUniformMatrix4fv(uniforms.model.mv_matrix, 1, GL_FALSE, value_ptr(view_matrix * model_rotation_matrix * model_matrix));
	glUniformMatrix4fv(uniforms.model.proj_matrix, 1, GL_FALSE, value_ptr(proj_matrix));

	glUniformMatrix4fv(uniforms.model.m_matrix, 1, GL_FALSE, value_ptr(model_rotation_matrix * model_matrix));
	glUniformMatrix4fv(uniforms.model.view_matrix, 1, GL_FALSE, value_ptr(lookAt(vec3(30.0f, 150.0f, -10.0f), vec3(1000.0f, 320.0f, -280.0f), vec3(0.0f, 1.0f, 0.0f))));
	
	glUniform4fv(uniforms.model.plane_normal, 1, plane_normal.data());

	glUniform1i(uniforms.shadow.useFog, useFog);

	mat4 shadow_matrix = shadow_sbpv_matrix * model_rotation_matrix * model_matrix;
	glUniformMatrix4fv(uniforms.shadow.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));

	for (int i = 0; i < v_shape.size(); i++)
	{
		glBindVertexArray(v_shape[i].vao);
		int materialID = v_shape[i].materialID;

		//pass texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, v_material[materialID].diffuse_tex);
		glUniform1i(tex, 0);

		//pass the depth map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		glUniform1i(uniforms.shadow.shadow_tex, 1);

		glDrawElements(GL_TRIANGLES, v_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	glDisable(GL_CLIP_PLANE0);

	glUseProgram(0);
}

void Shadow_display()
{
	// ----- Begin Shadow Map Pass -----
	glUseProgram(depthProg);
	glEnable(GL_DEPTH_TEST);

	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	glClear(GL_DEPTH_BUFFER_BIT);

	//glViewport(0, 0, 600, 600);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);

	glUniformMatrix4fv(light_mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * model_rotation_matrix * model_matrix));
	for (int i = 0; i < v_shape.size(); i++)
	{
		glBindVertexArray(v_shape[i].vao);
		glDrawElements(GL_TRIANGLES, v_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	// ----- End Shadow Map Pass -----
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void water_display()
{
	move_factor = glutGet(GLUT_ELAPSED_TIME) / 1500000.0f;
	move_factor = fmod(move_factor, 1);
	glUseProgram(water_program);
	glUniform4fv(uniforms.water.plane_normal, 1, plane_normal.data());
	glUniformMatrix4fv(uniforms.water.mvp_matrix, 1, GL_FALSE, value_ptr(proj_matrix * view_matrix));
	glUniform1f(uniforms.water.move_factor, move_factor);

	glUniform1i(uniforms.water.useFog, useFog);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, reflection_colorTex);
	glUniform1i(uniforms.water.reflectTex, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, DudvTex);
	glUniform1i(uniforms.water.DudvTex, 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, refraction_colorTex);
	glUniform1i(uniforms.water.refractTex, 4);

	glBindVertexArray(water_quad_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);
}

void reflection_render()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reflection_fbo);
	glClearColor(99.0f / 255.0f, 194.0f / 255.0f, 250.0f / 255.0f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Skybox rendering
	skybox_display();

	//Scene Rendering
	glUseProgram(program);
	glEnable(GL_CLIP_PLANE0);
	//model_rotation_matrix = rotate(mat4(1.0f) , (float)deg2rad(270) , vec3(1.0 , 0.0 , 0.0));
	//model_matrix = model_rotation_matrix * model_matrix;
	glUniformMatrix4fv(uniforms.model.mv_matrix, 1, GL_FALSE, value_ptr(reflection_view_matrix * model_rotation_matrix * model_matrix));
	glUniformMatrix4fv(uniforms.model.proj_matrix, 1, GL_FALSE, value_ptr(proj_matrix));

	glUniformMatrix4fv(uniforms.model.m_matrix, 1, GL_FALSE, value_ptr(model_rotation_matrix * model_matrix));
	glUniformMatrix4fv(uniforms.model.view_matrix, 1, GL_FALSE, value_ptr(reflection_view_matrix));

	glUniform4fv(uniforms.model.plane_normal, 1, plane_normal.data());

	mat4 shadow_matrix = shadow_sbpv_matrix * model_rotation_matrix * model_matrix;
	glUniformMatrix4fv(uniforms.shadow.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));

	for (int i = 0; i < v_shape.size(); i++)
	{
		glBindVertexArray(v_shape[i].vao);
		int materialID = v_shape[i].materialID;

		//pass texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, v_material[materialID].diffuse_tex);
		glUniform1i(tex, 0);

		//pass the depth map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		glUniform1i(uniforms.shadow.shadow_tex, 1);

		glDrawElements(GL_TRIANGLES, v_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	glDisable(GL_CLIP_PLANE0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void refraction_render()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, refraction_fbo);
	glClearColor(99.0f / 255.0f, 194.0f / 255.0f, 250.0f / 255.0f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Skybox rendering
	skybox_display();

	//Scene Rendering
	glUseProgram(program);
	glEnable(GL_CLIP_PLANE0);
	//model_rotation_matrix = rotate(mat4(1.0f) , (float)deg2rad(270) , vec3(1.0 , 0.0 , 0.0));
	//model_matrix = model_rotation_matrix * model_matrix;
	glUniformMatrix4fv(uniforms.model.mv_matrix, 1, GL_FALSE, value_ptr(view_matrix * model_rotation_matrix * model_matrix));
	glUniformMatrix4fv(uniforms.model.proj_matrix, 1, GL_FALSE, value_ptr(proj_matrix));

	glUniformMatrix4fv(uniforms.model.m_matrix, 1, GL_FALSE, value_ptr(model_rotation_matrix * model_matrix));
	glUniformMatrix4fv(uniforms.model.view_matrix, 1, GL_FALSE, value_ptr(view_matrix));

	glUniform4fv(uniforms.model.plane_normal, 1, refraction_plane_normal.data());

	mat4 shadow_matrix = shadow_sbpv_matrix * model_rotation_matrix * model_matrix;
	glUniformMatrix4fv(uniforms.shadow.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));

	for (int i = 0; i < v_shape.size(); i++)
	{
		glBindVertexArray(v_shape[i].vao);
		int materialID = v_shape[i].materialID;

		//pass texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, v_material[materialID].diffuse_tex);
		glUniform1i(tex, 0);

		//pass the depth map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
		glUniform1i(uniforms.shadow.shadow_tex, 1);

		glDrawElements(GL_TRIANGLES, v_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	glDisable(GL_CLIP_PLANE0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}



void My_Display()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	
	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClearColor(0.5294f, 0.8078f, 0.94157f, 0.75f);

	// lookAt(camera_position, camera_viewing_vector, up_vector)
	view_matrix = lookAt(eye_position, eye_look, vec3(0.0f, 1.0f, 0.0f));
	reflected_eye_pos = mirror_point(plane_normal[0], plane_normal[1], plane_normal[2], plane_normal[3], eye_position.x, eye_position.y, eye_position.z);
	reflected_eye_look = mirror_point(plane_normal[0], plane_normal[1], plane_normal[2], plane_normal[3], eye_look.x, eye_look.y, eye_look.z);
	reflection_view_matrix = lookAt(reflected_eye_pos, reflected_eye_look, vec3(0.0f, 1.0f, 0.0f));
	//CAMERA position,看向的點,頭頂的方向
	
	//Skybox Rendering
	skybox_display();

	//Light Space
	const float shadow_range = 1000.0f;
	//正交投影
	light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 400.0f);
	light_view_matrix = lookAt(vec3(-3.37016f, 200.0f, -91.795f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	light_vp_matrix = light_proj_matrix * light_view_matrix;

	//Depth Map
	Shadow_display();

	scale_bias_matrix = translate(mat4(), vec3(0.5f, 0.5f, 0.5f)) * scale(mat4(), vec3(0.5f, 0.5f, 0.5f));
	shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;

	view_matrix = lookAt(eye_position, eye_look, vec3(0.0f, 1.0f, 0.0f));
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glViewport(0, 0, window_width, window_height);
	reflection_render();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	refraction_render();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	model_display();
	water_display();

	//FBO
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.5294f, 0.8078f, 0.94157f, 0.75f);

	glBindVertexArray(vao2);
	glUseProgram(program2);
	glUniform1f(time, timer_cnt);
	glUniform1i(texture_location1, 0);
	//glUniform1i(texture_location2, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboDataTexture); //將FBO產生的貼圖傳到shader
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, noise_tex);
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	

	TwDraw();
	glutSwapBuffers();
}



void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	current_bar = current_bar / window_width *int(width);
	mag_x = mag_x / window_width * int(width);
	mag_y = mag_y / window_height * int(height);
	glUniform1i(uni_mag_center_x, mag_x);
	glUniform1i(uni_mag_center_y, int(height) - mag_y);
	glUniform1i(uni_mag_rad, mag_rad);
	//cout << current_bar;
	glUniform1i(bar_x, int(current_bar));
	window_width = int(width);
	window_height = int(height);

	float viewportAspect = (float)width / (float)height;
	
	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	proj_matrix = perspective(radians(90.0f), viewportAspect, 0.1f, 3000.0f);

	glDeleteRenderbuffers(1, &depthrbo);
	glDeleteTextures(1, &fboDataTexture);
	glGenRenderbuffers(1, &depthrbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

	glGenTextures(1, &fboDataTexture);
	glBindTexture(GL_TEXTURE_2D, fboDataTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboDataTexture, 0);
	//glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, noise_tex, 0);
	//重新附加texture

	// reset GUI windows
	TwWindowSize(width, height);
}

void CheckEffectMode() {

	switch (effect_mode)
	{
	case MENU_ORI:
		ori_scene = true;
		magnifier = false;
		glUseProgram(program);
		glUniform1i(nor_color, 0);
		glUseProgram(program2);
		glUniform1i(mode, 0);
		break;
	case MENU_NORMALCOLOR:
		magnifier = false;
		useFog = 0;
		glUseProgram(program);
		glUniform1i(nor_color, 1);
		glUseProgram(program2);
		glUniform1i(mode, 0);
		break;
	case MENU_ABSTRACT:
		magnifier = false;
		ori_scene = false;
		glUseProgram(program2);
		glUniform1i(mode, 2);
		break;
	case MENU_WATERCOLOR:
		magnifier = false;
		ori_scene = false;
		glUseProgram(program2);
		glUniform1i(mode, 3);
		break;
	case MENU_BLOOM:
		magnifier = false;
		ori_scene = false;
		glUseProgram(program2);
		glUniform1i(mode, 4);
		break;
	case MENU_PIXELIZATION:
		magnifier = false;
		ori_scene = false;
		glUseProgram(program2);
		glUniform1i(mode, 5);
		break;
	case MENU_SINEWAVE:
		magnifier = false;
		ori_scene = false;
		glUseProgram(program2);
		glUniform1i(mode, 6);
		break;
	case MENU_MAGNIFIER:
		magnifier = true;
		glUseProgram(program2);
		glUniform1i(mode, 7);
		break;
	case MENU_FOG:
		useFog = 1;
		//glUniform1i(island.useFog, 1);
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}

}

void My_Timer(int val)
{
	glutPostRedisplay();
	timer_cnt += 0.03;
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
	
	CheckEffectMode();
}


void My_Mouse(int button, int state, int x, int y)
{
	current_x = x;
	current_y = y;
	move_bar = false;
	move_mag = false;
	resize_mag = false;
	if(state == GLUT_DOWN)
	{
		//printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
		
		if (!ori_scene && current_x > current_bar - 6 && current_x < current_bar + 6 && !magnifier) move_bar = true;
		else move_bar = false;
		//resize
		if (magnifier && length(vec2(current_x, current_y) - vec2(mag_x, mag_y)) < mag_rad + 4 &&
			magnifier && length(vec2(current_x, current_y) - vec2(mag_x, mag_y)) > mag_rad - 4) {
			resize_mag = true;
		}
		else resize_mag = false;
		//move
		if (magnifier && length(vec2(current_x, current_y) - vec2(mag_x, mag_y)) < mag_rad - 4) {
			move_mag = true;
			cout << "mag!!\n";
		}
		else move_mag = false;
	}
	if(state == GLUT_UP)
	{
		//printf("current_bar(%d)\n", int(current_bar));
		//printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
	if (button == GLUT_LEFT_BUTTON)
	{
		glUniform1i(mouse_x, current_x);
		glUniform1i(mouse_y, current_y);
	}

	// GUI
	if (!TwEventMouseButtonGLUT(button, state, x, y))
	{
		//m_camera.mouseEvents(button, state, x, y);
	}

}

void Mouse_Moving(int x, int y)
{
	if (!TwEventMouseMotionGLUT(x, y))
	{
		int diff_x = x - current_x;
		int diff_y = y - current_y;
		current_x = x;
		current_y = y;
		glUniform1i(mouse_x, current_x);
		glUniform1i(mouse_y, current_y);

		if (move_bar) {
			if (x > window_width) current_bar = window_width - 10;
			else if (x < 10) current_bar = 10;
			else current_bar = x;
			glUniform1i(bar_x, int(current_bar));
		}
		else if (move_mag) {
			if (x > window_width) mag_x = window_width - 20;
			else if (x < 20) mag_x = 20;
			else mag_x = x;

			if (y > window_height) mag_y = window_height - 20;
			else if (y < 20) mag_y = 20;
			else mag_y = y;
			//cout << "???" << mag_x << mag_y;
			glUniform1i(uni_mag_center_x, mag_x);
			glUniform1i(uni_mag_center_y, int(window_height - mag_y));
		}
		else if (resize_mag)
		{
			mag_rad = int(length(vec2(x, y) - vec2(mag_x, mag_y)));
			if (mag_rad < 75) mag_rad = 75;
			else if (mag_rad > length(vec2(window_width / 2, window_height / 2)) - 100) mag_rad = length(vec2(window_width / 2, window_height / 2)) - 100;
			glUniform1i(uni_mag_rad, mag_rad);
		}
		else {
			GLfloat degree_x = -diff_x / 450.0f;
			GLfloat degree_y = -diff_y / 450.0f;
			//沿著Z軸
			mat3 rot_lookX = mat3(cos(degree_y), -sin(degree_y), 0.0,
				sin(degree_y), cos(degree_y), 0.0,
				0.0, 0.0, 1.0);
			//沿著Y軸
			mat3 rot_lookY = mat3(cos(degree_x), 0.0, sin(degree_x),
				0.0, 1.0, 0.0,
				-sin(degree_x), 0.0, cos(degree_x));

			mat4 rotation_X = rotate(mat4(), -degree_y, normalize(cross(eye_look - eye_position, vec3(0.0f, 1.0f, 0.0f))));
			mat4 rotation_Y = rotate(mat4(), -degree_x, vec3(0.0f, 1.0f, 0.0f));

			//printf( "%f \n", degrees(acos(dot(eye_look - eye_position, vec3(0.0f, 1.0f, 0.0f)) / 1.0f)) );

			eye_look -= eye_position;
			eye_look = vec3(rotation_Y * rotation_X * vec4(eye_look, 1.0));
			eye_look += eye_position;
			if (degrees(acos(dot(eye_look - eye_position, vec3(0.0f, 1.0f, 0.0f)) / 1.0f)) < 10.0f)
			{
				eye_look -= eye_position;
				eye_look = vec3(rotation_Y * rotate(mat4(), degree_y, normalize(cross(eye_look - eye_position, vec3(0.0f, 1.0f, 0.0f)))) * vec4(eye_look, 1.0));
				eye_look += eye_position;
			}
			else if (degrees(acos(dot(eye_look - eye_position, vec3(0.0f, 1.0f, 0.0f)) / 1.0f)) > 170.0f)
			{
				eye_look -= eye_position;
				eye_look = vec3(rotation_Y * rotate(mat4(), degree_y, normalize(cross(eye_look - eye_position, vec3(0.0f, 1.0f, 0.0f)))) * vec4(eye_look, 1.0));
				eye_look += eye_position;
			}
		}
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	
	vec3 move_fd = 1.5f * normalize(eye_look - eye_position);
	vec3 move_rd = 1.5f * normalize(cross(eye_look-eye_position, vec3(0.0f, 1.0f, 0.0f)));
	
	//printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	if (key == 'w')
	{
		eye_position += move_fd;
		eye_look += move_fd;
	}
	if (key == 's')
	{
		eye_position -= move_fd;
		eye_look -= move_fd;
	}
	if (key == 'd')
	{
		eye_position += move_rd;
		eye_look += move_rd;
	}
	if (key == 'a')
	{
		eye_position -= move_rd;
		eye_look -= move_rd;
	}
	if (key == 'z')
	{
		eye_position += vec3(0.0f, 1.5f, 0.0f);
		eye_look += vec3(0.0f, 1.5f, 0.0f);
	}
	if (key == 'x')
	{
		eye_position += vec3(0.0f, -1.5f, 0.0f);
		eye_look += vec3(0.0f, -1.5f, 0.0f);
	}
	/*
	cout << "Now, eyes position is at(" << eye_position.x << "," << eye_position.y << "," << eye_position.z << ")\n"
		<< "and look at(" << eye_look.x << "," << eye_look.y << "," << eye_look.z << ")\n";
	*/
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}


bool b_nor = FALSE;


int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 50);
	glutInitWindowSize(window_width, window_height);
	/*glutInitContextVersion(4, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);*/
	glutCreateWindow("Team #11 FP"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif

    glPrintContextInfo();
	My_Init();
	setupGUI();

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(Mouse_Moving);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();
	TwTerminate();

	return 0;
}
