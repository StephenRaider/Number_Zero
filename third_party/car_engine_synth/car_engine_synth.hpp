// car_engine_synth.hpp  --  header-only procedural inline-4 engine sound synthesiser.
// No raylib dependency, no audio files: everything is generated from maths.
// Profiles: Nissan Silvia S13 (SR20DET turbo) and Honda Civic EK9 (B16B, VTEC).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace carsynth {

constexpr float kTwoPi = 6.28318530718f;

// ============================================================================ profile
// Everything that makes one engine sound different from another lives here.
struct Profile {
    const char* name;
    // --- engine speed range (rpm)
    float idleRpm, limiterRpm;
    // --- combustion pulses (4 cylinders, firing order 1-3-4-2, one pulse per cylinder per cycle)
    float firingGain[4];   // relative strength of each cylinder's pulse (unequal = rough/raspy)
    float pulseMs;         // blowdown pulse length; shorter = crisper/brighter
    float jitter;          // random per-pulse strength variation (0..0.3)
    float idleLump;        // slow rpm wobble at idle (fraction, ~0.01..0.03)
    // --- exhaust pipe resonances (bandpass formants the harmonics sweep through)
    float formantHz[4], formantQ[4], formantGain[4];
    float lowpassClosed, lowpassOpen;  // tail-pipe lowpass (Hz) at light vs full throttle+revs
    float bodyGain;        // low "thump" under the note
    // --- intake / mechanical
    float intakeHz, intakeQ, intakeGain;   // induction roar (noise bandpass)
    float mechGain;        // valvetrain / gear whine hiss at high rpm
    // --- turbo (S13). turbo=false disables everything in this block.
    bool  turbo;
    float spoolStartRpm, spoolFullRpm;      // boost builds between these revs at full throttle
    float spoolUpTime, spoolDownTime;       // turbo shaft inertia (seconds)
    float whistleHz0, whistleHz1, whistleGain;  // compressor whine at low/high shaft speed
    float bovGain;                           // blow-off valve "pssh" loudness (0 = silent)
    float flutterGain;                       // compressor-surge flutter when lifting
    // --- VTEC (EK9). vtec=false disables.
    bool  vtec;
    float vtecRpm, vtecHyst;                // engage above vtecRpm on load, drop below vtecRpm-vtecHyst
    float vtecIntakeBoost;                  // extra induction bark after the cam switch
    float vtecBright;                       // brightening of the exhaust note after the switch
    // --- overrun / limiter
    float crackle;         // overrun pops per second at closed throttle (0 = none)
    float limiterHz;       // fuel-cut chatter rate at the rev limiter
    float muffleHz;        // final 3-pole lowpass (Hz): the overall muffling of the car (lower = duller, bassier)
    float outGain;         // final loudness trim
};

// ---------------------------------------------------------------- Nissan Silvia S13, SR20DET
// 2.0 l turbo inline-4. Muffled turbo exhaust: deep, slightly boomy low-mid note, relatively
// smooth; the character is the compressor whistle, the "pssh" of the blow-off valve and
// the flutter/pops on lift-off. Limiter ~7500 rpm.
inline Profile profileS13() {
    Profile p{};
    p.name = "Nissan Silvia S13 (SR20DET)";
    p.idleRpm = 850; p.limiterRpm = 7500;
    p.firingGain[0] = 1.00f; p.firingGain[1] = 0.88f; p.firingGain[2] = 1.06f; p.firingGain[3] = 0.94f;
    p.pulseMs = 1.25f; p.jitter = 0.07f; p.idleLump = 0.012f;
    const float fh[4] = {95, 210, 520, 1350}, fq[4] = {2.2f, 2.6f, 2.8f, 2.4f}, fg[4] = {1.2f, 1.15f, 0.45f, 0.10f};
    for (int i = 0; i < 4; ++i) { p.formantHz[i] = fh[i]; p.formantQ[i] = fq[i]; p.formantGain[i] = fg[i]; }
    p.lowpassClosed = 450; p.lowpassOpen = 1500;
    p.bodyGain = 1.5f;
    p.intakeHz = 900; p.intakeQ = 0.9f; p.intakeGain = 0.04f;
    p.mechGain = 0.004f;
    p.turbo = true;
    p.spoolStartRpm = 2600; p.spoolFullRpm = 4600; p.spoolUpTime = 0.55f; p.spoolDownTime = 0.9f;
    p.whistleHz0 = 1500; p.whistleHz1 = 3600; p.whistleGain = 0.016f;
    p.bovGain = 0.10f; p.flutterGain = 0.08f;
    p.vtec = false; p.vtecRpm = 99999; p.vtecHyst = 0; p.vtecIntakeBoost = 0; p.vtecBright = 0;
    p.crackle = 0.5f; p.limiterHz = 13;
    p.muffleHz = 1500;
    p.outGain = 1.0f;
    return p;
}

