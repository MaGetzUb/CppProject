#pragma once

#include "NodeGraph.h"
#include "Shader.h"
#include "Texture.h"

#include <memory>
#include <map>

constexpr uint32_t previewSize = 128;

std::string toCamelCase(const std::string& text);

class GraphicsNode : public Node {
public:
	virtual std::string functionName() = 0;
	virtual std::string library() = 0;
	virtual std::map<std::string, std::string> parameters() = 0;
	virtual void onCreate() = 0;

	void setup() override final;
	NodeValue solve();

	void addParam(const std::string& name, ValueType type);

	const NodeValue& param(const std::string& name) { return m_params[name]; }

	void setParam(const std::string& name, const RawValue& value) { m_params[name].value = value; m_changed = true; }
	void setParam(const std::string& name, float v) { m_params[name].value[0] = v; m_changed = true; }
	void setParam(const std::string& name, float x, float y) {
		m_params[name].value[0] = x;
		m_params[name].value[1] = y;
		m_changed = true;
	}
	void setParam(const std::string& name, float x, float y, float z) {
		m_params[name].value[0] = x;
		m_params[name].value[1] = y;
		m_params[name].value[2] = z;
		m_changed = true;
	}
	void setParam(const std::string& name, float x, float y, float z, float w) {
		m_params[name].value[0] = x;
		m_params[name].value[1] = y;
		m_params[name].value[2] = z;
		m_params[name].value[3] = w;
		m_changed = true;
	}
	void setParam(const std::string& name, size_t index, float v) { m_params[name].value[index] = v; m_changed = true; }

	bool hasParam(const std::string& name) { return m_params.find(name) != m_params.end(); }

	GLuint textureID() const { return m_texture ? m_texture->id() : 0; }

	const std::map<std::string, NodeValue>& params() { return m_params; }

protected:
	std::unique_ptr<Texture> m_texture;
	std::unique_ptr<Shader> m_shader;
	
	std::map<std::string, NodeValue> m_params;

	std::string processedSource();
};