#pragma once
#include <cmath>

constexpr double EPS = 1E-10;

inline int compute_last_index(double t, double T, int N) {
    double dt = T / N;
    int nearest = std::round(t / dt);
    if (std::fabs(nearest * dt - t) < EPS) return nearest;
    return int(t / dt);
}