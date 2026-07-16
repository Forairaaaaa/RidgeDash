#version 330

// Fast desktop reset transition: the previous frame is dissolved into the
// already-reset live world through stable pixel/block thresholds, animated
// high-contrast static, sync tearing, and a brief vertical roll.
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;   // frozen old frame
uniform sampler2D liveTexture; // live frame after reset
uniform vec4 colDiffuse;
uniform float uProgress;
uniform float uTime;
uniform vec2 uNativeResolution;

out vec4 finalColor;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash11(float p)
{
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

void main()
{
    vec2 signalUv = fragTexCoord;
    float noiseFrame = floor(uTime * 90.0);

    // Brief vertical-sync loss. Keeping this below a few percent of the frame
    // gives a readable old-TV kick without making frequent restarts nauseating.
    float rollWindow = 1.0 - smoothstep(0.0, 0.30, uProgress);
    float rollJitter = hash11(noiseFrame * 0.37) - 0.5;
    signalUv.y = fract(signalUv.y + rollJitter * 0.065 * rollWindow);

    vec2 pixelCoord = floor(signalUv * uNativeResolution);
    float fineNoise = hash12(pixelCoord);
    float coarseNoise = hash12(floor(pixelCoord / 3.0) + vec2(100.0));
    float threshold = mix(fineNoise, coarseNoise, 0.55);

    const float edge = 0.035;
    // Extending progress beyond 0..1 by the edge width guarantees a completely
    // old first frame and a completely new final frame without a visible pop.
    float dissolveProgress = mix(-edge, 1.0 + edge, uProgress);
    float mixAmount = smoothstep(threshold - edge, threshold + edge, dissolveProgress);
    float signalCross = smoothstep(0.16, 0.38, uProgress) *
                        (1.0 - smoothstep(0.62, 0.86, uProgress));

    // Random native rows briefly lose horizontal lock near the switching edge.
    // The row-coherent offset is what makes this read as a signal fault rather
    // than more per-pixel dissolve noise.
    float boundaryRaw = 1.0 - smoothstep(0.0, edge * 1.35, abs(dissolveProgress - threshold));
    float row = floor(pixelCoord.y);
    float tearSeed = hash11(row * 12.9898 + noiseFrame * 0.71);
    float tearActive = step(0.86, tearSeed) * boundaryRaw;
    float tearOffset = (hash11(row * 3.1 + noiseFrame * 1.3) - 0.5) * 0.040;
    vec2 sampleUv = vec2(fract(signalUv.x + tearOffset * tearActive), signalUv.y);

    // During the short hand-off window, whole scanlines choose between the old
    // and new channel. This breaks up the readable per-pixel dissolve front and
    // creates a brief interlaced two-signal overlap.
    float rowChannel = step(hash11(row * 5.73 + noiseFrame * 0.43), uProgress);
    mixAmount = mix(mixAmount, rowChannel, signalCross * 0.48);

    vec3 oldColor = texture(texture0, sampleUv).rgb;
    vec3 newColor = texture(liveTexture, sampleUv).rgb;
    vec3 color = mix(oldColor, newColor, mixAmount);

    // Posterized pepper-and-salt static: sparse white/color sparks and even
    // sparser black dropouts, rather than the previous continuous gray fog.
    float staticRaw = hash12(pixelCoord + vec2(noiseFrame * 7.13, noiseFrame * 3.71));
    float brightSpeckle = step(0.91, staticRaw);
    float darkSpeckle = step(0.95, 1.0 - staticRaw);
    float staticWindow = smoothstep(0.02, 0.14, uProgress) *
                         (1.0 - smoothstep(0.80, 0.98, uProgress));
    // At mid-transition the noise briefly spreads beyond the dissolve edge,
    // masking the wipe and reading as a channel-switch burst.
    float noiseCoverage = max(boundaryRaw, signalCross * 0.62);
    float boundary = noiseCoverage * staticWindow;

    float colorSeed = hash12(pixelCoord + vec2(noiseFrame * 2.17, 41.0));
    vec3 speckleColor = mix(vec3(0.88, 0.96, 1.0),
                            vec3(1.0, 0.88, 0.96),
                            step(0.5, colorSeed));
    color += speckleColor * brightSpeckle * boundary * 0.82;
    color = mix(color, vec3(0.015), darkSpeckle * boundary * 0.68);

    // A very faint residual wash connects the sparse sparks into a noisy band,
    // but stays weak enough not to return to the old gray-dissolve appearance.
    color = mix(color, vec3(staticRaw), boundary * 0.10);

    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0) * colDiffuse * fragColor;
}
