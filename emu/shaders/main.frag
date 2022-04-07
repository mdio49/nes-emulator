#version 330 core
out vec4 FragColor;

in vec2 screenPos;
uniform int data[256 * 256];

void main() {
    int pixel = data[screenPos.y * 256 + screenPos.x];
    int r = pixel & 255;
    int g = (pixel >> 8) & 255;
    int b = (pixel >> 16) & 255;
    FragColor = vec4(r / 255f, g / 255f, b / 255f, 1.0f);
}
