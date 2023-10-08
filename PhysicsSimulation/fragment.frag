#version 330 core

in vec2 TexCoords;
in vec3 fragColor;

out vec4 color;

uniform sampler2D image;

void main() {
	color = vec4(fragColor, 1.0) * texture(image, TexCoords);
}