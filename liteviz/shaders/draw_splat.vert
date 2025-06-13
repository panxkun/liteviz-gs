#version 430 core

#define SH_C0 0.28209479177387814f
#define SH_C1 0.4886025119029199f

#define SH_C2_0 1.0925484305920792f
#define SH_C2_1 -1.0925484305920792f
#define SH_C2_2 0.31539156525252005f
#define SH_C2_3 -1.0925484305920792f
#define SH_C2_4 0.5462742152960396f

#define SH_C3_0 -0.5900435899266435f
#define SH_C3_1 2.890611442640554f
#define SH_C3_2 -0.4570457994644658f
#define SH_C3_3 0.3731763325901154f
#define SH_C3_4 -0.4570457994644658f
#define SH_C3_5 1.445305721320277f
#define SH_C3_6 -0.5900435899266435f

#define P_INDEX 0
#define R_INDEX 3
#define S_INDEX 7
#define O_INDEX 10
#define C_INDEX 11

layout(location = 0) in vec2 position;

layout (std430, binding=0) buffer _splats {
	float splat[];
};
layout (std430, binding=1) buffer _index {
	int index[];
};

uniform mat4 projmat;
uniform mat4 viewmat;
uniform vec3 cam_pos;
uniform vec2 tanxy;
uniform float focal;
uniform float scale_modifier;
uniform int max_sh_dim;
uniform int render_mod;

out vec3 color;
out float alpha;
out vec3 conic;
out vec2 coordxy;  // local coordinate in quad, unit in pixel

mat3 computeCov3D(vec3 scale, vec4 q)  // should be correct
{
    mat3 S = mat3(0.f);
    S[0][0] = scale.x;
	S[1][1] = scale.y;
	S[2][2] = scale.z;
	float r = q.x;
	float x = q.y;
	float y = q.z;
	float z = q.w;

    mat3 R = mat3(
		1.f - 2.f * (y * y + z * z), 2.f * (x * y - r * z), 2.f * (x * z + r * y),
		2.f * (x * y + r * z), 1.f - 2.f * (x * x + z * z), 2.f * (y * z - r * x),
		2.f * (x * z - r * y), 2.f * (y * z + r * x), 1.f - 2.f * (x * x + y * y)
	);

    mat3 M = S * R;
    mat3 Sigma = transpose(M) * M;
    return Sigma;
}

vec3 computeCov2D(vec4 mean_view, float focal_x, float focal_y, float tan_fovx, float tan_fovy, mat3 cov3D, mat4 viewmatrix)
{
    vec4 t = mean_view;
    // why need this? Try remove this later
    float limx = 1.3f * tan_fovx;
    float limy = 1.3f * tan_fovy;
    float txtz = t.x / t.z;
    float tytz = t.y / t.z;
    t.x = min(limx, max(-limx, txtz)) * t.z;
    t.y = min(limy, max(-limy, tytz)) * t.z;

    mat3 J = mat3(
        focal_x / t.z, 0.0f, -(focal_x * t.x) / (t.z * t.z),
		0.0f, focal_y / t.z, -(focal_y * t.y) / (t.z * t.z),
		0, 0, 0
    );
    mat3 W = transpose(mat3(viewmatrix));
    mat3 T = W * J;

    mat3 cov = transpose(T) * transpose(cov3D) * T;
    // Apply low-pass filter: every Gaussian should be at least
	// one pixel wide/high. Discard 3rd row and column.
	cov[0][0] += 0.3f;
	cov[1][1] += 0.3f;
    return vec3(cov[0][0], cov[0][1], cov[1][1]);
}

vec3 get_vec3(int offset)
{
	return vec3(splat[offset], splat[offset + 1], splat[offset + 2]);
}
vec4 get_vec4(int offset)
{
	return vec4(splat[offset], splat[offset + 1], splat[offset + 2], splat[offset + 3]);
}

