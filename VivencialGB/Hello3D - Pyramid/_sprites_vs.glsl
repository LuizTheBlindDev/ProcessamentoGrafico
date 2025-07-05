#version 410

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 texture_mapping;

uniform vec2 spritePos;
uniform float offsetx;
uniform float offsety;

out vec2 texture_coords;

void main() {
    // Aplica offset da animação
    texture_coords = texture_mapping * vec2(0.25, 0.25) + vec2(offsetx, offsety);
    gl_Position = vec4(vertex_position + spritePos, 0.1, 1.0);  // z = 0.1 opcional
}