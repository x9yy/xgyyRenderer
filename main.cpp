#include <iostream>
#include <memory>
#include <numeric>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include "shader.h"


TGAColor WHITE(255, 255, 255, 255);
TGAColor RED(255, 0, 0, 255);
TGAColor GREEN(0, 255, 0, 255);
TGAColor BLUE(0, 0, 255, 255);

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;

vec3 EYE = { 0,0,3 };
vec3 CENTER = { 0,0,0 };

void render_shadow(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return ;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords, normals;
	std::array<vec4, 3> screen_coords;

	Light light;
	light.position = vec3(3, 3, 0);
	light.direction = vec3(-2, 2, 2);
	light.ambient = 0.1 * vec3(255, 255, 255);
	light.diffuse = vec3(255, 255, 255);
	light.specular = vec3(255, 255, 255);

	Material material;
	material.ambient = material.diffuse = material.specular = vec3(255, 231, 111);
	material.shininess = 32;

	float* zbuffer = new float[HEIGHT * WIDTH], * shadow_buffer = new float[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();
	for (int i = 0; i < HEIGHT * WIDTH; i++) shadow_buffer[i] = -std::numeric_limits<float>::max();

	DeepthShader deepth_shader;
	ShadowShader shadow_shader;
	for (int n = 1; n < argc; n++) {
		//Model model(R"(D:\code\MyTinyRenderer\obj\spot_triangulated_good.obj)");
		Model model(argv[n]);
		deepth_shader.projection = mat<4, 4>::identity();
		deepth_shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		deepth_shader.lookat = get_lookat(light.position, CENTER, vec3(0, 1, 0));
		deepth_shader.model = mat<4, 4>::identity();

		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				normals[ivert] = model.normal(iface, ivert).normalize();
			}
			screen_coords = deepth_shader.vertex(world_coords);
			triangle(screen_coords, deepth_shader, shadow_buffer, image);
		}
	}

	for (int n = 1; n < argc; n++) {
		Model model(argv[n]);
		shadow_shader.projection = get_projection(EYE, CENTER);
		shadow_shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		shadow_shader.lookat = get_lookat(EYE, CENTER, vec3(0, 1, 0));
		shadow_shader.model = mat<4, 4>::identity();
		shadow_shader.deepth_matrix = deepth_shader.viewport * deepth_shader.projection * deepth_shader.lookat * deepth_shader.model;
		shadow_shader.light = light;
		shadow_shader.material = material;
		shadow_shader.eye = EYE;
		shadow_shader.shadow_buffer = shadow_buffer;
		shadow_shader.dim = vec2(WIDTH, HEIGHT);

		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				normals[ivert] = model.normal(iface, ivert).normalize();
			}
			shadow_shader.normals = normals;
			screen_coords = shadow_shader.vertex(world_coords);
			triangle(screen_coords, shadow_shader, zbuffer, image);
		}

	}

	//image.flip_vertically();
	image.write_tga_file("output.tga");
}

void render_texture(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords;
	std::array<vec4, 3> screen_coords;
	std::array<vec2, 3> uvs;

	float* zbuffer = new float[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();
	
	TextureShader shader;
	for (int n = 1; n < argc; n++) {
		Model model(argv[n]);
		shader.projection = get_projection(vec3(2,0,3), CENTER);
		shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		shader.lookat = get_lookat(vec3(2,0,3), CENTER, vec3(0, 1, 0));
		shader.model = mat<4, 4>::identity();

		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				uvs[ivert] = model.uv(iface, ivert);
			}
			shader.uvs = uvs;
			shader.texture = model.diffuse();
			screen_coords = shader.vertex(world_coords);
			triangle(screen_coords, shader, zbuffer, image);
		}
	}

	

	//image.flip_vertically();
	image.write_tga_file("output.tga");
}

