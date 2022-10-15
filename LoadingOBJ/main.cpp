#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <map>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint trfloc, prloc, lightloc, camloc, timerloc;
float a;

glm::mat4 trf;
GLuint vbo, ibo;

struct VertexData
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texcoord;
};

std::vector<VertexData> verts;
std::vector<unsigned int> indices;

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void timer(int)
{
	//trf = glm::translate(trf, glm::vec3(0.001, 0, 0));
	//trf = glm::scale(trf, glm::vec3(0.99, 1, 1));
	trf = glm::rotate(trf, 0.05f, glm::vec3(0, 1, 1));
	a += 30;

	auto cpos = glm::vec3(4);
	glm::mat4 proj = glm::perspective(45.f, 1.0f, 0.1f, 100.f);
	glm::mat4 lookAt = glm::lookAt(cpos, glm::vec3(0), glm::vec3(0, 0, 1));

	proj = proj * lookAt;
	glUniformMatrix4fv(trfloc, 1, GL_FALSE, glm::value_ptr(trf));
	glUniformMatrix4fv(prloc, 1, GL_FALSE, glm::value_ptr(proj));
	auto lpos = glm::vec3(1, 1, 2);
	glUniform3fv(lightloc, 1, glm::value_ptr(lpos));
	glUniform3fv(camloc, 1, glm::value_ptr(cpos));
	glUniform1f(timerloc, a);
	glutPostRedisplay();
	glutTimerFunc(30, timer, 0);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(512, 512);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow(argv[0]);

	glewInit();

	GLuint shp = glCreateProgram();

	GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vshCode =
		"#version 330 \n"
		"layout (location = 0) in vec3 pos;\n"
		"layout (location = 1) in vec3 normal;\n"
		"layout (location = 2) in vec2 texcrd;\n"
		"uniform mat4 transf;"
		"uniform mat4 proj;"
		"out vec3 out_pos;"
		"out vec3 out_normal;"
		"out vec2 out_texcrd;"
		"uniform float timer;"
		"uniform vec3 light_pos;"
		"void main(){"
		"out_texcrd = texcrd;"
		"out_pos = (transf*vec4(pos, 1)).xyz;"
		"out_normal = (transf*vec4(normal, 1)).xyz;"
		"gl_Position = proj*transf*vec4(pos, 1);}";
	GLint vshlen = strlen(vshCode);
	glShaderSource(vsh, 1, &vshCode, &vshlen);
	glAttachShader(shp, vsh);
	glCompileShader(vsh);

	GLint isCompiled = 0;
	glGetShaderiv(vsh, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(vsh, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(vsh, maxLength, &maxLength, &errorLog[0]);

		std::cout << "VS" << std::string(errorLog.data());
		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(vsh); // Don't leak the shader.
		return 0;
	}
	GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fshCode =
		"#version 330 \n"
		"in vec3 out_pos;"
		"in vec3 out_normal;"
		"in vec2 out_texcrd;"
		"uniform vec3 light_pos;"
		"uniform vec3 cam_pos;"
		"uniform sampler2D skytex;"
		"uniform float timer;"
		"out vec4 FragColor;"
		"void main(){"
		"float diffuse = max(dot(normalize(out_normal), normalize(light_pos - out_pos)), 0.2);"
		"vec3 ref = -reflect(normalize(out_normal), normalize(light_pos - out_pos));"
		"float spec = pow(max(dot(ref, normalize(cam_pos - out_pos)), 0), 45);"
		"vec2 texcrd = out_texcrd;"
		"vec3 final_color = diffuse*texture2D(skytex, texcrd ).xyz + spec*vec3(1,1,1);"
		//"float r = 0.2*final_color.r + 0.5*final_color.g + 0.3*final_color.b;"
		//"final_color = vec3(timer/3000.0,0,0);"
		"FragColor = vec4(final_color, 1);}";
	GLint fshlen = strlen(fshCode);
	glShaderSource(fsh, 1, &fshCode, &fshlen);
	glAttachShader(shp, fsh);
	glCompileShader(fsh);

	isCompiled = 0;
	glGetShaderiv(fsh, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(fsh, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(fsh, maxLength, &maxLength, &errorLog[0]);


		std::cout << "FS" << std::string(errorLog.data());
		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(fsh); // Don't leak the shader.
		return 0;
	}
	glValidateProgram(shp);
	glLinkProgram(shp);
	glUseProgram(shp);

	glEnable(GL_DEPTH_TEST);

	//auto pi = acos(-1);
	//for (int i = 0; i < 10; ++i)
	//{
	//	glm::vec3 C = glm::vec3(0, 0, 1);
	//	glm::vec3 B = glm::vec3(cos(i * 36.0 * pi / 180.0), sin(i * 36.0 * pi / 180.0), 0);
	//	glm::vec3 A = glm::vec3(cos((i + 1) * 36.0 * pi / 180.0), sin((i + 1) * 36.0 * pi / 180.0), 0);

	//	auto a = A - C;
	//	auto b = B - C;
	//	auto n = glm::normalize(glm::cross(b, a));

	//	verts.push_back({ C, n, glm::vec2(0.5, 1) });
	//	verts.push_back({ B, n, glm::vec2(0,0) });
	//	verts.push_back({ A, n, glm::vec2(1,0) });
	//}

	//glm::vec3 n;
	//for (int i = 0; i < verts.size()/3; ++i)
	//{
	//	verts[3 * i].normal = glm::vec3(0, 0, 1);
	//	if (i != 0 && i != verts.size() / 3 - 1)
	//	{
	//		n = glm::normalize(0.5f * (verts[3 * i + 1].normal + verts[3 * i - 1].normal));
	//		verts[3 * i + 1].normal = n;
	//		verts[3 * i - 1].normal = n;
	//	}
	//}

	//n = glm::normalize(0.5f * (verts[1].normal + verts[verts.size() - 1].normal));
	//verts[1].normal = n;
	//verts[verts.size() - 1].normal = n;

	std::fstream myfile("obj/MI SMART TV.obj");
	std::string flag;
	myfile >> flag;
	std::vector<glm::vec3> poss;
	std::vector<glm::vec3> norms;
	std::vector<glm::vec2> tcs;
	struct index_comb
	{
		unsigned int posi;
		unsigned int normi;
		unsigned int tci;
	};

	auto cmp = [](const index_comb const& lhs, const index_comb const& rhs)->bool
	{
		if (lhs.posi < rhs.posi) return true;
		if (lhs.posi > rhs.posi) return false;
		if (lhs.normi < rhs.normi) return true;
		if (lhs.normi > rhs.normi) return false;
		if (lhs.tci < rhs.tci) return true;
		else return false;
	};

	std::map < index_comb, unsigned int, decltype(cmp)> index_map(cmp);
	unsigned int index = 0;
	do {
		if (flag == "v")
		{
			float x, y, z;
			myfile >> x >> y >> z;
			//verts.push_back({glm::vec3(x,y,z), glm::vec3(1,0,0), glm::vec2(1,0)});
			poss.push_back({ glm::vec3(x,y,z) });
		}
		else
			if (flag == "vn")
			{
				float x, y, z;
				myfile >> x >> y >> z;
				//verts.push_back({glm::vec3(x,y,z), glm::vec3(1,0,0), glm::vec2(1,0)});
				norms.push_back({ glm::vec3(x,y,z) });
			}
			else
				if (flag == "vt")
				{
					float x, y;
					myfile >> x >> y;
					//verts.push_back({glm::vec3(x,y,z), glm::vec3(1,0,0), glm::vec2(1,0)});
					tcs.push_back({ glm::vec2(x,y) });
				}
				else
					if (flag == "f")
					{
						for (int i = 0; i < 3; ++i)
						{
							unsigned int ind1, ind2, ind3;
							std::string str;
							myfile >> str;
							int slash1 = str.find('/');
							int slash2 = str.find('/', slash1 + 1);
							int slash3 = str.find('/', slash2 + 1);
							ind1 = atoi(str.substr(0, slash1).c_str());
							ind2 = atoi(str.substr(slash1 + 1, slash2).c_str());
							ind3 = atoi(str.substr(slash2 + 1, slash3).c_str());
							if (index_map.find({ ind1 - 1, ind3 - 1, ind2 - 1 }) == index_map.end())
							{
								index_map[{ ind1 - 1, ind3 - 1, ind2 - 1 }] = index;
								index++;
								verts.push_back({ poss[ind1 - 1], norms[ind3 - 1], tcs[ind2 - 1] });
							}
							indices.push_back(index_map[{ ind1 - 1, ind3 - 1, ind2 - 1 }]);
						}
					}
		myfile >> flag;
	} while (!myfile.fail());

	//for ( int i = 0 ; i < indices.size()/3; ++i)
	//{
	//	auto i1 = indices[3 * i];
	//	auto i2 = indices[3 * i+1];
	//	auto i3 = indices[3 * i+2];
	//	glm::vec3 C = verts[i1].pos;
	//	glm::vec3 B = verts[i2].pos;
	//	glm::vec3 A = verts[i3].pos;

	//	auto a = A - C;
	//	auto b = B - C;
	//	auto n = glm::normalize(glm::cross(b, a));

	//	verts[i1].normal = n;
	//	verts[i2].normal = n;
	//	verts[i3].normal = n;
	//	verts[i1].texcoord = glm::vec2(0.5, 1);
	//	verts[i2].texcoord = glm::vec2(0,0);
	//	verts[i3].texcoord = glm::vec2(1,0);
	//}

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VertexData), verts.data(), GL_STATIC_DRAW);

	GLuint posloc = 0;
	glVertexAttribPointer(posloc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
	glEnableVertexAttribArray(posloc);

	GLuint normloc = 1;
	glVertexAttribPointer(normloc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)sizeof(glm::vec3));
	glEnableVertexAttribArray(normloc);

	GLuint tcloc = 2;
	glVertexAttribPointer(tcloc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(sizeof(glm::vec3) + sizeof(glm::vec3)));
	glEnableVertexAttribArray(tcloc);

	trfloc = glGetUniformLocation(shp, "transf");
	prloc = glGetUniformLocation(shp, "proj");
	lightloc = glGetUniformLocation(shp, "light_pos");
	camloc = glGetUniformLocation(shp, "cam_pos");
	timerloc = glGetUniformLocation(shp, "timer");

	trf = glm::mat4(1);

	int width, height, channels;
	unsigned char* img = stbi_load("obj/textures/2.jpg", &width, &height, &channels, 0);

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	GLuint texloc = glGetUniformLocation(shp, "skytex");
	glUniform1i(texloc, 0);

	//glFrontFace(GL_CW);
	//glCullFace(GL_BACK);
	//glEnable(GL_CULL_FACE);

	glClearColor(1, 1, 1, 1);
	glutDisplayFunc(display);
	glutTimerFunc(0, timer, 0);

	glutMainLoop();
}