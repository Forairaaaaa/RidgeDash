#version 330

// CRT / old-TV post-process for RidgeDash, applied to the final upscaled frame as
// it is blitted to the window. A single-pass distillation of the two details that
// give the "guest" CRT shaders their fine look:
//   - an aperture-grille phosphor mask keyed to physical output pixels (the crisp
//     RGB-stripe "pixelation"), and
//   - beam-shaped scanlines (bright pixels bloom into a fuller beam, dark pixels
//     stay as thin lines) instead of a flat sine darkening.
// Plus barrel curvature, chromatic aberration, and a vignette. UV past the curved
// edge renders black, which reads as the CRT bezel.
//
// This is NOT the full guest pipeline (no NTSC signal, LUT, halation/bloom, or
// multi-pass) — just the perceptual essentials in one fragment shader.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 uResolution; // destination size in pixels
uniform float uTime;      // seconds, for subtle animated flicker

out vec4 finalColor;

// --- tunables ---------------------------------------------------------------
const float CURVATURE      = 0.10;   // barrel distortion amount (0 = flat)
const float ABERRATION     = 0.0016; // R/B channel UV split at the edges
const float MASK_SIZE      = 3.0;    // output pixels per RGB triad (smaller = finer)
const float MASK_DARK      = 0.60;   // brightness of the two unlit subpixels (0..1)
const float SCANLINE_MIN   = 0.32;   // beam half-width for dark pixels (thin line)
const float SCANLINE_MAX   = 0.62;   // beam half-width for bright pixels (fuller)
const float SCANLINE_DEPTH = 0.50;   // darkness between scanlines (0 = none)
const float VIGNETTE       = 0.26;   // corner darkening strength
const float FLICKER        = 0.025;  // subtle brightness flicker amount
const float FINAL_GAIN     = 1.50;   // compensate for mask/scanline darkening
const float NATIVE_HEIGHT  = 170.0;  // logical rows (RIDGEDASH_SCREEN_HEIGHT)

// Barrel-distort UV around the center.
vec2 curve(vec2 uv)
{
    uv = uv * 2.0 - 1.0;            // -1..1
    vec2 offset = abs(uv.yx) / vec2(1.0 / CURVATURE + 1.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;         // back to 0..1
}

// Aperture grille: vertical RGB stripes tied to physical output columns. Each
// triad lights one channel fully and dims the other two — the phosphor look.
vec3 apertureGrille(float px)
{
    float f = fract(px / MASK_SIZE); // 0..1 across one RGB triad
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

    // Outside the curved screen -> bezel (black).
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Chromatic aberration: split R/B outward from center.
    vec2 dir = uv - 0.5;
    float amt = ABERRATION * dot(dir, dir) * 4.0;
    vec3 col = vec3(texture(texture0, uv + dir * amt).r,
                    texture(texture0, uv).g,
                    texture(texture0, uv - dir * amt).b);

    // Beam-shaped scanlines: brighter pixels get a wider (fuller) beam, dark ones
    // stay a thin line. Gaussian falloff across each native row.
    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    float pos = fract(uv.y * NATIVE_HEIGHT) - 0.5;
    float w = mix(SCANLINE_MIN, SCANLINE_MAX, lum);
    float beam = exp(-(pos * pos) / (2.0 * w * w));
    col *= mix(1.0 - SCANLINE_DEPTH, 1.0, beam);

    // Aperture-grille phosphor mask on physical output pixels.
    col *= apertureGrille(gl_FragCoord.x);

    // Vignette: darken toward the corners.
    float vig = 1.0 - VIGNETTE * dot(dir, dir) * 2.2;
    col *= clamp(vig, 0.0, 1.0);

    // Subtle flicker + brightness compensation.
    col *= 1.0 - FLICKER * (0.5 + 0.5 * sin(uTime * 12.0));
    col *= FINAL_GAIN;

    finalColor = vec4(col, 1.0) * colDiffuse * fragColor;
}