void render_phong(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords, normals;
	std::array<vec4, 3> screen_coords;
	std::array<vec2, 3> uvs;

	Light light;
	light.position = vec3(3, 3, 0);
	light.direction = vec3(-2, 2, 2);
	light.ambient = 0.1 * vec3(255, 255, 255);
	light.diffuse = vec3(255, 255, 255);
	light.specular = vec3(255, 255, 255);

	Material material;
	material.ambient = material.diffuse = material.specular = vec3(249, 210, 228);
	material.shininess = 32;

	float* zbuffer = new float[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();



	PhoneLightShader shader;
	for (int n = 1; n < argc; n++) {
		Model model(argv[n]);
		shader.projection = get_projection(EYE, CENTER);
		shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		shader.lookat = get_lookat(EYE, CENTER, vec3(0, 1, 0));
		shader.model = get_rotate(vec3(0,1,0),45);
		shader.eye = EYE;
		shader.material = material;
		shader.light = light;
		

		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				normals[ivert] = model.normal(iface, ivert).normalize();
				uvs[ivert] = model.uv(iface, ivert);
			}
			shader.normals = normals;
			screen_coords = shader.vertex(world_coords);
			triangle(screen_coords, shader, zbuffer, image);
		}
	}



	image.write_tga_file("output.tga");
}

void render_normal(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords, normals;
	std::array<vec4, 3> screen_coords;

	float* zbuffer = new float[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();

	NormalShader shader;
	for (int n = 1; n < argc; n++) {
		Model model(argv[n]);
		shader.projection = get_projection(EYE, CENTER);
		shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		shader.lookat = get_lookat(EYE, CENTER, vec3(0, 1, 0));
		shader.model = get_rotate(vec3(0, 1, 0), 45);

		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				normals[ivert] = model.normal(iface, ivert).normalize();
			}
			shader.normals = normals;
			screen_coords = shader.vertex(world_coords);
			triangle(screen_coords, shader, zbuffer, image);
		}
	}

	image.write_tga_file("output.tga");
}

void ssaa_render_phong(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords, normals;
	std::array<vec4, 3> screen_coords;
	std::array<vec2, 3> uvs;

	Light light;
	light.position = vec3(3, 3, 0);
	light.direction = vec3(-2, 2, 2);
	light.ambient = 0.1 * vec3(255, 255, 255);
	light.diffuse = vec3(255, 255, 255);
	light.specular = vec3(255, 255, 255);

	Material material;
	material.ambient = material.diffuse = material.specular = vec3(249, 210, 228);
	material.shininess = 32;

	float* zbuffer = new float[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();
	float** ssaa_zbuffer = new float* [HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) {
		ssaa_zbuffer[i] = new float[4];
		for (int j = 0; j < 4; j++) ssaa_zbuffer[i][j] = -std::numeric_limits<float>::max();
	}
	vec3** ssaa_framebuffer = new vec3 * [HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) {
		ssaa_framebuffer[i] = new vec3[4];
		for (int j = 0; j < 4; j++) ssaa_framebuffer[i][j] = vec3(0, 0, 0);
	}



	PhoneLightShader shader;
	for (int n = 1; n < argc; n++) {
		Model model(argv[n]);
		shader.projection = get_projection(EYE, CENTER);
		shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		shader.lookat = get_lookat(EYE, CENTER, vec3(0, 1, 0));
		shader.model = get_rotate(vec3(0, 1, 0), 45);
		shader.eye = EYE;
		shader.material = material;
		shader.light = light;


		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				normals[ivert] = model.normal(iface, ivert).normalize();
				uvs[ivert] = model.uv(iface, ivert);
			}
			shader.normals = normals;
			screen_coords = shader.vertex(world_coords);
			//triangle(screen_coords, shader, zbuffer, image);
			ssaa_triangle(screen_coords, shader, zbuffer, image, ssaa_zbuffer, ssaa_framebuffer);
		}
	}

	image.write_tga_file("output.tga");
}

