#version 300 es

// CRT / old-TV post-process for RidgeDash — WebGL2 / GLSL ES 3.00 version.
// Functionally identical to crt.fs (desktop GLSL 330); the only differences
// are the version directive and the required precision qualifier.

precision highp float;

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 uResolution; // destination size in pixels
uniform float uTime;      // seconds, for subtle animated flicker

out vec4 finalColor;

// --- tunables ---------------------------------------------------------------
const float CURVATURE      = 0.10;
const float ABERRATION     = 0.0016;
const float MASK_SIZE      = 3.0;
const float MASK_DARK      = 0.82;   // brightness of unlit subpixels; higher =
                                     // subtler grille (canvas is CSS-upscaled so
                                     // MASK_DARK=0.60 looks too coarse on web)
const float SCANLINE_MIN   = 0.32;
const float SCANLINE_MAX   = 0.62;
const float SCANLINE_DEPTH = 0.45;   // slightly reduced vs desktop (0.50) because
                                     // each native scanline spans more display pixels
const float VIGNETTE       = 0.26;
const float EDGE_FEATHER   = 0.015;
const float FLICKER        = 0.025;
const float FINAL_GAIN     = 1.50;
const float NATIVE_HEIGHT  = 170.0;

vec2 curve(vec2 uv)
{
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(1.0 / CURVATURE + 1.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;
}

vec3 apertureGrille(float px)
{
    float f = fract(px / MASK_SIZE);
    vec3 m = vec3(MASK_DARK);
    if (f < 1.0 / 3.0) {
        m.r = 1.0;
    } else if (f < 2.0 / 3.0) {
        m.g = 1.0;
    } else {
        m.b = 1.0;
    }
    return m;
}

void main()
{
    vec2 uv = curve(fragTexCoord);

    vec2 edge = smoothstep(vec2(0.0), vec2(EDGE_FEATHER), uv) *
                smoothstep(vec2(0.0), vec2(EDGE_FEATHER), 1.0 - uv);
    float edgeMask = edge.x * edge.y;
    if (edgeMask <= 0.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 dir = uv - 0.5;
    float amt = ABERRATION * dot(dir, dir) * 4.0;
    vec3 col = vec3(texture(texture0, uv + dir * amt).r,
                    texture(texture0, uv).g,
                    texture(texture0, uv - dir * amt).b);

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    float pos = fract(uv.y * NATIVE_HEIGHT) - 0.5;
    float w = mix(SCANLINE_MIN, SCANLINE_MAX, lum);
    float beam = exp(-(pos * pos) / (2.0 * w * w));
    col *= mix(1.0 - SCANLINE_DEPTH, 1.0, beam);

    // Aperture-grille keyed to integer canvas pixels (gl_FragCoord.x) for a
    // clean, moiré-free RGB triad.  MASK_SIZE=3 → one triad per 3 canvas pixels;
    // MASK_DARK is raised to 0.82 so the grille is subtle at the CSS-upscaled size.
    col *= apertureGrille(gl_FragCoord.x);

    float vig = 1.0 - VIGNETTE * dot(dir, dir) * 2.2;
    col *= clamp(vig, 0.0, 1.0);

    col *= 1.0 - FLICKER * (0.5 + 0.5 * sin(uTime * 12.0));
    col *= FINAL_GAIN;

    col *= edgeMask;

    finalColor = vec4(col, 1.0) * colDiffuse * fragColor;
}
