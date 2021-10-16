#version 330 core

layout (location = 0) in vec3 pos;

void main() {
    gl_Position = vec4(pos.x/320-1, (480-pos.y)/240-1, pos.z, 1.0);
}