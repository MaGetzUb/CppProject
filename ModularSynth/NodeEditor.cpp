#include "NodeEditor.h"

#include <iostream>

size_t NodeEditor::g_NodeID = 1;

constexpr float titleFontSize = 15.0f;
constexpr float bodyTextFontSize = 12.0f;
constexpr int gapBetweenSides = 32;
constexpr int gapBetweenInOuts = 4;
constexpr int padding = 7;
constexpr float socketVicinityRadius = 18.0f;

static Point lerpPoint(Point a, Point b, float t) {
	return {
		.x = float((1.0f - t) * a.x + b.x * t),
		.y = float((1.0f - t) * a.y + b.y * t)
	};
}

static void beginConnection(NVGcontext* ctx, Point a, Point b) {
	const Point ia{ a.x + 20, a.y };
	const Point ib{ b.x - 20, b.y };
	const Point mid{ (a.x + b.x) / 2, (a.y + b.y) / 2 };

	nvgBeginPath(ctx);
	if (ia.x > ib.x) {
		nvgMoveTo(ctx, a.x, a.y);
		nvgBezierTo(ctx, ia.x, ia.y, ia.x, mid.y, mid.x, mid.y);
		nvgBezierTo(ctx, ib.x, mid.y, ib.x, ib.y, b.x, b.y);
	}
	else {
		nvgMoveTo(ctx, a.x, a.y);
		nvgBezierTo(ctx, mid.x, a.y, mid.x, b.y, b.x, b.y);
	}
}

NodeEditor::NodeEditor(NodeGraph* graph) {
	if (!graph) m_graph = std::make_unique<NodeGraph>();
	else m_graph = std::unique_ptr<NodeGraph>(graph);
}

