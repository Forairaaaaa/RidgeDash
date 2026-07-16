#version 330

// Desktop bloom bright-pass. This is rendered at half output resolution; the
// following mip chain supplies the actual broad blur through bilinear filtering.
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float uThreshold;
uniform float uKnee;

out vec4 finalColor;

void main()
{
    vec3 col = texture(texture0, fragTexCoord).rgb;
    float luminance = dot(col, vec3(0.299, 0.587, 0.114));
    float weight = smoothstep(uThreshold, uThreshold + uKnee, luminance);
    // A second-order shoulder keeps midtones from glowing while letting the
    // brightest pixels roll into bloom without a visible cutoff.
    weight *= weight;
    finalColor = vec4(col * weight, 1.0) * colDiffuse * fragColor;
}
