#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

// Helper: 4x4 = 16 Subpixel Super-Sampling Coverage calculation for Circles
float calculateCircleCoverage(vec2 p, vec2 dPdx, vec2 dPdy, float maxR) {
    float cov = 0.0;
    for (int j = 0; j < 4; j++) {
        float sy = (float(j) + 0.5) * 0.25 - 0.5;
        for (int i = 0; i < 4; i++) {
            float sx = (float(i) + 0.5) * 0.25 - 0.5;
            vec2 subP = p + sx * dPdx + sy * dPdy;
            if (length(subP) <= maxR) {
                cov += 0.0625;
            }
        }
    }
    return cov;
}

// 2D RoundRect SDF
float sdRoundRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - (b - vec2(r));
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float calculateRoundRectCoverage(vec2 p, vec2 dPdx, vec2 dPdy, vec2 b, float r) {
    float cov = 0.0;
    for (int j = 0; j < 4; j++) {
        float sy = (float(j) + 0.5) * 0.25 - 0.5;
        for (int i = 0; i < 4; i++) {
            float sx = (float(i) + 0.5) * 0.25 - 0.5;
            vec2 subP = p + sx * dPdx + sy * dPdy;
            if (sdRoundRect(subP, b, r) <= 0.0) {
                cov += 0.0625;
            }
        }
    }
    return cov;
}

float calculateRoundRectStrokeCoverage(vec2 p, vec2 dPdx, vec2 dPdy, vec2 b, float r, float strokePx) {
    float covOuter = calculateRoundRectCoverage(p, dPdx, dPdy, b, r);
    vec2 bInner = b - vec2(strokePx);
    float rInner = max(0.0, r - strokePx);
    float covInner = calculateRoundRectCoverage(p, dPdx, dPdy, bInner, rInner);
    return clamp(covOuter - covInner, 0.0, 1.0);
}


float calculateStrokeCoverage(vec2 p, vec2 dPdx, vec2 dPdy, float targetR, float halfStroke) {
    float cov = 0.0;
    for (int j = 0; j < 4; j++) {
        float sy = (float(j) + 0.5) * 0.25 - 0.5;
        for (int i = 0; i < 4; i++) {
            float sx = (float(i) + 0.5) * 0.25 - 0.5;
            vec2 subP = p + sx * dPdx + sy * dPdy;
            float r = length(subP);
            if (abs(r - targetR) <= halfStroke) {
                cov += 0.0625;
            }
        }
    }
    return cov;
}

void main() {
    float u = fragTexCoord.x;
    float v = fragTexCoord.y;

    // Mode -100: Solid Rectangle
    if (u < -80.0) {
        outColor = fragColor;
        return;
    }

    // Mode -70: GPU Parametric Bézier Curve (Loop-Blinn) for Marlin Vector Pipeline
    if (u < -65.0) {
        vec2 p = vec2(u + 70.0, v);
        float f = p.x * p.x - p.y;
        vec2 grad = vec2(dFdx(f), dFdy(f));
        float gLen = length(grad);
        if (gLen > 0.0) {
            float dist = f / gLen;
            float alpha = clamp(0.5 - dist, 0.0, 1.0);
            if (alpha <= 0.0) discard;
            outColor = vec4(fragColor.rgb, fragColor.a * alpha);
        } else {
            if (f > 0.0) discard;
            outColor = fragColor;
        }
        return;
    }

    // Mode -60: Oval Fill WITHOUT AA (u in [-61, -59])
    if (u < -55.0) {
        vec2 p = vec2(u + 60.0, v);
        if (length(p) > 1.0) discard;
        outColor = fragColor;
        return;
    }

    // Mode -50: Oval Outline WITHOUT AA (u in [-51, -49])
    if (u < -45.0) {
        vec2 p = vec2(u + 50.0, v);
        vec2 dPdx = dFdx(p);
        vec2 dPdy = dFdy(p);
        float pxSize = max(length(dPdx), length(dPdy));
        if (abs(length(p) - 1.0) > 0.5 * pxSize) discard;
        outColor = fragColor;
        return;
    }

    // Mode -30: Oval Fill WITH 16x Subpixel AA (u in [-31, -29])
    if (u < -25.0) {
        vec2 p = vec2(u + 30.0, v);
        vec2 dPdx = dFdx(p);
        vec2 dPdy = dFdy(p);
        float alpha = calculateCircleCoverage(p, dPdx, dPdy, 1.0);
        if (alpha <= 0.0) discard;
        outColor = vec4(fragColor.rgb, fragColor.a * alpha);
        return;
    }

    // Mode -20: RoundRectangle Fill WITH 16x Subpixel AA (u in [-21, -19])
    if (u < -15.0) {
        vec2 p = vec2(u + 20.0, v);
        vec2 dPdx = dFdx(p);
        vec2 dPdy = dFdy(p);
        float alpha = calculateRoundRectCoverage(p, dPdx, dPdy, vec2(1.0, 1.0), 0.35);
        if (alpha <= 0.0) discard;
        outColor = vec4(fragColor.rgb, fragColor.a * alpha);
        return;
    }

    // Mode -10: RoundRectangle Outline WITH 16x Subpixel AA (u in [-11, -9])
    if (u < -5.0) {
        vec2 p = vec2(u + 10.0, v);
        vec2 dPdx = dFdx(p);
        vec2 dPdy = dFdy(p);
        float pxSize = max(length(dPdx), length(dPdy));
        // Stroke width of 1 physical pixel in normalized coordinates = pxSize
        // Calibrated corner radius: 0.368 matches Java2D arc center on expanded w+1 quad
        float alpha = calculateRoundRectStrokeCoverage(p, dPdx, dPdy, vec2(1.0, 1.0), 0.368, pxSize);
        if (alpha <= 0.0) discard;
        outColor = vec4(fragColor.rgb, fragColor.a * alpha);
        return;
    }

    // Mode -2: Oval Outline WITH 16x Subpixel AA (u in [-3, -1])
    if (u < -0.5) {
        vec2 p = vec2(u + 2.0, v);
        vec2 dPdx = dFdx(p);
        vec2 dPdy = dFdy(p);
        float pxSize = max(length(dPdx), length(dPdy));
        float halfStroke = 0.5 * pxSize;
        float alpha = calculateStrokeCoverage(p, dPdx, dPdy, 1.0, halfStroke);
        if (alpha <= 0.0) discard;
        outColor = vec4(fragColor.rgb, fragColor.a * alpha);
        return;
    }

    // Standard Texture Mode: u >= 0.0
    outColor = texture(texSampler, fragTexCoord) * fragColor;
}
