#version 410

in vec4 aColor;
in vec2 aTexCoord0;

out vec4 FragColor;

uniform sampler2D uBirdsTex;

void main() {
  FragColor = texture(uBirdsTex, aTexCoord0);
}