// ---------------------------------------------------------------- Honda Civic EK9, B16B
// 1.6 l naturally-aspirated, 8400 rpm VTEC Type R. Short-runner 4-2-1 header: raspy, bright and
// high-pitched, loud induction. Below ~5700 rpm it is a busy, buzzy little four; at the VTEC
// crossover the high-lift cams come in and it turns hard, bright and angry up to the limiter.
inline Profile profileEK9() {
    Profile p{};
    p.name = "Honda Civic EK9 (B16B VTEC)";
    p.idleRpm = 900; p.limiterRpm = 8500;
    p.firingGain[0] = 1.00f; p.firingGain[1] = 0.82f; p.firingGain[2] = 1.10f; p.firingGain[3] = 0.90f;
    p.pulseMs = 0.75f; p.jitter = 0.10f; p.idleLump = 0.008f;
    const float fh[4] = {160, 420, 1100, 2700}, fq[4] = {2.4f, 3.0f, 3.2f, 3.0f}, fg[4] = {1.1f, 1.15f, 0.55f, 0.15f};
    for (int i = 0; i < 4; ++i) { p.formantHz[i] = fh[i]; p.formantQ[i] = fq[i]; p.formantGain[i] = fg[i]; }
    p.lowpassClosed = 700; p.lowpassOpen = 2200;
    p.bodyGain = 1.3f;
    p.intakeHz = 1100; p.intakeQ = 2.2f; p.intakeGain = 0.03f;
    p.mechGain = 0.006f;
    p.turbo = false;
    p.spoolStartRpm = 0; p.spoolFullRpm = 1; p.spoolUpTime = 1; p.spoolDownTime = 1;
    p.whistleHz0 = 0; p.whistleHz1 = 0; p.whistleGain = 0; p.bovGain = 0; p.flutterGain = 0;
    p.vtec = true; p.vtecRpm = 5700; p.vtecHyst = 300; p.vtecIntakeBoost = 0.6f; p.vtecBright = 0.5f;
    p.crackle = 0.0f; p.limiterHz = 16;
    p.muffleHz = 2100;
    p.outGain = 1.0f;
    return p;
}

// ============================================================================ engine
class Engine {
public:
    explicit Engine(const Profile& p, int sampleRate = 44100, unsigned seed = 12345u)
        : p_(p), sr_(sampleRate), rng_(seed ? seed : 1u) { updateFormants(0, 0); }