void Bilinear_render_texture(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords;
	std::array<vec4, 3> screen_coords;
	std::array<vec2, 3> uvs;

	float* zbuffer = new float[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();

	BilinearTextureShader shader;
	for (int n = 1; n < argc; n++) {
		Model model(argv[n]);
		shader.projection = get_projection(vec3(2, 0, 3), CENTER);
		shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
		shader.lookat = get_lookat(vec3(2, 0, 3), CENTER, vec3(0, 1, 0));
		shader.model = mat<4, 4>::identity();

		for (int iface = 0; iface < model.nfaces(); iface++) {
			for (int ivert = 0; ivert < 3; ivert++) {
				world_coords[ivert] = model.vert(iface, ivert);
				uvs[ivert] = model.uv(iface, ivert);
			}
			shader.uvs = uvs;
			shader.texture = model.diffuse();
			screen_coords = shader.vertex(world_coords);
			triangle(screen_coords, shader, zbuffer, image);
		}
	}

	//image.flip_vertically();
	image.write_tga_file("output.tga");
}

void render_occlusion(int argc, char** argv) {
	if (2 > argc) {
		std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
		return;
	}

	TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);

	std::array<vec3, 3> world_coords, normals;
	std::array<vec4, 3> screen_coords;
	std::array<vec2, 3> uvs;

	DeepthShader deepth_shader;
	OcclusionShader occlu_shader;
	occlu_shader.occl.read_tga_file("occl.tga");

	const int nrenders = 30;
	for (int iter = 1; iter <= nrenders; iter++) {
		vec3 eye, up;
		std::cerr << iter << " from " << nrenders << std::endl;
		for (int i = 0; i < 3; i++) up[i] = (float)rand() / (float)RAND_MAX;
		eye = rand_point_on_unit_sphere();
		eye.y = std::abs(eye.y);
		std::cout << "v " << eye << std::endl;

		float* zbuffer = new float[HEIGHT * WIDTH], * shadow_buffer = new float[HEIGHT * WIDTH];
		for (int i = 0; i < HEIGHT * WIDTH; i++) zbuffer[i] = -std::numeric_limits<float>::max();
		for (int i = 0; i < HEIGHT * WIDTH; i++) shadow_buffer[i] = -std::numeric_limits<float>::max();


		for (int n = 1; n < argc-1; n++) {
			//Model model(R"(D:\code\MyTinyRenderer\obj\spot_triangulated_good.obj)");
			Model model(argv[n]);
			deepth_shader.projection = mat<4, 4>::identity();
			deepth_shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
			deepth_shader.lookat = get_lookat(eye, CENTER, up);
			deepth_shader.model = mat<4, 4>::identity();

			for (int iface = 0; iface < model.nfaces(); iface++) {
				for (int ivert = 0; ivert < 3; ivert++) {
					world_coords[ivert] = model.vert(iface, ivert);
				}
				screen_coords = deepth_shader.vertex(world_coords);
				triangle(screen_coords, deepth_shader, shadow_buffer, image);
			}
		}

		occlu_shader.occl.clear();
		for (int n = 1; n < argc-1; n++) {
			Model model(argv[n]);
			occlu_shader.projection = get_projection(eye, CENTER);
			occlu_shader.viewport = get_viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4);
			occlu_shader.lookat = get_lookat(eye, CENTER, up);
			occlu_shader.model = mat<4, 4>::identity();
			occlu_shader.deepth_matrix = deepth_shader.viewport * deepth_shader.projection * deepth_shader.lookat * deepth_shader.model;
			occlu_shader.shadow_buffer = shadow_buffer;
			occlu_shader.dim = vec2(WIDTH, HEIGHT);

			for (int iface = 0; iface < model.nfaces(); iface++) {
				for (int ivert = 0; ivert < 3; ivert++) {
					world_coords[ivert] = model.vert(iface, ivert);
					uvs[ivert] = model.uv(iface, ivert);
				}
				occlu_shader.uvs = uvs;
				screen_coords = occlu_shader.vertex(world_coords);
				triangle(screen_coords, occlu_shader, zbuffer, image);
			}
		}

		for (int i = 0; i < WIDTH; i++) {
			for (int j = 0; j < HEIGHT; j++) {
				float tmp = image.get(i, j)[0];
				float factor = (tmp * (iter - 1) + occlu_shader.occl.get(i, j)[0]) / (float)iter + .5f;
				image.set(i, j, TGAColor(factor,factor,factor,255));
			}
		}

		delete[] zbuffer;
		delete[] shadow_buffer;
	}

	//image.flip_vertically();
	image.write_tga_file("total_occl.tga");
	occlu_shader.occl.write_tga_file("occl.tga");
}

int main(int argc, char** argv) {
	render_occlusion(argc, argv);
	return 0;
}