void NodeEditor::onDraw(NVGcontext* ctx, float deltaTime) {
	if (m_graph->hasChanges()) {
		if (onParamChange) onParamChange();
		m_graph->clearChanges();
	}

	Rect b = bounds;

	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, 0.0f, 0.0f, b.width, b.height, 16.0f);
	nvgFillColor(ctx, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.35f));
	nvgFill(ctx);

	nvgSave(ctx);

	nvgScissor(ctx, 6.0f, 6.0f, b.width - 12.0f, b.height - 12.0f);
	for (size_t nodeId : m_drawOrders) {
		auto node = get(nodeId);
		node->onDraw(ctx, deltaTime);
	}

	nvgSave(ctx);
	nvgStrokeWidth(ctx, 3.0f);
	nvgStrokeColor(ctx, nvgRGBf(1.0f, 1.0f, 1.0f));
	nvgLineCap(ctx, NVG_ROUND);

	for (auto&& conn : m_connections) {
		Rect outRect = conn.source->getOutputRect(conn.sourceOutput);
		Rect inRect = conn.destination->getInputRect(conn.destinationInput);

		beginConnection(ctx, { outRect.x + 5, outRect.y + 5 }, { inRect.x + 5, inRect.y + 5 });
		nvgStroke(ctx);

		nvgBeginPath(ctx);
		nvgCircle(ctx, outRect.x + 5, outRect.y + 5, 4.5f);
		nvgCircle(ctx, inRect.x + 5, inRect.y + 5, 4.5f);
		nvgFillColor(ctx, nvgRGBf(1.0f, 1.0f, 1.0f));
		nvgFill(ctx);

		Point mid = lerpPoint({ outRect.x + 5, outRect.y + 5 }, { inRect.x + 5, inRect.y + 5 }, 0.5f);

		nvgBeginPath(ctx);
		nvgMoveTo(ctx, mid.x - 6, mid.y - 6);
		nvgLineTo(ctx, mid.x + 6, mid.y + 6);
		nvgMoveTo(ctx, mid.x + 6, mid.y - 6);
		nvgLineTo(ctx, mid.x - 6, mid.y + 6);
		nvgStroke(ctx);
	}
	nvgRestore(ctx);

	if (m_state == NodeEditorState::draggingConnection) {
		VisualNode* node = get(m_selectedNode);
		if (node) {

			if(m_selectedOutput == SIZE_MAX && m_selectedInput == SIZE_MAX)
				return;

			Rect socketRect = {};
			if(m_selectedOutput >= 0)
				socketRect = node->getOutputRect(m_selectedOutput);
			else if(m_selectedInput >= 0) 
				socketRect = node->getInputRect(m_selectedInput);

			Point source = { socketRect.x + 5, socketRect.y + 5 };
			Point destination = { m_mousePos.x, m_mousePos.y };

			if(m_selectedInput > m_selectedOutput)
				std::swap(source, destination);

			nvgSave(ctx);
			beginConnection(ctx, source, destination);
			nvgStrokeWidth(ctx, 3.0f);
			nvgStrokeColor(ctx, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.5f));
			nvgStroke(ctx);

			nvgBeginPath(ctx);
			nvgCircle(ctx, source.x, source.y, 4.5f);
			nvgCircle(ctx, destination.x, destination.y, 4.5f);
			nvgFillColor(ctx, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.5f));
			nvgFill(ctx);

			{
				VisualNode* targetNode = nullptr;

				int socketIndex = -1;
				int distSquared = INT_MAX;

				if(m_selectedOutput != -1) 
					distSquared = getClosestInput(m_mousePos, targetNode, socketIndex);
				else if(m_selectedInput != -1)
					distSquared = getClosestOutput(m_mousePos, targetNode, socketIndex);

				if((targetNode != nullptr) && (socketIndex >= 0) && (targetNode != node)) 
				{

					if(distSquared < (18 * 18)) 
					{
						Rect socketRect = {};

						if(m_selectedOutput != -1)
							socketRect = targetNode->getInputRect(socketIndex);
						else if(m_selectedInput != -1)
							socketRect = targetNode->getOutputRect(socketIndex);

						nvgBeginPath(ctx);
						nvgCircle(ctx, socketRect.x + 5, socketRect.y + 5, socketVicinityRadius * m_proximityAnimation);
						nvgFillPaint(ctx, nvgRadialGradient(ctx, socketRect.x+5, socketRect.y+5, 1.0f, socketVicinityRadius * m_proximityAnimation, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f), nvgRGBAf(1.0f, 1.0f, 1.0f, 0.0f)));
						//nvgFillColor(ctx, nvgRGBAf(1.0f, 1.0f, 0.5f, 0.5f));
						nvgFill(ctx);

						m_proximityAnimation -= deltaTime;

						if(m_proximityAnimation < 0.0f)
							m_proximityAnimation += 1.0f;

					}
				} 
				else 
				{
					m_proximityAnimation = 1.0f;
				}

			}

			nvgRestore(ctx);
			
		}

	} 


	if (m_selectedNode) {
		auto node = get(m_selectedNode);
		Dimension sz = node->size();
		Rect bounds = { node->position.x, node->position.y, sz.width, sz.height };
		bounds.inflate(10);

		nvgBeginPath(ctx);
		nvgMoveTo(ctx, bounds.x, bounds.y + 10);
		nvgArcTo(ctx, bounds.x, bounds.y, bounds.x + 10, bounds.y, 10.0f);

		nvgMoveTo(ctx, bounds.x + bounds.width - 10, bounds.y);
		nvgArcTo(ctx, bounds.x + bounds.width, bounds.y, bounds.x + bounds.width, bounds.y + 10, 10.0f);

		nvgMoveTo(ctx, bounds.x, bounds.y + bounds.height - 10);
		nvgArcTo(ctx, bounds.x, bounds.y + bounds.height, bounds.x + 10, bounds.y + bounds.height, 10.0f);

		nvgMoveTo(ctx, bounds.x + bounds.width, bounds.y + bounds.height - 10);
		nvgArcTo(
			ctx,
			bounds.x + bounds.width,
			bounds.y + bounds.height,
			bounds.x + bounds.width - 10,
			bounds.y + bounds.height, 10.0f
		);

		nvgStrokeWidth(ctx, 6.0f);
		nvgStrokeColor(ctx, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.3f));
		nvgStroke(ctx);
	}

	nvgRestore(ctx);
}

