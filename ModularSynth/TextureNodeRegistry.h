#pragma once

#define NOMINMAX
#include <algorithm>

#include "TextureNodes.hpp"
#include "NodeEditor.h"

#include "GUISystem.h"
#include "Slider.h"
#include "Panel.h"
#include "Label.h"
#include "RadioSelector.h"
#include "Button.h"
#include "Edit.h"

#include "portable-file-dialogs.h"

#include "nanovg/stb_image.h"

#define hex2rgbf(h) { float((h & 0xFF0000) >> 16) / 255.0f, float((h & 0xFF00) >> 8) / 255.0f, float(h & 0xFF) / 255.0f, 1.0f }

static constexpr Color generatorNodeColor = hex2rgbf(0x47b394);
static constexpr Color operatorNodeColor = hex2rgbf(0xb86335);
static constexpr Color externalNodeColor = hex2rgbf(0x7a30ba);
static constexpr Color multisampleNodeColor = hex2rgbf(0x59cf36);
static constexpr Color resultNodeColor = hex2rgbf(0x256bdb);

/* UI Editing */
using GuiBuilder = std::function<Control* (VisualNode*)>;

/* ========== */

using NodeBuilder = std::function<VisualNode* (NodeEditor*, const std::string&, const std::string&)>;

struct NodeContructor {
	std::string code, name;
	NodeBuilder onCreate;
	GuiBuilder onGui;
};

#define NodeCtor(T, c) [](NodeEditor* editor, const std::string& name, const std::string& code) { \
	return editor->create<T, VisualNode>(name, code, c);\
}

static Control* gui_ValueSlider(
	const std::string& label,
	float value,
	const std::function<void(float)> setter,
	float min = 0.0f,
	float max = 1.0f,
	float step = 0.05f
) {
	Panel* row = new Panel();
	row->drawBackground(false);
	row->bounds = { 0, 0, 0, 30 };

	Label* lbl = new Label();
	lbl->text = label;
	lbl->alignment = HorizontalAlignment::right;
	row->addChild(lbl);

	Slider* sld = new Slider();
	sld->min = min;
	sld->max = max;
	sld->step = step;
	sld->value = value;
	sld->onChange = [=](float v) {
		setter(v);
	};
	row->addChild(sld);
	
	RowLayout* rl = new RowLayout(2, 3);
	rl->expansion[0] = 0.7f;
	rl->expansion[1] = 1.3f;
	row->setLayout(rl);

	return row;
}

template <size_t Size>
static Control* gui_Vector(
	const std::string& label,
	RawValue& value
) {
	const std::string labels[] = { "X:", "Y:", "Z:", "W:" };

	Panel* root = new Panel();
	root->drawBackground(false);
	root->setLayout(new ColumnLayout(0));
	root->bounds = { 0, 0, 0, 64 };

	Label* lbl = new Label();
	lbl->text = label;
	lbl->bounds = { 0, 0, 0, 24 };
	root->addChild(lbl);

	Panel* vec = new Panel();
	vec->drawBackground(false);
	vec->setLayout(new RowLayout(Size, 0));
	vec->bounds = { 0, 0, 0, 34 };
	root->addChild(vec);

	for (size_t i = 0; i < std::min(4ull, Size); i++) {
		Edit* edt = new Edit();
		edt->inputFilter = std::regex("[0-9\\.\\-]");
		edt->text = std::to_string(value[i]);
		edt->label = labels[i];
		edt->onChange = [&value, edt, i](const std::string& text) {
			value[i] = std::stof(text);
			edt->text = std::to_string(value[i]);
		};
		vec->addChild(edt);
	}

	return root;
}

static Control* gui_ColorNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 0 };
	pnl->setLayout(new ColumnLayout());

	const std::string labels[] = { "Red", "Geen", "Blue", "Alpha" };

	GraphicsNode* nd = (GraphicsNode*)node->node();
	for (size_t i = 0; i < 4; i++) {
		auto ctrl = gui_ValueSlider(
			labels[i], nd->param("Color").value[i],
			[=](float v) {
				nd->setParam("Color", i, v);
			}
		);
		pnl->addChild(ctrl);
		pnl->bounds.height += 30;
	}

	return pnl;
}

