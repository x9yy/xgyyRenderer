#ifndef SHADER_H
#define SHADER_H

#include "our_gl.h"
#include "geometry.h"

#include <memory>

//法线可视化
class NormalShader :public Shader {
public:
	mat<4, 4> projection;
	mat<4, 4> viewport;
	mat<4, 4> lookat;
	mat<4, 4> model;
	std::array<vec3,3> normals;
	std::array<vec4,3> coords;

	std::array<vec4,3> vertex(std::array<vec3,3> world_coords){
		for (int i = 0; i < 3; i++) {
			normals[i] = model.invert_transpose().get_minor(3, 3) * normals[i];
			coords[i] = Homogenization(viewport * projection * lookat * model * embed<4>(world_coords[i]));
			coords[i].z = coords[i].w * coords[i].z;
		}
		return coords;
	} 
	std::optional<TGAColor> fragment(vec3 bar){

		vec3 normal = normals[0] * bar[0] + normals[1] * bar[1] + normals[2] * bar[2];
		normal = (normal.normalize() + vec3(1.0f, 1.0f, 1.0f)) / 2;
		TGAColor color(normal.x * 255, normal.y * 255, normal.z * 255, 255);
		return color;
	}
	
};

//冯氏光照(平行光)
class PhoneLightShader : public Shader {
public:
	mat<4, 4> projection;
	mat<4, 4> viewport;
	mat<4, 4> lookat;
	mat<4, 4> model;
	vec3 eye;
	std::array<vec3, 3> normals;
	std::array<vec4, 3> coords;

	Material material;
	Light light;
	
	std::array<vec4, 3> vertex(std::array<vec3, 3> world_coords) {
		std::array<vec4, 3> res;
		for (int i = 0; i < 3; i++) {
			normals[i] =  model.invert_transpose().get_minor(3, 3) * normals[i];
			coords[i] = model * embed<4>(world_coords[i],1);
			res[i] = Homogenization(viewport * projection * lookat * model * embed<4>(world_coords[i],1));
		}
		return res;
	}

	std::optional<TGAColor> fragment(vec3 bar) {
		vec3 normal = (normals[0] * bar[0] + normals[1] * bar[1] + normals[2] * bar[2]).normalize();
		vec3 ambient, diffuse, specular;
		float diff, spec;


		auto absorb = [](const vec3& color1,const vec3& color2) -> vec3 {
			return { color1.x * color2.x / 255,color1.y * color2.y / 255,color1.z * color2.z / 255 };
		};

		//环境光
		ambient = absorb(material.ambient, light.ambient);
		//漫反射
		light.direction = light.direction.normalize();
		diff = std::max(light.direction * normal, double(0));
		diffuse = diff * absorb(material.diffuse, light.diffuse);
		//镜面光
		vec3 coord = proj<3>(coords[0] * bar[0] + coords[1] * bar[1] + coords[2] * bar[2]);
		vec3 eye_direction = (eye - coord).normalize();
		vec3 mid_vector = (eye_direction + light.direction) / 2;
		vec3 r = (normal * (normal * light.direction * 2.f) - light.direction).normalize();   // reflected light
		spec = std::max(r * eye_direction, double(0));
		spec = std::pow(spec, material.shininess);
		specular = absorb(material.specular, light.specular) * spec ;

		vec3 res = ambient + diffuse + specular;
		for (int i = 0; i < 3; i++) res[i] = res[i] > 255 ? 255 : res[i];
		return TGAColor(res.x,res.y,res.z,255);

	}
};

//纹理
class TextureShader:public Shader {
public:
	TGAImage texture;
	mat<4, 4> projection;
	mat<4, 4> viewport;
	mat<4, 4> lookat;
	mat<4, 4> model;
	std::array<vec4, 3> coords;
	std::array<vec2, 3> uvs;

	TGAColor sample2D(const TGAImage& img, vec2& uvf) {
		return img.get(uvf[0] * img.width(), uvf[1] * img.height());
	}

	std::array<vec4, 3> vertex(std::array<vec3, 3> world_coords) {
		std::array<vec4, 3> res;
		for (int i = 0; i < 3; i++) {
			coords[i] = model * embed<4>(world_coords[i], 1);
			res[i] = Homogenization(viewport * projection * lookat * model * embed<4>(world_coords[i], 1));
		}
		return res;
	}