void NodeEditor::onMouseDown(int button, int x, int y) {
	const float titleHeight = titleFontSize + 8.0f;

	Point mouse{ float(x), float(y) };

	if (button == 1) {
		bool clickedOnSomet = false;
		size_t clickedNode = 0;
		for (size_t i = m_drawOrders.size(); i-- > 0;) {
			auto node = get(m_drawOrders[i]);
			Dimension sz = node->size();
			Rect bounds = { node->position.x - 5, node->position.y, sz.width + 10, sz.height };
			if (bounds.hasPoint(mouse)) {
				clickedOnSomet = true;
				clickedNode = node->id();
				m_selectedOutput = -1;
				m_selectedInput = -1;
				rebuildDrawOrder();

				// check for out click
				for (size_t i = 0; i < node->outputCount(); i++) {
					Rect outRect = node->getOutputRect(i);
					outRect.inflate(2);
					if (outRect.hasPoint(mouse)) {
						m_selectedOutput = i;
						break;
					}
				}

				//Clicked input node instead, decide do we reroute a connection, or do we start a new connection.
				for (size_t i = 0; i < node->inputCount(); i++) {
					Rect inRect = node->getInputRect(i);
					inRect.inflate(2);
					if (inRect.hasPoint(mouse))
					{
						if (node->node()->input(i).connected) 
						{
							auto connections = getConnectionsTo(node);
							VisualConnection connection = { nullptr, nullptr, -1, -1 };
							for (auto c : connections) {
								if(c.destinationInput == i) {
									connection = c;
								}
							}
							if (connection.source && connection.destination) 
							{
								removeConnection(connection.source, connection.sourceOutput, connection.destination, connection.destinationInput);
								m_selectedOutput = connection.sourceOutput;
								clickedNode = connection.source->id();
								break;
							}
						}
						else 
						{
							m_selectedInput = i;
							m_state = NodeEditorState::draggingConnection;
							break;
						}
						break;
					}
				}

				break;
			}
		}

		if (!clickedOnSomet) {
			m_selectedNode = 0;
		}
		else {
			if(m_selectedOutput != -1 || m_selectedInput != -1) 
			{
				m_state = NodeEditorState::draggingConnection;
			}
			else 
			{
				m_state = NodeEditorState::draggingNode;
			}

			if(m_state == NodeEditorState::draggingConnection)
					m_proximityAnimation = 1.0f;

			if (clickedNode != m_selectedNode) {
				m_selectedNode = clickedNode;
				if (onSelect) onSelect(get(m_selectedNode));
			}
		}
	}
}

void NodeEditor::onMouseUp(int button, int x, int y) {
	Point mouse{ float(x), float(y) };

	if (m_state == NodeEditorState::draggingConnection) {
		if(m_selectedOutput >= 0)  {
			VisualNode* source = get(m_selectedNode);
			VisualNode* target = nullptr;
			int input = -1;
			int dist = getClosestInput(mouse, target, input);

			if(target && input >= 0 && dist < (socketVicinityRadius * socketVicinityRadius)) {
				if(!target->node()->input(input).connected) 
					connect(source, m_selectedOutput, target, input);
			}
		}
		else if(m_selectedInput >= 0) {
			VisualNode* source = get(m_selectedNode);
			VisualNode* target = nullptr;
			int output = -1;
			int dist = getClosestOutput(mouse, target, output);

			if(target && output >= 0 && dist < (socketVicinityRadius * socketVicinityRadius)) {
				connect(target, output, source, m_selectedInput);
			}
		}
	}
	else {
		for (auto&& conn : m_connections) {
			Rect outRect = conn.source->getOutputRect(conn.sourceOutput);
			Rect inRect = conn.destination->getInputRect(conn.destinationInput);
			Point mid = lerpPoint({ outRect.x + 5, outRect.y + 5 }, { inRect.x + 5, inRect.y + 5 }, 0.5f);
			Rect midRect = { mid.x - 6, mid.y - 6, 12, 12 };
			if (midRect.hasPoint(mouse) && button == 3) { // RMB = remove link
				removeConnection(conn.source, conn.sourceOutput, conn.destination, conn.destinationInput);
				break;
			}
		}
	}
	m_state = NodeEditorState::idling;
}