static Control* gui_MixNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 90 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	RadioSelector* rsel = new RadioSelector();
	rsel->bounds = { 0, 0, 0, 25 };
	rsel->addOption(0, "Mix");
	rsel->addOption(1, "Add");
	rsel->addOption(2, "Sub");
	rsel->addOption(3, "Mul");
	rsel->select(int(nd->param("Mode").value[0]));
	rsel->onSelect = [=](int index) {
		nd->setParam("Mode", float(index));
	};
	pnl->addChild(rsel);

	auto ctrl = gui_ValueSlider(
		"Factor", nd->param("Factor").value[0],
		[=](float v) {
			nd->setParam("Factor", v);
		}
	);
	pnl->addChild(ctrl);

	return pnl;
}

static Control* gui_SimpleGradientNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 60 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	Label* lbl = new Label();
	lbl->text = "Angle";
	lbl->bounds = { 0, 0, 0, 18 };
	pnl->addChild(lbl);
	//gui->addControl(lbl);

	Slider* sld = new Slider();
	sld->value = nd->param("Angle").value[0];
	sld->onChange = [=](float v) {
		nd->setParam("Angle", v);
	};
	sld->min = -PI;
	sld->max = PI;
	sld->bounds = { 0, 0, 0, 30 };
	pnl->addChild(sld);

	return pnl;
}

static Control* gui_NoiseNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 60 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	auto scale = gui_ValueSlider(
		"Scale", nd->param("Scale").value[0],
		[=](float v) {
			nd->setParam("Scale", v);
		},
		1.0f, 99.0f
	);
	auto patx = gui_ValueSlider(
		"Pattern X", nd->param("Pattern X").value[0],
		[=](float v) {
			nd->setParam("Pattern X", v);
		}
	);
	auto paty = gui_ValueSlider(
		"Pattern Y", nd->param("Pattern Y").value[0],
		[=](float v) {
			nd->setParam("Pattern Y", v);
		}
	);

	pnl->addChild(scale);
	pnl->addChild(patx);
	pnl->addChild(paty);

	return pnl;
}

static Control* gui_ThresholdNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 60 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	auto thr = gui_ValueSlider(
		"Threshold", nd->param("Threshold").value[0],
		[=](float v) {
			nd->setParam("Threshold", v);
		}
	);
	auto feat = gui_ValueSlider(
		"Feather", nd->param("Feather").value[0],
		[=](float v) {
			nd->setParam("Feather", v);
		}
	);

	pnl->addChild(thr);
	pnl->addChild(feat);

	return pnl;
}

static Control* gui_ImageNode(VisualNode* node) {
	Button* btn = new Button();
	btn->text = "Load Texture";

	ImageNode* nd = (ImageNode*)node->node();
	
	btn->bounds = { 0, 0, 0, 25 };
	btn->onPress = [=]() {
		auto fp = pfd::open_file(
			"Load Image",
			pfd::path::home(),
			{ "Image Files", "*.png *.jpg *.bmp" },
			pfd::opt::none
		);
		if (!fp.result().empty()) {
			if (nd->handle) {
				delete nd->handle;
			}

			int w, h, comp;
			auto data = stbi_loadf(fp.result().front().c_str(), &w, &h, &comp, STBI_rgb_alpha);

			nd->handle = new Texture({ uint32_t(w), uint32_t(h) }, GL_RGBA32F);
			nd->handle->loadFromMemory(data, GL_RGBA, GL_FLOAT);

			nd->setParam("Image", float(nd->handle->id()));
		}
	};
	return btn;
}

static Control* gui_UVNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 60 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	RadioSelector* rsel = new RadioSelector();
	rsel->bounds = { 0, 0, 0, 25 };
	rsel->addOption(0, "Clamp");
	rsel->addOption(1, "Repeat");
	rsel->addOption(2, "Mirror");
	rsel->select(int(nd->param("Clamp").value[0]));
	rsel->onSelect = [=](int index) {
		nd->setParam("Clamp", float(index));
	};
	pnl->addChild(rsel);

	auto defAmt = gui_ValueSlider(
		"Def. Amt.", nd->param("Deform Amount").value[0],
		[=](float v) {
			nd->setParam("Deform Amount", v);
		}
	);
	pnl->addChild(defAmt);

	auto rep = gui_Vector<2ull>("Repeat", nd->paramValue("Repeat"));
	pnl->addChild(rep);

	auto pos = gui_Vector<2ull>("Position", nd->paramValue("Position"));
	pnl->addChild(pos);

	auto scl = gui_Vector<2ull>("Scale", nd->paramValue("Scale"));
	pnl->addChild(scl);

	auto rot = gui_ValueSlider(
		"Rotation", nd->param("Rotation").value[0],
		[=](float v) {
			nd->setParam("Rotation", v);
		},
		0.0f, PI * 2.0f, 0.01f
	);
	pnl->addChild(rot);

	return pnl;
}

