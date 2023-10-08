#version 330 core

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec3 fColor;
layout(location = 2) in mat4 instanceMatrix;

out vec2 TexCoords;
out vec3 fragColor;

// uniform mat4 model;
uniform mat4 projection;

void main() {
	TexCoords = vertex.zw;
	gl_Position = projection * instanceMatrix * vec4(vertex.xy, 0.0, 1.0);
	fragColor = fColor;
}