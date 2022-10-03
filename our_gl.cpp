#include "our_gl.h"

float to_radian(float angle) {
	float pi = 3.141592653;
	return (angle / 180) * pi;
}

mat<4, 4> get_rotate(vec3 axis, float angle){
	float radian = to_radian(angle);
	axis = axis.normalize();

	mat<4, 4> rotate = mat<4, 4>::identity();

	auto f = [=](vec3 v)-> vec3 {
		return cos(radian) * v + (1 - cos(radian)) * (axis*v) * axis + sin(radian) * cross(axis,v);
	};
	vec3 e1, e2, e3;
	e1 = f(vec3(1.0f, 0.0f, 0.0f));
	e2 = f(vec3(0.0f, 1.0f, 0.0f));
	e3 = f(vec3(0.0f, 0.0f, 1.0f));

	rotate = { {{e1[0], e2[0], e3[0], 0},{e1[1], e2[1], e3[1], 0},{e1[2], e2[2], e3[2], 0},{0, 0, 0, 1}} };
	return rotate;
}

mat<4, 4> get_trans(vec3 t) {
	mat<4, 4> trans = mat<4, 4>::identity();
	trans = { {{1,0,0,t.x},{0,1,0,t.y},{0,0,1,t.z},{0,0,0,1}} };
	return trans;
}

mat<4, 4> get_lookat(vec3 eye, vec3 center, vec3 up){
	vec3 x_axis, y_axis, z_axis;
	z_axis = (eye - center).normalize();
	x_axis = cross(up, z_axis).normalize();
	y_axis = cross(z_axis, x_axis).normalize();

	mat<4, 4> rotate = mat<4,4>::identity(), trans = mat<4,4>::identity();

	for (int i = 0; i < 3; i++) {
		rotate[0][i] = x_axis[i];
		rotate[1][i] = y_axis[i];
		rotate[2][i] = z_axis[i];
		trans[i][3] = -eye[i];
	}
	return rotate * trans;

}

mat<4, 4> get_projection(vec3 eye,vec3 center){
	float d = (eye - center).norm();
	mat<4, 4> projection = { {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,-1 / d,0}} };
	return projection;
}

mat<4, 4> get_viewport(int x, int y, int w, int h){
	float d = 255;
	//mat<4, 4> viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,2 / d,2 / d}, {0,0,0,1}} };
	mat<4, 4> viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
	return viewport;
}

vec4 Homogenization(vec4 v) {
	return { v[0] / v[3],v[1] / v[3],v[2] / v[3],v[3]};
}

void line(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color) {
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int y = y0;
	int error = 0, dlx = dx << 1, dly = std::abs(dy) << 1;
	for (int x = x0; x <= x1; x++) {
		if (steep)
			image.set(y, x, color);
		else
			image.set(x, y, color);
		error += dly;
		if (error > dx) {
			y += (y0 < y1 ? 1 : -1);
			error -= dlx;
		}
	}
}

void triangle(std::array<vec4,3> v, Shader& shader, float* zbuffer, TGAImage& image) {
	//退化成一条线
	if ((v[0].x == v[1].x && v[1].x == v[2].x) || (v[0].y == v[1].y && v[1].y == v[2].y)) return;
	//找到boundingBox
	auto [left, right, bottom, top] = boundingBox(v);
	//裁剪
	if (left < 0) left = 0;if (bottom < 0) bottom = 0;
	if (right > image.width()) right = image.width() - 1;if (top > image.height()) top = image.height() - 1;
	//获取坐标
	auto get_index = [&](int x, int y) -> int {
		return x + y * image.width();
	};
	//计算z坐标
	for (vec4& coord : v) coord.z = coord.z * coord.w;
	//渲染
	for (int x = left; x <= right; x++) {
		for (int y = bottom; y <= top; y++) {
			//auto [alpha, beta, gamma] = computeBarycentric2D(x + 0.5, y + 0.5, v);
			auto bary_coords = computeBarycentric2D(x , y , v);
			if (bary_coords[0] < 0 || bary_coords[1] < 0 || bary_coords[2] < 0)	continue;

			//float z_interpolated = bary_coords[0] * v[0].z + bary_coords[1] * v[1].z + bary_coords[2] * v[2].z;
			
			//矫正
			for (int i = 0; i < 3; i++) bary_coords[i] /= v[i].z;
			float z_interpolated = 1.f / (bary_coords[0] + bary_coords[1] + bary_coords[2]);
			for (int i = 0; i < 3; i++) bary_coords[i] *= z_interpolated;

			if (zbuffer[get_index(x, y)] < z_interpolated) {
				zbuffer[get_index(x, y)] = z_interpolated;
				auto color = shader.fragment(bary_coords);
				if (color.has_value())
					image.set(x, y, *color);
			}
		}
	}
}

