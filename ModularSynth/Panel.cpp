#include "Panel.h"

constexpr float titleHeight = 40.0f;

void Panel::onDraw(NVGcontext* ctx, float deltaTime) {
	Rect b = bounds;

	if (m_drawBackground) {
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, 0.0f, 0.0f, b.width, b.height, 16.0f);
		nvgFillColor(ctx, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.35f));
		nvgFill(ctx);

		nvgBeginPath(ctx);
		nvgRoundedRectVarying(ctx, 0.0f, 0.0f, b.width, titleHeight, 16.0f, 16.0f, 0.0f, 0.0f);
		nvgFillColor(ctx, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.45f));
		nvgFill(ctx);

		nvgFillColor(ctx, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));
		nvgFontSize(ctx, 16.0f);
		nvgTextAlign(ctx, NVG_ALIGN_MIDDLE);
		nvgText(ctx, 16.0f, titleHeight / 2 + 1.5f, title.c_str(), nullptr);
	}

	int index = 0;

	if (m_layout) m_layout->beginLayout();
	for (auto&& child : m_children) {
		if (m_layout) m_layout->performLayout(child, { b.width, b.height }, index);
		
		if (m_drawBackground && m_layout) {
			child->bounds.y += titleHeight;
		}

		nvgSave(ctx);
		Rect cbounds = child->bounds;
		nvgTranslate(ctx, cbounds.x, cbounds.y);

		child->onDraw(ctx, deltaTime);
		nvgRestore(ctx);

		index++;
	}
	if (m_layout) m_layout->endLayout();
}

void Panel::setLayout(Layout* layout) {
	m_layout.reset(layout);
}

void Panel::addChild(Control* control) {
	control->parent(this);
	m_children.push_back(control);
}

void ColumnLayout::beginLayout() {
	m_ypos = 0;
}

void ColumnLayout::performLayout(Control* control, Dimension parentSize, size_t index) {
	control->bounds.width = parentSize.width - padding * 2;
	control->bounds.x = padding;
	control->bounds.y = padding + m_ypos;
	m_ypos += control->bounds.height;
}

void RowLayout::beginLayout() {
	m_xpos = 0;
}

void RowLayout::performLayout(Control* control, Dimension parentSize, size_t index) {
	const int columnWidth = (parentSize.width - padding * 2) / columns;

	control->bounds.height = parentSize.height - padding * 2;
	control->bounds.width = columnWidth;
	control->bounds.x = m_xpos + padding;
	control->bounds.y = padding;

	m_xpos += columnWidth;
}