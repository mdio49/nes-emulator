#version 330 core
layout (location = 0) in vec3 pos;
out vec2 screenPos;

void main() {
	vec3 ndcPos = 2 * pos - vec3(1);
	screenPos = vec2(ndcPos.x * 256, ndcPos.y * 256);
	gl_Position = vec4(ndcPos.x, ndcPos.y, ndcPos.z, 1.0f);
}