    // Call once per audio sample. rpm: engine speed. throttle: 0..1 (accelerator position).
    // Returns one mono sample, roughly -1..1. Cheap: ~100 flops/sample.
    float process(float rpm, float throttle) {
        const float dt = 1.0f / sr_;
        thr_ += (throttle - thr_) * (1 - std::exp(-dt / 0.03f));
        // rpm smoothing (8 ms) so abrupt rpm changes do not click; idle lump adds life
        lump_ += (noise() - lump_) * 0.0006f;
        const float idleness = std::clamp(1.0f - (rpm - p_.idleRpm) / 1500.0f, 0.0f, 1.0f);
        const float rpmT = rpm * (1.0f + (p_.idleLump * idleness + 0.0015f) * lump_ * 4.0f);
        rpmS_ += (rpmT - rpmS_) * (1 - std::exp(-dt / 0.008f));
        const float rev = std::clamp(rpmS_ / p_.limiterRpm, 0.0f, 1.2f);

        // ---- rev limiter: fuel cut chatter
        bool cut = false;
        if (rpmS_ > p_.limiterRpm * 0.985f && thr_ > 0.5f) {
            limClock_ += dt * p_.limiterHz;
            cut = (limClock_ - std::floor(limClock_)) < 0.5f;
        } else limClock_ = 0;

        // ---- VTEC state with hysteresis
        if (p_.vtec) {
            if (!vtecOn_ && rpmS_ > p_.vtecRpm && thr_ > 0.35f) { vtecOn_ = true; vtecKick_ = 1.0f; }
            if (vtecOn_ && (rpmS_ < p_.vtecRpm - p_.vtecHyst || thr_ < 0.15f)) vtecOn_ = false;
            vtecMix_ += ((vtecOn_ ? 1.0f : 0.0f) - vtecMix_) * (1 - std::exp(-dt / 0.035f));
            vtecKick_ *= std::exp(-dt / 0.12f);
        }

        // ---- crank phase: one 4-stroke cycle = 2 crank revs; 4 firings per cycle
        phase_ += (rpmS_ / 120.0f) * dt;
        if (phase_ >= 1.0) phase_ -= 1.0;
        const int idx = std::min(3, (int)(phase_ * 4.0));
        const float load = 0.22f + 0.78f * thr_;
        if (idx != lastIdx_) {
            lastIdx_ = idx;
            const bool overrun = thr_ < 0.06f && rpmS_ > 1500;   // closed throttle: only air pumping
            float amp = p_.firingGain[idx] * (1.0f + p_.jitter * noise()) * load;
            if (overrun) amp *= 0.35f;
            if (cut) amp *= 0.06f;
            const float len = p_.pulseMs * (1.0f - 0.35f * p_.vtecBright * vtecMix_ * (p_.vtec ? 1.0f : 0.0f));
            tauFast_ = len * 1e-3f;
            env1_ += amp;
            env2_ += amp * 0.9f;
            // overrun pops (afterburn in the exhaust): random, only on closed throttle
            if (overrun && p_.crackle > 0 && rev > 0.35f) {
                const float perFire = p_.crackle / std::max(5.0f, rpmS_ / 30.0f);
                if ((noise() * 0.5f + 0.5f) < perFire) { pop_ = 0.5f + 0.5f * (noise() * 0.5f + 0.5f); popHz_ = 70 + 70 * (noise() * 0.5f + 0.5f); }
            }
        }
        env1_ *= std::exp(-dt / tauFast_);
        env2_ *= std::exp(-dt / (tauFast_ * 3.5f));
        float xr = env1_ - 0.6f * env2_;
        xs_ += (xr - xs_) * 0.22f;  // soften the pulse edge
        float x = xs_;  // sharp positive blowdown, then a slow negative scavenge tail

        // ---- exhaust resonances (coefficients refreshed every 32 samples)
        if ((++coefTick_ & 31) == 0) updateFormants(thr_, vtecMix_);
        float y = 0;
        for (int k = 0; k < 4; ++k) {
            const float v3 = x - fz2_[k];
            const float v1 = fa1_[k] * fz1_[k] + fa2_[k] * v3;
            const float v2 = fz2_[k] + fa2_[k] * fz1_[k] + fa3_[k] * v3;
            fz1_[k] = 2 * v1 - fz1_[k];
            fz2_[k] = 2 * v2 - fz2_[k];
            y += v1 * p_.formantGain[k] * (k >= 2 ? 1.0f + p_.vtecBright * vtecMix_ : 1.0f);
        }
        // tail-pipe lowpass opens with throttle*revs
        const float fc = p_.lowpassClosed + (p_.lowpassOpen - p_.lowpassClosed) * thr_ * std::min(1.0f, rev);
        const float a = 1 - std::exp(-kTwoPi * fc * dt);
        lp1_ += (x - lp1_) * a;
        lp2_ += (lp1_ - lp2_) * a;
        // low body thump
        const float kb = 1 - std::exp(-kTwoPi * 110.0f * dt);
        bd1_ += (x - bd1_) * kb;
        bd2_ += (bd1_ - bd2_) * kb;

        float sig = 0.6f * y + 0.6f * lp2_ + p_.bodyGain * (0.4f + 0.8f * thr_) * (1.15f - 0.6f * rev) * bd2_;

        // ---- intake roar: noise through a band-pass, breathing at firing rate, louder with load and revs
        const float n = noise();
        const float breath = 0.55f + 0.45f * std::sin(kTwoPi * (float)phase_ * 4.0f);
        const float ig = p_.intakeGain * (1.0f + p_.vtecIntakeBoost * vtecMix_ + 1.2f * vtecKick_ * (p_.vtec ? 1 : 0));
        const float ih = p_.intakeHz * (1.0f + 0.35f * vtecMix_ + 0.6f * rev);
        const float gI = std::tan(3.14159265f * std::min(ih, 0.45f * sr_) / sr_), rI = 1.0f / p_.intakeQ;
        const float a1 = 1.0f / (1.0f + gI * (gI + rI)), a2 = gI * a1, a3 = gI * a2;
        const float v3 = n - iz2_;
        const float v1 = a1 * iz1_ + a2 * v3;
        const float v2 = iz2_ + a2 * iz1_ + a3 * v3;
        iz1_ = 2 * v1 - iz1_; iz2_ = 2 * v2 - iz2_;
        sig += v1 * ig * thr_ * rev * rev * (1.8f + breath) * 1.5f;

        // ---- valvetrain / mechanical hiss (highpassed noise growing with rev^2)
        hp_ += (n - hp_) * 0.25f;
        sig += (n - hp_) * p_.mechGain * rev * rev * (0.3f + 0.7f * thr_);

        // ---- overrun pop: low thump + crack, decaying
        float popSig = 0;
        if (pop_ > 0.001f) {
            popPh_ += kTwoPi * popHz_ * dt;
            popSig = pop_ * std::sin(popPh_) * 0.6f;
            pop_ *= std::exp(-dt / 0.05f);
        }
        sig += popSig * 0.5f;

        // ---- saturation: dirtier the harder it is driven (asymmetric -> even harmonics)
        const float level = (0.28f + 0.72f * thr_) * (0.45f + 0.55f * std::min(1.0f, rev * 1.15f));
        const float k = 1.2f + 1.8f * thr_;
        float s = (std::tanh(k * sig * 1.6f * level + 0.3f) - std::tanh(0.3f)) / k;
        dcOut_ = s - dcIn_ + 0.9985f * dcOut_;
        dcIn_ = s;
        s = dcOut_ * 1.15f;

        // ---- overall muffling: 3 cascaded one-pole lowpasses (the car as heard through the bodywork)
        const float km = 1 - std::exp(-kTwoPi * (p_.muffleHz * (0.8f + 0.4f * thr_)) * dt);
        m1_ += (s - m1_) * km; m2_ += (m1_ - m2_) * km; m3_ += (m2_ - m3_) * km;
        s = m3_ * 2.0f;
        // ---- turbo / BOV (after saturation; softened by its own lowpass so it never turns to hiss)
        if (p_.turbo) { t1_ += (turboLayer(dt, n, rev) - t1_) * 0.28f; s += t1_; }

        // pre-filtered output ceiling
        return std::tanh(s * p_.outGain * 1.1f) * 0.85f;
    }

