#ifndef OUR_GL_H
#define OUR_GL_H

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

#include <tuple>
#include <optional>
#include <array>

//光照属性
struct Light{
	vec3 position;
	vec3 direction;
	float intensity;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

//材质属性
struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float shininess;
};

class Shader {
public:
	virtual std::array<vec4,3> vertex(std::array<vec3,3> world_coords) = 0;
	virtual std::optional<TGAColor> fragment(vec3 bar) = 0;
};

float to_radian(float angle);

mat<4,4> get_rotate(vec3 axis,float angle);

mat<4, 4> get_trans(vec3 t);

mat<4,4> get_lookat(vec3 eye,vec3 center,vec3 up);

mat<4,4> get_projection(vec3 eye, vec3 center);

mat<4,4> get_viewport(int x,int y,int w,int h);

vec4 Homogenization(vec4 v);

template <typename T>
std::tuple<float, float, float, float> boundingBox(const T v) {
	float left = v[0].x, right = v[0].x, bottom = v[0].y, top = v[0].y;
	for (int i = 1; i < 3; i++) {
		left = left < v[i].x ? left : v[i].x;
		right = right > v[i].x ? right : v[i].x;
		bottom = bottom < v[i].y ? bottom : v[i].y;
		top = top > v[i].y ? top : v[i].y;
	}
	left = floor(left), bottom = floor(bottom), right = ceil(right), top = ceil(top);
	return { left,right,bottom,top };
}

template <typename T>
vec3 computeBarycentric2D(float x, float y, const T v) {
	float beta = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);
	float gamma = (x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * y + v[0].x * v[1].y - v[1].x * v[0].y) / (v[2].x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * v[2].y + v[0].x * v[1].y - v[1].x * v[0].y);
	float alpha = 1 - beta - gamma;
	return { alpha,beta,gamma };
}

template <typename T>
T interpolate(float alpha, float beta, float gamma, T vert1, T vert2, T vert3, float weight) {
	//return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
	return (vert1 * alpha + vert2 * beta + vert3 * gamma) / weight;
}

void line(int x0, int x1, int y0, int y1, TGAImage& image, const TGAColor& color);

void triangle(std::array<vec4,3> v,Shader& shader,float* zbuffer,TGAImage& image);

void ssaa_triangle(std::array<vec4, 3> v, Shader& shader, float* zbuffer, TGAImage& image, float** ssaa_zbuffer, vec3** ssaa_framebuffer);

TGAColor getColorBilinear(TGAImage& texture, vec2 uv);

vec3 rand_point_on_unit_sphere();

#endif // !OUR_GL_H