std::vector<VisualConnection> NodeEditor::getConnectionsTo(VisualNode* node) {
	std::vector<VisualConnection> ret;
	for (auto&& conn : m_connections) {
		if (conn.destination != node) continue;
		ret.push_back(conn);

		auto conns = getConnectionsTo(conn.source);
		ret.insert(ret.end(), conns.begin(), conns.end());
	}
	return ret;
}

std::vector<VisualConnection> NodeEditor::getConnectionsFrom(VisualNode* node) {
	std::vector<VisualConnection> ret;
	for (auto&& conn : m_connections) {
		if (conn.source != node) continue;
		ret.push_back(conn);

		auto conns = getConnectionsFrom(conn.destination);
		ret.insert(ret.end(), conns.begin(), conns.end());
	}
	return ret;
}

void NodeEditor::moveNodeTree(VisualNode* node, int dx, int dy) {
	node->position.x += dx;
	node->position.y += dy;

	// move outputs
	for (auto&& conn : getConnectionsFrom(node)) {
		conn.destination->position.x += dx;
		conn.destination->position.y += dy;
	}

	// mode inputs
	for (auto&& conn : getConnectionsTo(node)) {
		conn.source->position.x += dx;
		conn.source->position.y += dy;
	}
}

void NodeEditor::onMouseMove(int x, int y, int dx, int dy) {
	m_mousePos.x = x;
	m_mousePos.y = y;

	if (m_state == NodeEditorState::draggingNode) {
		VisualNode* node = get(m_selectedNode);
		if (!node) return;

		// Move nodes connected to this node output
		if (m_shiftPressed) {
			moveNodeTree(node, dx, dy);
		}
		else {
			node->position.x += dx;
			node->position.y += dy;
		}
	}
}

void NodeEditor::onMouseLeave() {
	m_state = NodeEditorState::idling;
}

bool NodeEditor::onKeyPress(int key) {
	if (key == VK_SHIFT) {
		m_shiftPressed = true;
		return true;
	}
	return false;
}

bool NodeEditor::onKeyRelease(int key) {
	if (key == VK_SHIFT) {
		m_shiftPressed = false;
		return true;
	}
	return false;
}

int NodeEditor::getClosestInput(Point p, VisualNode*& node, int& inputRectIndex)
{
	int maxDistance = INT_MAX;
	node = nullptr;
	inputRectIndex = -1;
	for(auto& n: m_nodes) 
	{
		for(int i = 0; i < n->inputCount(); i++)
		{
			Rect rect = n->getInputRect(i);
			if(int dist = rect.distanceToPointSquared(p); dist < maxDistance) 
			{
				node = n.get();
				inputRectIndex = i;
				maxDistance = dist;
			}
		}
	}

	return maxDistance;

}

int NodeEditor::getClosestOutput(Point p, VisualNode*& node, int& outputRectIndex)
{
	int maxDistance = INT_MAX;
	node = nullptr;
	outputRectIndex = -1;
	for(auto& n: m_nodes) 
	{
		for(int i = 0; i < n->outputCount(); i++)
		{
			Rect rect = n->getOutputRect(i);
			if(int dist = rect.distanceToPointSquared(p); dist < maxDistance) 
			{
				node = n.get();
				outputRectIndex = i;
				maxDistance = dist;
			}
		}
	}

	return maxDistance;

}

Rect VisualNode::getOutputRect(size_t index) {
	return m_outputRects[index];
}

Rect VisualNode::getInputRect(size_t index) {
	return m_inputRects[index];
}

