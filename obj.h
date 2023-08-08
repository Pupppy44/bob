#pragma once
#include "bob.h"
#include <functional>

class Object {
public:
	virtual void init() {};
	virtual void render() {};

	// Events
	void click(int x, int y) {
		for (auto &e : click_events) {
			e(x, y);
		}
	}
	void add_click(std::function<void(int x, int y)> e) {
		click_events.push_back(e);
	};
public:
	const char* name;

	float x;
	float y;
	float w;
	float h;

	ID2D1HwndRenderTarget* m_pRenderTarget;
private:
	std::vector<std::function<void(int x, int y)>> click_events;
};