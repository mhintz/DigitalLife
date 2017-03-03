#version 330

in vec2 TexCoord0;

out vec4 FragColor;

uniform sampler2D uBirdsTex;

void main() {
  FragColor = texture(uBirdsTex, TexCoord0);
}