void VisualNode::onDraw(NVGcontext* ctx, float deltaTime) {
	Dimension sz = computeSize(ctx);
	Rect b = { 0, 0, sz.width, sz.height };
	Color col = color();

	const float titleHeight = titleFontSize + 8.0f;

	nvgSave(ctx);
	nvgTranslate(ctx, position.x, position.y);

	// drop shadow :)
	NVGpaint shadowPaint = nvgBoxGradient(
		ctx,
		b.x, b.y + 2, b.width, b.height,
		5.0f, 10.0f,
		nvgRGBAf(0.0f, 0.0f, 0.0f, 0.4f),
		nvgRGBAf(0.0f, 0.0f, 0.0f, 0.0f)
	);
	nvgBeginPath(ctx);
	nvgRect(ctx, b.x - 10, b.y - 10, b.width + 20, b.height + 20);
	nvgRoundedRect(ctx, b.x, b.y, b.width, b.height, 5.0f);
	nvgPathWinding(ctx, NVG_HOLE);
	nvgFillPaint(ctx, shadowPaint);
	nvgFill(ctx);

	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, 0.0f, 0.0f, b.width, b.height, 5.0f);
	nvgFillColor(ctx, nvgRGBAf(col.r, col.g, col.b, col.a));
	nvgFill(ctx);

	nvgBeginPath(ctx);
	nvgRoundedRectVarying(ctx, 0.0f, 0.0f, b.width, titleHeight, 5.0f, 5.0f, 0.0f, 0.0f);
	nvgFillColor(ctx, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.5f));
	nvgFill(ctx);

	nvgFillColor(ctx, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));
	nvgFontSize(ctx, titleFontSize);
	nvgTextAlign(ctx, NVG_ALIGN_MIDDLE);
	nvgText(ctx, padding, titleHeight / 2 + 1.5f, name().c_str(), nullptr);

#ifdef _DEBUG
	auto idname = std::to_string(m_node->id());
	nvgFillColor(ctx, nvgRGBAf(0.0f, 1.0f, 0.0f, 1.0f));
	nvgFontSize(ctx, titleFontSize);
	nvgTextAlign(ctx, NVG_ALIGN_MIDDLE);
	nvgText(ctx, 0.0f, 0.0f, idname.c_str(), nullptr);
#endif

	float size[4];
	float posY = titleHeight + padding;

	if (m_inputRects.size() < inputCount()) {
		m_inputRects.resize(inputCount());
	}

	nvgFontSize(ctx, bodyTextFontSize);
	nvgFontFace(ctx, "default-bold");
	nvgTextAlign(ctx, NVG_ALIGN_TOP);
	nvgStrokeWidth(ctx, 1.0f);

	for (size_t i = 0; i < inputCount(); i++) {
		nvgTextBounds(ctx, 0.0f, 0.0f, m_node->inputName(i).c_str(), nullptr, size);
		nvgFillColor(ctx, nvgRGBf(0.0f, 0.0f, 0.0f));
		nvgText(ctx, padding, posY + 1.5f, m_node->inputName(i).c_str(), nullptr);

		float halfTextHeight = (size[3] - size[1]) / 2.0f;

		nvgBeginPath(ctx);
		nvgCircle(ctx, 0.0f, posY + halfTextHeight, 5.0f);
		nvgStrokeColor(ctx, nvgRGB(0, 0, 0));
		nvgFillColor(ctx, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.8f));
		nvgFill(ctx);
		nvgStroke(ctx);

		Rect outRect = { position.x - 5.0f, posY + halfTextHeight - 5.0f + position.y, 10.0f, 10.0f };
		m_inputRects[i] = outRect;

		posY += halfTextHeight * 2.0f + gapBetweenInOuts;
	}

	nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);

	posY = titleHeight + padding;

	if (m_outputRects.size() < outputCount()) {
		m_outputRects.resize(outputCount());
	}

	for (size_t i = 0; i < outputCount(); i++) {
		nvgTextBounds(ctx, 0.0f, 0.0f, m_node->outputName(i).c_str(), nullptr, size);
		nvgFillColor(ctx, nvgRGBf(0.0f, 0.0f, 0.0f));
		nvgText(ctx, b.width - padding, posY + 1.5f, m_node->outputName(i).c_str(), nullptr);

		float halfTextHeight = (size[3] - size[1]) / 2.0f;

		nvgBeginPath(ctx);
		nvgCircle(ctx, b.width, posY + halfTextHeight, 5.0f);
		nvgStrokeColor(ctx, nvgRGB(0, 0, 0));
		nvgFillColor(ctx, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.8f));
		nvgFill(ctx);
		nvgStroke(ctx);

		Rect outRect = { b.width - 5.0f + position.x, posY + halfTextHeight - 5.0f + position.y, 10.0f, 10.0f };
		m_outputRects[i] = outRect;

		posY += halfTextHeight * 2.0f + gapBetweenInOuts;
	}

	if (extraSize().width * extraSize().height > 0) {
		nvgSave(ctx);
		nvgTranslate(ctx, padding, sz.height - (extraSize().height + padding));
		onExtraDraw(ctx, deltaTime);
		nvgRestore(ctx);
	}

	nvgRestore(ctx);
}

