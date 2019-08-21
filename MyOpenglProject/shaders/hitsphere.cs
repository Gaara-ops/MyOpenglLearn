#version 430

layout (local_size_x = 1, local_size_y = 1) in;

layout (rgba32f, binding = 0) uniform image2D destTex;

struct      Camera {
    vec3    origin;
    vec3    lower_left_corner;
    vec3    horizontal;
    vec3    vertical;
};

struct ray {
    vec3    origin;
    vec3    direction;
};

uniform Camera  uCamera;

uniform vec2    uSize;

float dot( vec3 v1,  vec3 v2) {
    return v1.x *v2.x + v1.y *v2.y  + v1.z *v2.z;
}
bool hit_sphere( vec3 center, float radius, ray r)
{
	vec3 oc = r.origin - center;
	float a = dot(r.direction, r.direction);
	float b = 2.0f*dot(oc, r.direction);
	float c = dot(oc, oc) - radius*radius;
	float discrimiant = b*b - 4.0f*a*c;
	return (discrimiant > 0.0f);
}

vec4 GetColor(ray r){
	if (hit_sphere(vec3(0.0f, 0.0f, -1.0f), 0.5f, r)){
        return vec3(1.0f, 0.0f, 0.0f);
    }

	vec3 unit_direction = normalize(r.direction);

	float t = 0.5f*(unit_direction.y + 1.0f);

	return (1.0f - t)*vec3(1.0f, 1.0f, 1.0f) + t*vec3(0.5f, 0.7f, 1.0f);
}

void        main() {
    ivec2   pos = ivec2(gl_GlobalInvocationID.xy);
    vec2    fpos = vec2(pos.xy);
    
    float u = float(fpos.x) / float(uSize.x);
    float v = float(fpos.y) / float(uSize.y);
    
    ray r;
    r.origin = uCamera.origin;
    r.direction = uCamera.lower_left_corner + u*uCamera.horizontal + v*uCamera.vertical;
    
    vec4    color = GetColor(r);

    imageStore(destTex, pos, color);
}
