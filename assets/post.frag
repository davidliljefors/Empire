in vec2 v_texcoordScene;
uniform sampler2D scene;

in vec2 v_texcoordPalette;
uniform sampler2D palette;

out vec4 frag_color;

void main() {
    vec3 base_color;
    
    vec3 sceneColor = vec3(texture(scene, v_texcoord));
    vec3 paletteColor = vec3(v_texcoordScene / max(0.05, sceneColor.r) )

    frag_color = vec4(paletteColor, 1.0);
}
