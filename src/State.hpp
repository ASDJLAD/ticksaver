#pragma once

#include <Geode/Geode.hpp>

namespace ticksaver {

// Allowed tick-save values. F6 cycles through them.
inline constexpr int kSteps[] = {0, 1, 2, 4, 8};
inline constexpr int kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

struct State {
	int lastUsed = 0;        // ticks used by the most recent save
	int attemptUsed = 0;     // total ticks used in this attempt
	int attemptSaves = 0;    // number of saves in this attempt
	int totalSaves = 0;      // number of saves since the game started
	std::string lastKind = "-";

	void resetAttempt() {
		lastUsed = 0;
		attemptUsed = 0;
		attemptSaves = 0;
		lastKind = "-";
	}

	void record(int ticks, char const* kind) {
		lastUsed = ticks;
		attemptUsed += ticks;
		++attemptSaves;
		++totalSaves;
		lastKind = kind;
	}
};

inline State& state() {
	static State s;
	return s;
}

inline int tickSave() {
	int v = geode::Mod::get()->getSavedValue<int>("tick-save", 2);
	for (int s : kSteps) if (s == v) return v;
	return 2;
}

inline void setTickSave(int v) {
	geode::Mod::get()->setSavedValue<int>("tick-save", v);
}

// Move to the next value in kSteps, wrapping around. Returns the new value.
inline int cycleTickSave() {
	int cur = tickSave();
	int idx = 0;
	for (int i = 0; i < kStepCount; ++i) if (kSteps[i] == cur) idx = i;
	int next = kSteps[(idx + 1) % kStepCount];
	setTickSave(next);
	return next;
}

} // namespace ticksaver