Dimension VisualNode::computeSize(NVGcontext* ctx) {
	int width = padding * 2;
	int height = padding * 2;

	nvgSave(ctx);

	// title size
	float size[4];

	nvgFontSize(ctx, titleFontSize);
	nvgFontFace(ctx, "default-bold");
	nvgTextBounds(ctx, 0.0f, 0.0f, name().c_str(), nullptr, size);

	width += int(size[2] - size[0]);
	height += int(size[3] - size[1]);

	// inputs text size (get the max)
	nvgFontSize(ctx, bodyTextFontSize);
	nvgFontFace(ctx, "default-bold");

	float maxInputWidth = 0.0f;
	float inputsHeight = 0.0f;

	for (size_t i = 0; i < inputCount(); i++) {
		nvgTextBounds(ctx, 0.0f, 0.0f, m_node->inputName(i).c_str(), nullptr, size);
		maxInputWidth = std::max(maxInputWidth, size[2] - size[0]);
		inputsHeight += size[3] - size[1];
	}

	// outputs text size (get the max)
	float maxOutputWidth = 0.0f;
	float outputsHeight = 0.0f;

	for (size_t i = 0; i < outputCount(); i++) {
		nvgTextBounds(ctx, 0.0f, 0.0f, m_node->outputName(i).c_str(), nullptr, size);
		maxOutputWidth = std::max(maxOutputWidth, size[2] - size[0]);
		outputsHeight += size[3] - size[1];
	}

	width = std::max(width, int(maxInputWidth) + gapBetweenSides + int(maxOutputWidth));
	height += std::max(int(inputsHeight), int(outputsHeight));

	const size_t connectableCount = std::max(outputCount(), inputCount());
	height += gapBetweenInOuts * connectableCount;

	width = std::max(width, extraSize().width + padding * 2);
	height += extraSize().height;

	// some extra magic-number-driven padding on the bottom...
	height += 8;

	nvgRestore(ctx);

	m_size.width = width;
	m_size.height = height;

	return m_size;
}

void NodeEditor::connect(VisualNode* source, size_t sourceOutput, VisualNode* destination, size_t destinationInput) {
	VisualConnection conn{
		.source = source,
		.destination = destination,
		.destinationInput = destinationInput,
		.sourceOutput = sourceOutput
	};

	if(m_graph->connect(source->node(), sourceOutput, destination->node(), destinationInput)) {
		m_connections.push_back(conn);
		m_graph->solve();
	}
}

void NodeEditor::removeConnection(VisualNode* source, size_t sourceOutput, VisualNode* destination, size_t destinationInput) {
	auto pos = std::find_if(m_connections.begin(), m_connections.end(), [=](const VisualConnection& cn) {
		return cn.destination == destination &&
			cn.destinationInput == destinationInput &&
			cn.source == source &&
			cn.sourceOutput == sourceOutput;
		});
	if (pos == m_connections.end()) return;
	m_connections.erase(pos);
	m_graph->removeConnection(source->node(), sourceOutput, destination->node(), destinationInput);
	m_graph->solve();
}

void NodeEditor::rebuildDrawOrder() {
	m_drawOrders.clear();
	for (auto&& node : m_nodes) {
		if (node->id() == m_selectedNode) continue;
		m_drawOrders.push_back(node->id());
	}
	if (m_selectedNode) m_drawOrders.push_back(m_selectedNode);
}