	std::optional<TGAColor> fragment(vec3 bar) {
		vec2 uv = uvs[0] * bar[0] + uvs[1] * bar[1] + uvs[2] * bar[2];
		return sample2D(texture, uv);
	}

};

//深度贴图
class DeepthShader :public Shader {
public:
	mat<4, 4> projection;
	mat<4, 4> viewport;
	mat<4, 4> lookat;
	mat<4, 4> model;

	std::array<vec4, 3> vertex(std::array<vec3, 3> world_coords) {
		std::array<vec4, 3> res;
		for (int i = 0; i < 3; i++) res[i] = Homogenization(viewport * projection * lookat * model * embed<4>(world_coords[i], 1));
		return res;
	}

	std::optional<TGAColor> fragment(vec3 bar) {
		return std::nullopt;
	}

};

//阴影
class ShadowShader :public Shader {
private:
	std::array<vec4, 3> deepth_coords;
public:
	mat<4, 4> projection;
	mat<4, 4> viewport;
	mat<4, 4> lookat;
	mat<4, 4> model;
	mat<4, 4> deepth_matrix;
	vec3 eye;
	std::array<vec3, 3> normals;
	std::array<vec4, 3> coords;
	float* shadow_buffer;
	vec2 dim;

	Material material;
	Light light;

	std::array<vec4, 3> vertex(std::array<vec3, 3> world_coords) {
		std::array<vec4, 3> res;
		for (int i = 0; i < 3; i++) {
			normals[i] = model.invert_transpose().get_minor(3, 3) * normals[i];
			coords[i] = model * embed<4>(world_coords[i], 1);
			deepth_coords[i] = Homogenization(deepth_matrix * embed<4>(world_coords[i], 1));
			deepth_coords[i].z = deepth_coords[i].z * deepth_coords[i].w;
			res[i] = Homogenization(viewport * projection * lookat * model * embed<4>(world_coords[i], 1));
		}
		return res;
	}

	std::optional<TGAColor> fragment(vec3 bar) {
		vec3 normal = (normals[0] * bar[0] + normals[1] * bar[1] + normals[2] * bar[2]).normalize();
		vec3 ambient, diffuse, specular;
		float diff, spec,shadow;

		auto absorb = [](const vec3& color1, const vec3& color2) -> vec3 {
			return { color1.x * color2.x / 255,color1.y * color2.y / 255,color1.z * color2.z / 255 };
		};

		vec3 coord = proj<3>(coords[0] * bar[0] + coords[1] * bar[1] + coords[2] * bar[2]);
		float z_interpolated = deepth_coords[0].z * bar[0] + deepth_coords[1].z * bar[1] + deepth_coords[2].z * bar[2];
		vec3 ori_bar;
		for (int i = 0; i < 3; i++) ori_bar[i] = (bar[i] / z_interpolated) * deepth_coords[i].z;
		vec3 point = proj<3>(deepth_coords[0] * ori_bar[0] + deepth_coords[1] * ori_bar[1] + deepth_coords[2] * ori_bar[2]);
		int idx = int(point.x) + int(point.y) * int(dim.x);
		if (idx <= 0 || idx >= dim.x * dim.y - 1) shadow = 0.3;
		else shadow = 0.3 + 0.7 * (shadow_buffer[idx] < z_interpolated + 0.08 );
		//环境光
		ambient = absorb(material.ambient, light.ambient);
		//漫反射
		light.direction = (light.position - coord).normalize();
		diff = std::max(light.direction * normal, double(0));
		diffuse = diff * absorb(material.diffuse, light.diffuse);
		//镜面光
		vec3 eye_direction = (eye - coord).normalize();
		vec3 mid_vector = (eye_direction + light.direction) / 2;
		vec3 r = (normal * (normal * light.direction * 2.f) - light.direction).normalize();   // reflected light
		//spec = std::max(mid_vector.normalize() * normal, double(0));
		spec = std::max(r * eye_direction, double(0));
		spec = std::pow(spec, material.shininess);
		specular = absorb(material.specular, light.specular) * spec;

		vec3 res = ambient + diffuse + specular;
		res = { res.x * shadow ,res.y * shadow,res.z * shadow };
		for (int i = 0; i < 3; i++) res[i] = res[i] > 255 ? 255 : res[i];
		return TGAColor(res.x, res.y, res.z, 255);

	}
};



#endif // !SHADER_H
