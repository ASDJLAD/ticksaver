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
	int seen = 0;            // presses the mod saw this attempt
	int buffered = 0;        // presses that could not be used and were held
	int expired = 0;         // held presses that ran out of time
	int totalSaves = 0;      // number of saves since the game started
	std::string lastKind = "-";

	void resetAttempt() {
		lastUsed = 0;
		attemptUsed = 0;
		attemptSaves = 0;
		seen = 0;
		buffered = 0;
		expired = 0;
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

// Ticks per second used to turn the tick save into time.
// 60 = one tick is one frame of a 60 FPS recording. 240 = one physics step at the default 240 TPS.
inline double tickSeconds() {
	int rate = static_cast<int>(geode::Mod::get()->getSettingValue<int64_t>("tick-rate"));
	if (rate < 1) rate = 60;
	return 1.0 / static_cast<double>(rate);
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
