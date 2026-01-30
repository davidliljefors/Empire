layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_mvp;

out vec3 v_normal;
out vec2 v_texcoord;
out vec3 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = a_normal;
    v_texcoord = a_texcoord;
    
    // Simple directional lighting
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(a_normal, light_dir), 0.0) * 0.8 + 0.2;
    v_color = vec3(diffuse);
}
