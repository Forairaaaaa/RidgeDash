#version 330

// CRT power-off transition. The CPU freezes the final scene during collapse,
// holds on black briefly, then closes the game.
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float uCollapse; // 0 = full frame, 0.62 = horizontal beam, 1 = center point
uniform float uBlackout;
uniform float uFlash;

out vec4 finalColor;

void main()
{
    if (uBlackout > 0.5) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 center = vec2(0.5);
    float verticalProgress = clamp(uCollapse / 0.62, 0.0, 1.0);
    float horizontalProgress = clamp((uCollapse - 0.62) / 0.38, 0.0, 1.0);

    // Compress the complete image rather than cropping it. A few surviving
    // pixels at the limits form the characteristic bright line and center dot.
    float halfHeight = mix(0.5, 0.0045, verticalProgress);
    float halfWidth = mix(0.5, 0.0040, horizontalProgress);
    vec2 sampleScale = vec2(max(halfWidth * 2.0, 0.008),
                            max(halfHeight * 2.0, 0.009));
    vec2 sampleUv = center + (fragTexCoord - center) / sampleScale;

    vec2 distanceFromCenter = abs(fragTexCoord - center);
    float featherX = max(fwidth(fragTexCoord.x) * 1.5, 0.0008);
    float featherY = max(fwidth(fragTexCoord.y) * 1.5, 0.0008);
    float maskX = 1.0 - smoothstep(halfWidth, halfWidth + featherX, distanceFromCenter.x);
    float maskY = 1.0 - smoothstep(halfHeight, halfHeight + featherY, distanceFromCenter.y);
    float mask = maskX * maskY;

    vec3 color = texture(texture0, clamp(sampleUv, 0.0, 1.0)).rgb;
    float beamFlash = exp(-pow((uCollapse - 0.62) / 0.085, 2.0)) * uFlash;
    color = color * (1.0 + beamFlash * 0.70) + vec3(beamFlash * 0.055);

    finalColor = vec4(color * mask, 1.0) * colDiffuse * fragColor;
}
