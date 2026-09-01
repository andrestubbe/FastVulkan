#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
    if (fragTexCoord.x < 0.0) {
        // Colored quad mode — no texture, pure vertex color
        outColor = fragColor;
    } else {
        // Textured quad mode
        outColor = texture(texSampler, fragTexCoord) * fragColor;
    }
}
