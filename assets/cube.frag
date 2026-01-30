in vec3 v_normal;
in vec2 v_texcoord;
in vec3 v_color;

uniform sampler2D u_texture;
uniform int u_has_texture;

out vec4 frag_color;

void main() {
    vec3 base_color;
    
    if (u_has_texture != 0) {
        // Use texture if available
        base_color = texture(u_texture, v_texcoord).rgb;
    } else {
        // Use white color for non-textured models
        base_color = vec3(1.0, 1.0, 1.0);
    }
    
    // Apply lighting from vertex shader
    frag_color = vec4(base_color * v_color, 1.0);
}