    // Convenience: stereo duplicate with a tiny decorrelation (optional).
    void processStereo(float rpm, float throttle, float& l, float& r) { l = r = process(rpm, throttle); }

    bool vtecActive() const { return vtecOn_; }
    float boost() const { return shaft_; }
    const Profile& profile() const { return p_; }

private:
    float turboLayer(float dt, float n, float rev) {
        // boost target rises with revs between spoolStart/spoolFull and with throttle
        const float spool = std::clamp((rpmS_ - p_.spoolStartRpm) / (p_.spoolFullRpm - p_.spoolStartRpm), 0.0f, 1.0f);
        const float target = spool * std::pow(std::clamp(thr_, 0.0f, 1.0f), 1.3f);
        const float tau = target > shaft_ ? p_.spoolUpTime : p_.spoolDownTime;
        const float prevShaft = shaft_;
        shaft_ += (target - shaft_) * (1 - std::exp(-dt / tau));
        // throttle slammed shut with pressure in the intake -> blow-off / flutter
        if (lastThr_ >= 0.35f && thr_ < 0.35f && shaft_ > 0.22f && bov_ < 0.05f) {
            bov_ = shaft_ * p_.bovGain;
            flutter_ = shaft_ * p_.flutterGain;
        }
        lastThr_ = thr_;
        (void)prevShaft;
        // compressor whistle: sine (plus 2nd harmonic) at a frequency that tracks shaft speed
        const float sp = std::pow(std::clamp(shaft_, 0.0f, 1.0f), 0.8f);
        const float wf = p_.whistleHz0 + (p_.whistleHz1 - p_.whistleHz0) * sp;
        wPh_ += kTwoPi * wf * dt;
        if (wPh_ > kTwoPi) wPh_ -= kTwoPi;
        wMod_ += (noise() - wMod_) * 0.002f;
        float w = std::sin(wPh_) + 0.28f * std::sin(2 * wPh_ + 0.7f) + 0.10f * std::sin(3 * wPh_);
        w *= p_.whistleGain * shaft_ * shaft_ * (0.8f + 0.5f * wMod_ * 4.0f) * (0.4f + 0.6f * std::min(1.0f, rev * 1.3f));
        // faint rotor whoosh (broadband) while boosting
        wHp_ += (n - wHp_) * 0.12f;
        w += (n - wHp_) * 0.002f * shaft_ * shaft_;
        // BOV: bandpass-ish noise burst, falling in pitch; flutter: AM chatter on the whistle
        float b = 0;
        if (bov_ > 0.002f) {
            bovLp_ += (n - bovLp_) * 0.07f;
            bovHp_ += (bovLp_ - bovHp_) * 0.03f;
            b = (bovLp_ - bovHp_) * bov_ * 1.5f;
            bov_ *= std::exp(-dt / 0.22f);
        }
        if (flutter_ > 0.002f) {
            flPh_ += kTwoPi * (11.0f + 14.0f * flutter_) * dt;
            const float gate = std::sin(flPh_) > 0.2f ? 1.0f : 0.0f;
            w += gate * flutter_ * 0.10f * (std::sin(wPh_ * 1.0f) + 0.4f * n);
            flutter_ *= std::exp(-dt / 0.5f);
        }
        return w + b;
    }

