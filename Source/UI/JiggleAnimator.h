#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace tits {
class JiggleAnimator {
  public:
    struct Spring {
        double x = 0, v = 0, secondary = 0;
    };
    std::array<Spring, 2> springs;
    double phase = 0, lastMacro = 0;
    void step(double elapsed, float macro, float amount, bool reduced) {
        double dt = std::clamp(elapsed, 0., .05);
        double intensity = std::pow(std::clamp(double(macro), 0., 1.), 1.4) *
                           (1 + 2.6 * std::pow(std::clamp(double(macro), 0., 1.), 3)) * amount *
                           (reduced ? .15 : 1);
        double impulse =
            std::clamp((macro - lastMacro) * .28, -.07, .07) * amount * (reduced ? .15 : 1);
        lastMacro = macro;
        int steps = std::max(1, int(std::ceil(dt / .004)));
        double h = dt / steps;
        for (int k = 0; k < steps; ++k) {
            phase += h * (1.05 + 2.2 * macro) * 6.283185307;
            phase = std::fmod(phase, 6.283185307);
            for (int i = 0; i < 2; ++i) {
                auto &s = springs[i];
                if (k == 0)
                    s.v += impulse * (i == 0 ? 1 : .88);
                double target = .03 * intensity * std::sin(phase + i * .65);
                double acceleration = 150 * (target - s.x) - 15 * s.v;
                s.v = std::clamp(s.v + acceleration * h, -1.2, 1.2);
                s.x = std::clamp(s.x + s.v * h, -.085, .085);
                s.secondary += (s.x - s.secondary) * h * 12;
            }
        }
        if (amount <= 0)
            for (auto &s : springs)
                s = {};
    }
};
} // namespace tits
