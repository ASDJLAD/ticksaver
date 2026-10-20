#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "Hud.hpp"
#include "State.hpp"

using namespace geode::prelude;
using namespace ticksaver;

// Tick Saver
//
// A "tick" is one physics step: GJGameState::m_currentProgress goes up by one per step.
// With a tick save of N:
//   * Early press: a press made while it cannot be used (in the air, not on an orb) is held.
//     If the player lands or touches an orb within N ticks, the press is applied then.
//   * Extended hold (optional): a release is delayed by N ticks, and cancelled if the
//     button is pressed again inside that window.
// A press that comes AFTER the moment it was needed cannot be saved, because that would
// need the game to rewind its state, which this mod does not do.

namespace {
	bool enabled() { return Mod::get()->getSettingValue<bool>("enabled") && tickSave() > 0; }
	bool bufferPress() { return Mod::get()->getSettingValue<bool>("buffer-press"); }
	bool bufferOrbs() { return Mod::get()->getSettingValue<bool>("buffer-orbs"); }
	bool bufferGround() { return Mod::get()->getSettingValue<bool>("buffer-ground"); }
	bool extendHold() { return Mod::get()->getSettingValue<bool>("extend-hold"); }

	// Cube, ball, robot and spider jump from the ground. Ship, UFO, wave and swing do not.
	bool groundBufferable(PlayerObject* p) {
		return !(p->m_isShip || p->m_isBird || p->m_isDart || p->m_isSwing);
	}

	// Can a press do something right now?
	bool pressUsableNow(PlayerObject* p) {
		if (!p->m_touchedRings.empty()) return true;
		if (!groundBufferable(p)) return true; // flying modes react to the button at any time
		return p->m_isOnGround;
	}
}

// F6 cycles the tick save: off, 1, 2, 4, 8.
class $modify(TickKeys, CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
		if (down && !repeat && key == KEY_F6) {
			int v = cycleTickSave();
			std::string msg = v == 0 ? std::string("Tick save: OFF") : fmt::format("Tick save: {} ticks", v);
			Notification::create(msg.c_str(), NotificationIcon::None, 1.0f)->show();
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
	}
};

class $modify(TickGameLayer, GJBaseGameLayer) {
	struct Slot {
		bool physHeld = false;       // is the physical button held right now?
		bool pending = false;        // an early press is waiting to become usable
		unsigned pressTick = 0;
		bool relPending = false;     // a release is being delayed
		unsigned relTick = 0;
		bool injectedPress = false;  // we pressed for the player, release it next tick
		unsigned injectTick = 0;
	};

	struct Fields {
		Slot slot[2];
	};

	bool isActivePlay() {
		auto pl = PlayLayer::get();
		return pl && static_cast<GJBaseGameLayer*>(pl) == this;
	}

	unsigned nowTick() {
		return m_gameState.m_currentProgress;
	}

	void handleButton(bool down, int button, bool isPlayer1) {
		if (button != 1 || !enabled() || !this->isActivePlay()) {
			GJBaseGameLayer::handleButton(down, button, isPlayer1);
			return;
		}

		int i = isPlayer1 ? 0 : 1;
		auto player = isPlayer1 ? m_player1 : m_player2;
		auto& s = m_fields->slot[i];
		unsigned now = this->nowTick();

		if (down) {
			s.physHeld = true;

			// Pressed again while a release was being delayed: the hold just continues.
			if (s.relPending) {
				s.relPending = false;
				state().record(static_cast<int>(now >= s.relTick ? now - s.relTick : 0), "hold");
				return;
			}

			if (bufferPress() && player && !player->m_isDead && !pressUsableNow(player)) {
				s.pending = true;
				s.pressTick = now;
			} else {
				s.pending = false;
			}
		} else {
			s.physHeld = false;
			s.injectedPress = false;

			if (extendHold() && player && !player->m_isDead) {
				s.relPending = true;
				s.relTick = now;
				return; // swallow the release for now
			}
		}

		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}

	void processCommands(float dt, bool isHalfTick, bool isLastTick) {
		if (enabled() && this->isActivePlay()) {
			this->tickStep();
		}
		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
	}

	void tickStep() {
		unsigned now = this->nowTick();
		unsigned window = static_cast<unsigned>(tickSave());

		for (int i = 0; i < 2; ++i) {
			bool isP1 = (i == 0);
			auto player = isP1 ? m_player1 : m_player2;
			if (!player) continue;
			auto& s = m_fields->slot[i];

			// Level restarted: the tick counter went backwards.
			if (now < s.pressTick || now < s.relTick || now < s.injectTick) {
				s = Slot{};
				continue;
			}

			// Finish a delayed release.
			if (s.relPending && now - s.relTick >= window) {
				s.relPending = false;
				GJBaseGameLayer::handleButton(false, 1, isP1);
			}

			// Release a press we made for the player.
			if (s.injectedPress && !s.physHeld && now > s.injectTick) {
				s.injectedPress = false;
				GJBaseGameLayer::handleButton(false, 1, isP1);
			}

			if (!s.pending) continue;

			if (player->m_isDead || now - s.pressTick > window) {
				s.pending = false;
				continue;
			}

			bool onRing = bufferOrbs() && !player->m_touchedRings.empty();
			bool onGround = bufferGround() && player->m_isOnGround && groundBufferable(player);
			if (!onRing && !onGround) continue;

			s.pending = false;
			state().record(static_cast<int>(now - s.pressTick), onRing ? "orb" : "ground");

			// If the button is still held, the game's own hold handling takes the jump.
			// If it was already released, press for the player and release next tick.
			if (onRing || !s.physHeld) {
				GJBaseGameLayer::handleButton(true, 1, isP1);
				s.injectedPress = !s.physHeld;
				s.injectTick = now;
			}
		}
	}
};

class $modify(TickPlayLayer, PlayLayer) {
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
		state().resetAttempt();
		auto hud = TickHud::create();
		hud->setID("tick-hud"_spr);
		this->addChild(hud, 1000);
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		state().resetAttempt();
	}
};
