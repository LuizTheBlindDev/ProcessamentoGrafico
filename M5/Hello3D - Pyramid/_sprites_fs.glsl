#version 410

in vec2 texture_coords;

uniform sampler2D sprite;

out vec4 frag_color;

void main() {
    frag_color = texture(sprite, texture_coords);
}