void main()
{
	int splat_idx = index[gl_InstanceID];
	int splat_dim = 3 + 4 + 3 + 1 + max_sh_dim;

	int start = splat_idx * splat_dim;

	vec4 g_pos = vec4(get_vec3(start + P_INDEX), 1.f);
    vec4 g_pos_view = viewmat * g_pos;
    vec4 g_pos_screen = projmat * g_pos_view;

	g_pos_screen.xyz = g_pos_screen.xyz / g_pos_screen.w;
    g_pos_screen.w = 1.f;

	if (any(greaterThan(abs(g_pos_screen.xyz), vec3(1.3)))){
		gl_Position = vec4(-100, -100, -100, 1);
		return;
	}
	vec4 g_rot = get_vec4(start + R_INDEX);
	vec3 g_scale = get_vec3(start + S_INDEX);
	float g_opacity = splat[start + O_INDEX];

    mat3 cov3d = computeCov3D(g_scale * scale_modifier, g_rot);
    vec2 wh = 2 * tanxy * focal;
    vec3 cov2d = computeCov2D(g_pos_view, focal, focal, tanxy.x, tanxy.y, cov3d, viewmat);

    // Invert covariance (EWA algorithm)
	float det = (cov2d.x * cov2d.z - cov2d.y * cov2d.y);
	if (det == 0.0f)
		gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
    
    float det_inv = 1.f / det;
	conic = vec3(cov2d.z * det_inv, -cov2d.y * det_inv, cov2d.x * det_inv);
    
    vec2 quadwh_scr = vec2(3.f * sqrt(cov2d.x), 3.f * sqrt(cov2d.z));  // screen space half quad height and width
    vec2 quadwh_ndc = quadwh_scr / wh * 2;  // in ndc space
    g_pos_screen.xy = g_pos_screen.xy + position * quadwh_ndc;
    coordxy = position * quadwh_scr;
    gl_Position = g_pos_screen;
    
    alpha = g_opacity;

	if (render_mod == 4){ // depth
		float depth = -g_pos_view.z;
		depth = depth < 0.05 ? 1 : depth;
		depth = 1 / depth;
		color = vec3(depth, depth, depth);
		return;
	}

	// Covert SH to color
	int sh_start = start + C_INDEX;
	vec3 dir = g_pos.xyz - cam_pos;
    dir = normalize(dir);
	color = SH_C0 * get_vec3(sh_start);
	
	if (render_mod >= 1 && max_sh_dim >= 4){
		float x = dir.x;
		float y = dir.y;
		float z = dir.z;
		color = color - 
			SH_C1 * y * get_vec3(sh_start + 1 * 3) + 
			SH_C1 * z * get_vec3(sh_start + 2 * 3) - 
			SH_C1 * x * get_vec3(sh_start + 3 * 3);
		if (render_mod >= 2 && max_sh_dim >= 9){
			float xx = x * x, yy = y * y, zz = z * z;
			float xy = x * y, yz = y * z, xz = x * z;
			color = color +
				SH_C2_0 * xy * get_vec3(sh_start + 4 * 3) +
				SH_C2_1 * yz * get_vec3(sh_start + 5 * 3) +
				SH_C2_2 * (2.0f * zz - xx - yy) * get_vec3(sh_start + 6 * 3) +
				SH_C2_3 * xz * get_vec3(sh_start + 7 * 3) +
				SH_C2_4 * (xx - yy) * get_vec3(sh_start + 8 * 3);

			if (render_mod >= 3 && max_sh_dim >= 16){
				color = color +
					SH_C3_0 * y * (3.0f * xx - yy) * get_vec3(sh_start + 9 * 3) +
					SH_C3_1 * xy * z * get_vec3(sh_start + 10 * 3) +
					SH_C3_2 * y * (4.0f * zz - xx - yy) * get_vec3(sh_start + 11 * 3) +
					SH_C3_3 * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * get_vec3(sh_start + 12 * 3) +
					SH_C3_4 * x * (4.0f * zz - xx - yy) * get_vec3(sh_start + 13 * 3) +
					SH_C3_5 * z * (xx - yy) * get_vec3(sh_start + 14 * 3) +
					SH_C3_6 * x * (xx - 3.0f * yy) * get_vec3(sh_start + 15 * 3);
			}
		}
	}
	color += 0.5f;
}