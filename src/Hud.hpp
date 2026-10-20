#pragma once

#include <Geode/Geode.hpp>
#include "State.hpp"

// Small overlay in the top-left corner of the PlayLayer.
class TickHud : public cocos2d::CCNode {
public:
	static TickHud* create() {
		auto ret = new TickHud();
		if (ret && ret->init()) {
			ret->autorelease();
			return ret;
		}
		CC_SAFE_DELETE(ret);
		return nullptr;
	}

protected:
	cocos2d::CCLabelBMFont* m_label = nullptr;

	bool init() override {
		if (!CCNode::init()) return false;
		auto winSize = cocos2d::CCDirector::get()->getWinSize();
		m_label = cocos2d::CCLabelBMFont::create("", "chatFont.fnt");
		m_label->setAnchorPoint({0.f, 1.f});
		m_label->setScale(0.55f);
		m_label->setOpacity(180);
		m_label->setPosition({6.f, winSize.height - 6.f});
		this->addChild(m_label);
		this->scheduleUpdate();
		return true;
	}

	void update(float) override {
		using namespace ticksaver;
		auto& st = state();
		bool enabled = geode::Mod::get()->getSettingValue<bool>("enabled");
		int n = tickSave();
		std::string text;
		if (!enabled || n == 0) {
			text = "Tick save: OFF  (F6)";
		} else {
			text = fmt::format("Tick save: {}  (F6)\nLast used: {} ({})\nThis attempt: {} ticks / {} saves\nPresses {}  held {}  expired {}",
				n, st.lastUsed, st.lastKind, st.attemptUsed, st.attemptSaves, st.seen, st.buffered, st.expired);
		}
		m_label->setString(text.c_str());
		m_label->setVisible(geode::Mod::get()->getSettingValue<bool>("show-overlay"));
	}
};
