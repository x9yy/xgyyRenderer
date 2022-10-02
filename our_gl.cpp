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