    void updateFormants(float thr, float vmix) {
        for (int k = 0; k < 4; ++k) {
            // hot gas = faster sound = formants shift up with load
            float hz = p_.formantHz[k] * (1.0f + 0.10f * thr + 0.12f * vmix * p_.vtecBright);
            hz = std::min(hz, 0.45f * sr_);
            const float g = std::tan(3.14159265f * hz / sr_), r = 1.0f / p_.formantQ[k];
            fa1_[k] = 1.0f / (1.0f + g * (g + r));
            fa2_[k] = g * fa1_[k];
            fa3_[k] = g * fa2_[k];
        }
    }
    float noise() {  // xorshift32 white noise, -1..1
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return (rng_ >> 8) / 8388608.0f - 1.0f;
    }

    Profile p_;
    int sr_;
    unsigned rng_;
    double phase_ = 0;
    int lastIdx_ = -1, coefTick_ = 0;
    float thr_ = 0, rpmS_ = 0, lump_ = 0, limClock_ = 0, lastThr_ = 0;
    float env1_ = 0, env2_ = 0, tauFast_ = 0.001f;
    float fz1_[4] = {}, fz2_[4] = {}, fa1_[4] = {}, fa2_[4] = {}, fa3_[4] = {};
    float xs_ = 0, m1_ = 0, m2_ = 0, m3_ = 0, t1_ = 0, lp1_ = 0, lp2_ = 0, bd1_ = 0, bd2_ = 0, iz1_ = 0, iz2_ = 0, hp_ = 0;
    float dcIn_ = 0, dcOut_ = 0;
    float pop_ = 0, popHz_ = 200, popPh_ = 0;
    bool vtecOn_ = false;
    float vtecMix_ = 0, vtecKick_ = 0;
    float shaft_ = 0, wPh_ = 0, wMod_ = 0, wHp_ = 0, bov_ = 0, bovLp_ = 0, bovHp_ = 0, flutter_ = 0, flPh_ = 0;
};

// ============================================================================ WAV helper (offline)
inline bool writeWavMono(const char* path, const std::vector<float>& mono, int rate) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const uint32_t bytes = (uint32_t)(mono.size() * 2);
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + bytes); std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    std::fwrite("data", 1, 4, f); u32(bytes);
    for (float s : mono) { int16_t v = (int16_t)std::lround(std::clamp(s, -1.0f, 1.0f) * 32767.0f); std::fwrite(&v, 2, 1, f); }
    return std::fclose(f) == 0;
}

}  // namespace carsynth
