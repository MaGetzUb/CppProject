#include "GUISystem.h"

#include "Application.h"
#include "Panel.h"

ControlID GUISystem::g_ControlID = 1;

void GUISystem::createControl(Control* control) {
	control->m_id = g_ControlID++;
	m_controls[control->m_id] = std::shared_ptr<Control>(control);

	auto&& ctrl = m_controls[control->m_id];
	m_mouseButtonEventSystem->addListener(ctrl);
	m_mouseMotionEventSystem->addListener(ctrl);
	m_keyboardEventSystem->addListener(ctrl);
}

void GUISystem::addControl(Control* control) {
	m_controlsAdd.push_back(control);
}

void GUISystem::deleteControl(ControlID control) {
	if (m_controls.find(control) == m_controls.end()) return;

	auto&& ctrl = m_controls[control];
	ctrl->clearExtraControls();

	m_mouseButtonEventSystem->removeListener(ctrl);
	m_mouseMotionEventSystem->removeListener(ctrl);
	m_keyboardEventSystem->removeListener(ctrl);

	m_controls.erase(control);
}

void GUISystem::begin() {
	for (auto&& ctrl : m_controlsAdd) {
		createControl(ctrl);
	}
	m_controlsAdd.clear();
}

void GUISystem::end() {
	for (auto&& ctrl : m_controlsRemove) {
		deleteControl(ctrl);
	}
	m_controlsRemove.clear();
}

void GUISystem::removeControl(ControlID control) {
	m_controlsRemove.push_back(control);
	for (auto&& ctrl : m_controls[control]->onGetExtraControls()) {
		removeControl(ctrl->id());
	}
}

void GUISystem::renderAll(NVGcontext* ctx, float deltaTime) {
	end();
	begin();
	for (auto&& [ cid, ctrl ] : m_controls) {
		if (ctrl->parent()) continue; // Parent handles drawing of children.

		nvgSave(ctx);

		// make local coordinates
		Rect bounds = ctrl->screenSpaceBounds();
		nvgTranslate(ctx, bounds.x, bounds.y);

		ctrl->onDraw(ctx, deltaTime);
		nvgRestore(ctx);
	}

	for (auto&& [cid, ctrl] : m_controls) {
		nvgSave(ctx);

		// make local coordinates
		Rect bounds = ctrl->screenSpaceBounds();
		nvgTranslate(ctx, bounds.x, bounds.y);

		ctrl->onPostDraw(ctx, deltaTime);
		nvgRestore(ctx);
	}
}

void GUISystem::attachToApplication(Application& app) {
	m_mouseButtonEventSystem = &app.mouseEvents();
	m_mouseMotionEventSystem = &app.motionEvents();
	m_keyboardEventSystem = &app.keyboardEvents();
}
