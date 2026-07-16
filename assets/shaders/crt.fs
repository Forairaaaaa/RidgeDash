#version 330

// CRT / old-TV post-process for RidgeDash, applied to the final upscaled frame as
// it is blitted to the window. A single-pass distillation of the two details that
// give the "guest" CRT shaders their fine look:
//   - an aperture-grille phosphor mask keyed to physical output pixels (the crisp
//     RGB-stripe "pixelation"), resolution-adaptive so triad density stays
//     visually consistent across window sizes/DPI, and
//   - beam-shaped scanlines (bright pixels bloom into a fuller beam, dark pixels
//     stay as thin lines) instead of a flat sine darkening.
// Plus barrel curvature, chromatic aberration, and a vignette. UV past the curved
// edge renders black, which reads as the CRT bezel.
//
// Bloom is supplied by a separate desktop multi-pass downsample/upsample chain.
// Keeping that non-local effect outside this pass gives it a genuinely broad,
// soft radius without a large per-fragment sampling kernel here.
//
// Changelog vs. the previous version:
//   - uResolution now actually used: aperture-grille triad size scales with
//     output resolution so it doesn't get comically fine/coarse at odd window sizes.
//   - Aperture grille edges are smoothed (no more hard fract() cutoffs) to kill
//     moiré/aliasing when the window width isn't a multiple of MASK_SIZE.
//   - Chromatic-aberration sample UVs are clamped so we never sample outside
//     [0,1] near the curved edge (previously depended on implicit texture wrap).
//   - Mask + scanline attenuation now happens in linear light (gamma-correct),
//     then FINAL_GAIN is applied before converting back to sRGB. Fixes blown
//     highlights / muddy shadows that a flat post-hoc multiply produced.
//   - Scanline row position is computed from the PRE-curvature (fragTexCoord)
//     y-coordinate, so scanlines stay horizontal and evenly spaced instead of
//     bowing with the barrel distortion.
//   - Flicker is now two incommensurate sine waves instead of one, so it reads
//     as organic instead of a metronomic pulse.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D bloomTexture;
uniform vec4 colDiffuse;

uniform vec2 uResolution; // destination size in pixels
uniform float uTime;      // seconds, for subtle animated flicker
uniform float uBloomIntensity;

out vec4 finalColor;

// --- tunables ---------------------------------------------------------------
const float CURVATURE      = 0.10;   // barrel distortion amount (0 = flat)
const float ABERRATION     = 0.0016; // R/B channel UV split at the edges
const float MASK_SIZE      = 3.0;    // output pixels per RGB triad at reference height
const float MASK_REF_HEIGHT= 720.0;  // output height MASK_SIZE was tuned at
const float MASK_DARK      = 0.80;   // brightness of the two unlit subpixels (0..1) — higher = subtler stripes
const float MASK_SOFTNESS  = 0.40;   // edge feather width within a triad (0..1) — higher = softer, less noisy on flat color
const float MASK_STRENGTH  = 0.45;   // 0 = no vertical grille at all, 1 = full effect; independent overall dial
const float SCANLINE_MIN   = 0.20;   // beam half-width for dark pixels (thin line) — lower = crisper horizontal lines
const float SCANLINE_MAX   = 0.62;   // beam half-width for bright pixels (fuller)
const float SCANLINE_DEPTH = 0.65;   // darkness between scanlines (0 = none) — higher = more prominent horizontal read
const float VIGNETTE       = 0.26;   // corner darkening strength
const float EDGE_FEATHER   = 0.015;  // soft fade width at the curved screen edge (UV)
const float FLICKER        = 0.025;  // subtle brightness flicker amount
const float FINAL_GAIN     = 1.50;   // compensate for mask/scanline darkening (linear space)
const float NATIVE_HEIGHT  = 170.0;  // logical rows (RIDGEDASH_SCREEN_HEIGHT)
const float GAMMA          = 2.2;    // sRGB<->linear approximation