static Control* gui_NormalMapNode(VisualNode* node) {
	Panel* pnl = new Panel();;
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 60 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	auto ctrl = gui_ValueSlider(
		"Scale", nd->param("Scale").value[0],
		[=](float v) {
			nd->setParam("Scale", v);
		},
		0.01f, 1.0f, 0.01f
	);

	pnl->addChild(ctrl);

	return pnl;
}

static Control* gui_CircleShapeNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 60 };
	pnl->setLayout(new ColumnLayout());

	GraphicsNode* nd = (GraphicsNode*)node->node();

	auto ctrl = gui_ValueSlider(
		"Radius", nd->param("Radius").value[0],
		[=](float v) {
			nd->setParam("Radius", v);
		},
		0.01f, 1.0f, 0.01f
	);

	pnl->addChild(ctrl);

	return pnl;
}

static Control* gui_BoxShapeNode(VisualNode* node) {
	Panel* pnl = new Panel();
	pnl->drawBackground(false);
	pnl->bounds = { 0, 0, 0, 0 };
	pnl->setLayout(new ColumnLayout());

	const std::string labelsCorners[] = { "Bot. Right", "Top Right", "Bot. Left", "Top Left" };
	const std::string labelsBounds[] = { "Width", "Height" };

	GraphicsNode* nd = (GraphicsNode*)node->node();

	for (size_t i = 0; i < 2; i++) {
		auto ctrl = gui_ValueSlider(
			labelsBounds[i], nd->param("Bounds").value[i],
			[=](float v) {
				nd->setParam("Bounds", i, v);
			},
			0.0f, 1.0f, 0.01f
		);
		pnl->addChild(ctrl);
		pnl->bounds.height += 30;
	}

	for (size_t i = 0; i < 4; i++) {
		auto ctrl = gui_ValueSlider(
			labelsCorners[i], nd->param("Border Radius").value[i],
			[=](float v) {
				nd->setParam("Border Radius", i, v);
			},
			0.0f, 1.0f, 0.01f
		);
		pnl->addChild(ctrl);
		pnl->bounds.height += 30;
	}

	return pnl;
}

static NodeContructor nodeTypes[] = {
	{ "COL", "Color", NodeCtor(ColorNode, generatorNodeColor), gui_ColorNode },
	{ "MIX", "Mix", NodeCtor(MixNode, operatorNodeColor), gui_MixNode },
	{ "SGR", "Simple Gradient", NodeCtor(SimpleGradientNode, generatorNodeColor), gui_SimpleGradientNode },
	{ "NOI", "Noise", NodeCtor(NoiseNode, generatorNodeColor), gui_NoiseNode },
	{ "THR", "Threshold", NodeCtor(ThresholdNode, operatorNodeColor), gui_ThresholdNode },
	{ "IMG", "Image", NodeCtor(ImageNode, externalNodeColor), gui_ImageNode },
	//{ "CAM", "Camera", NodeCtor(WebCamNode, externalNodeColor), nullptr },
	{ "UVS", "UV", NodeCtor(UVNode, generatorNodeColor), gui_UVNode },
	{ "RGR", "Radial Gradient", NodeCtor(RadialGradientNode, generatorNodeColor), nullptr },
	{ "NRM", "Normal Map", NodeCtor(NormalMapNode, multisampleNodeColor), gui_NormalMapNode },
	{ "OUT", "Output", NodeCtor(OutputNode, resultNodeColor), nullptr },

	{ "SCIRCLE", "Circle", NodeCtor(CircleShapeNode, generatorNodeColor), gui_CircleShapeNode },
	{ "SBOX", "Box", NodeCtor(BoxShapeNode, generatorNodeColor), gui_BoxShapeNode },

	{ "", "", nullptr, nullptr }
};

static Control* createTextureNodeEditorGui(VisualNode* ref) {
	for (NodeContructor ctor : nodeTypes) {
		if (ctor.code == ref->code()) {
			return ctor.onGui != nullptr ? ctor.onGui(ref) : nullptr;
		}
	}
	return nullptr;
}

static VisualNode* createNewTextureNode(NodeEditor* editor, const std::string& code) {
	for (NodeContructor ctor : nodeTypes) {
		if (ctor.code == code) {
			return ctor.onCreate(editor, ctor.name, ctor.code);
		}
	}
	return nullptr;
}