void ssaa_triangle(std::array<vec4, 3> v, Shader& shader, float* zbuffer, TGAImage& image, float** ssaa_zbuffer, vec3** ssaa_framebuffer) {
	//退化成一条线
	if ((v[0].x == v[1].x && v[1].x == v[2].x) || (v[0].y == v[1].y && v[1].y == v[2].y)) return;
	//找到boundingBox
	auto [left, right, bottom, top] = boundingBox(v);
	//裁剪
	if (left < 0) left = 0; if (bottom < 0) bottom = 0;
	if (right > image.width()) right = image.width() - 1; if (top > image.height()) top = image.height() - 1;
	//获取坐标
	auto get_index = [&](int x, int y) -> int {
		return x + y * image.width();
	};
	//计算z坐标
	for (vec4& coord : v) coord.z = coord.z * coord.w;
	 //遍历bonding box
	for (float x = left; x <= right; x++)
		for (float y = bottom; y <= top; y++) {
			int index = 0;
			for (float i = 0.25f; i < 1.0f; i += 0.5f) {
				for (float j = 0.25f; j < 1.0f; j += 0.5f, index++) {
					//获取重心坐标
					auto bary_coords = computeBarycentric2D(x+i, y+j, v);
					if (bary_coords[0] < 0 || bary_coords[1] < 0 || bary_coords[2] < 0)	continue;
					//矫正
					for (int i = 0; i < 3; i++) bary_coords[i] /= v[i].z;
					float z_interpolated = 1.f / (bary_coords[0] + bary_coords[1] + bary_coords[2]);
					for (int i = 0; i < 3; i++) bary_coords[i] *= z_interpolated;
					//深度测试
					if (ssaa_zbuffer[get_index(x, y)][index] < z_interpolated) {
						ssaa_zbuffer[get_index(x, y)][index] = z_interpolated;
						//超采渲染
						auto color = shader.fragment(bary_coords);
						if (color.has_value())
							ssaa_framebuffer[get_index(x, y)][index] = {(double)color->bgra[2],(double)color->bgra[1],(double)color->bgra[0]};
					}
					
				}
			}
		}
	//求均值
	for (float x = left; x < right; x++)
		for (float y = bottom; y < top; y++) {
			vec3 color = { 0,0,0 };
			for (int i = 0; i < 4; i++)
				color = color + ssaa_framebuffer[get_index(x, y)][i];
			color = color / 4;
			image.set(x, y, TGAColor(color.x,color.y,color.z,255));
		}
}

TGAColor getColorBilinear(TGAImage& texture, vec2 uv) {
	float width = texture.width(), height = texture.height();
	float u_img = uv.x * width, v_img = uv.y * height;
	//坐标限定
	if (u_img < 0) u_img = 0;
	if (u_img >= width) u_img = width - 1;
	if (v_img < 0) v_img = 0;
	if (v_img >= height) v_img = height - 1;
	//算出各点坐标
	vec2 center = { std::round(u_img),std::round(v_img) };
	float step = 0.5f;
	std::array<vec2, 4> points = {
		vec2(center.x - step,center.y - step),
		vec2(center.x + step,center.y - step),
		vec2(center.x - step,center.y + step),
		vec2(center.x + step,center.y + step)
	};
	//算出各点颜色
	auto color = [&](const vec2& point) -> vec3 {
		TGAColor res = point.x >= 0 && point.x <= width && point.y >= 0 && point.y <= height ? texture.get(point.x, point.y) : texture.get(u_img, v_img);
		return { (double)res.bgra[2],(double)res.bgra[1],(double)res.bgra[0] };
	};
	std::array<vec3, 4> colors = {
		color(points[0]),
		color(points[1]),
		color(points[2]),
		color(points[3])
	};
	//进行插值
	float s = (u_img - points[0].x) / (step * 2), t = (v_img - points[0].y) / (step * 2);
	vec3 color1 = s * colors[1] + (1 - s) * colors[0], color2 = s * colors[3] + (1 - s) * colors[2];
	vec3 res_color = t * color2 + (1 - t) * color1;
	return TGAColor(res_color.x, res_color.y, res_color.z, 255);
}

vec3 rand_point_on_unit_sphere() {
	constexpr double pi = 3.141592653;
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;
	float theta = 2.f * pi * u;
	float phi = acos(2.f * v - 1.f);
	return vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}