// Barrel-distort UV around the center.
vec2 curve(vec2 uv)
{
    uv = uv * 2.0 - 1.0;            // -1..1
    vec2 offset = abs(uv.yx) / vec2(1.0 / CURVATURE + 1.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;         // back to 0..1
}

// Aperture grille: vertical RGB stripes tied to physical output columns, with
// resolution-adaptive triad width. Built from three cosine lobes at 120-degree
// phase offsets — periodic by construction, so there's no fract()-seam to
// special-case and no moiré from a hard cutoff at non-integer scales.
// MASK_SOFTNESS controls the lobe sharpness (higher power = narrower/harder stripe).
vec3 apertureGrille(float px, float maskScale)
{
    float triad = MASK_SIZE * maskScale;
    float phase = (px / triad) * 6.28318530718; // 2*pi across one triad

    float sharpness = mix(1.0, 6.0, clamp(1.0 - MASK_SOFTNESS, 0.0, 1.0));
    float onR = pow(max(0.5 + 0.5 * cos(phase), 0.0), sharpness);
    float onG = pow(max(0.5 + 0.5 * cos(phase - 2.09439510239), 0.0), sharpness); // -120deg
    float onB = pow(max(0.5 + 0.5 * cos(phase - 4.18879020479), 0.0), sharpness); // -240deg

    vec3 m;
    m.r = mix(MASK_DARK, 1.0, onR);
    m.g = mix(MASK_DARK, 1.0, onG);
    m.b = mix(MASK_DARK, 1.0, onB);
    return m;
}

void main()
{
    vec2 uv = curve(fragTexCoord);

    // Soft feathered edge instead of a hard cutoff: fade to black over EDGE_FEATHER
    // just inside each curved screen edge, so the four corners melt out smoothly.
    vec2 edge = smoothstep(vec2(0.0), vec2(EDGE_FEATHER), uv) *
                smoothstep(vec2(0.0), vec2(EDGE_FEATHER), 1.0 - uv);
    float edgeMask = edge.x * edge.y;
    if (edgeMask <= 0.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Chromatic aberration: split R/B outward from center. Clamp sample UVs so
    // we never read outside [0,1] near the curved edge regardless of the
    // texture's wrap mode (avoids edge smearing/repeat artifacts).
    vec2 dir = uv - 0.5;
    float amt = ABERRATION * dot(dir, dir) * 4.0;
    vec2 uvR = clamp(uv + dir * amt, 0.0, 1.0);
    vec2 uvB = clamp(uv - dir * amt, 0.0, 1.0);
    vec3 col = vec3(texture(texture0, uvR).r,
                    texture(texture0, uv).g,
                    texture(texture0, uvB).b);

    // Work in linear light for the mask/scanline multiplies so highlights and
    // shadows attenuate correctly instead of a flat sRGB-space multiply.
    col = pow(max(col, 0.0), vec3(GAMMA));

    // The bloom texture was built from the uncurved scene at several scales.
    // Sample it with the curved UV so its glow remains registered to the glass,
    // then add in linear light before scanlines and the grille shape it. The
    // Reinhard shoulder reins in the hot center while preserving dim halo detail.
    vec3 bloom = pow(max(texture(bloomTexture, clamp(uv, 0.0, 1.0)).rgb, 0.0), vec3(GAMMA));
    bloom = bloom / (vec3(1.0) + bloom);
    col += bloom * uBloomIntensity;

    // Beam-shaped scanlines: brighter pixels get a wider (fuller) beam, dark ones
    // stay a thin line. Gaussian falloff across each native row. Row position is
    // taken from the pre-curvature texcoord so scanlines stay flat/horizontal
    // rather than bowing with the barrel distortion.
    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    float pos = fract(fragTexCoord.y * NATIVE_HEIGHT) - 0.5;
    float w = mix(SCANLINE_MIN, SCANLINE_MAX, clamp(lum, 0.0, 1.0));
    float beam = exp(-(pos * pos) / (2.0 * w * w));
    col *= mix(1.0 - SCANLINE_DEPTH, 1.0, beam);

    // Aperture-grille phosphor mask on physical output pixels, scaled so triad
    // density looks consistent across different output resolutions. MASK_STRENGTH
    // blends it against "no mask" so overall vertical-stripe visibility can be
    // tuned independently of per-stripe contrast/softness — useful for pixel-art
    // content with large flat color blocks, where a strong grille reads as noise.
    float maskScale = clamp(uResolution.y / MASK_REF_HEIGHT, 0.5, 4.0);
    vec3 mask = apertureGrille(gl_FragCoord.x, maskScale);
    col *= mix(vec3(1.0), mask, MASK_STRENGTH);

    // Vignette: darken toward the corners.
    float vig = 1.0 - VIGNETTE * dot(dir, dir) * 2.2;
    col *= clamp(vig, 0.0, 1.0);

    // Subtle flicker: two incommensurate sine waves so it doesn't read as a
    // perfectly periodic pulse.
    float flick = 0.5 + 0.25 * sin(uTime * 12.0) + 0.25 * sin(uTime * 7.3 + 1.7);
    col *= 1.0 - FLICKER * flick;

    // Compensate for mask/scanline darkening, then back to sRGB.
    col *= FINAL_GAIN;
    col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / GAMMA));

    // Feather the curved screen edge into the bezel.
    col *= edgeMask;

    finalColor = vec4(col, 1.0) * colDiffuse * fragColor;
}
