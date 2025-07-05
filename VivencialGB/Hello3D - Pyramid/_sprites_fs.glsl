#version 410

in vec2 texture_coords;
uniform sampler2D sprite;
out vec4 frag_color;

void main() {
    vec4 texColor = texture(sprite, texture_coords);

    if (texColor.a < 0.01)
        discard;

    frag_color = texColor;
}