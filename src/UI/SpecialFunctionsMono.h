#pragma once

#include "SpecialFunctions.h"

#include <mono/utils/mono-forward.h>

#include <vector>
#include <tuple>

struct MonoScript
{
	std::string name;
	std::string path;
	std::string assembly;
};

class CustomMono : public FunctionBase
{
	std::vector<MonoScript> Scripts;
	std::string CompilerOutput;

	void InitMono() noexcept;
	void ReloadScripts() noexcept;
	void RunExtension(const std::string& assembly) noexcept;
	void GetExtensionTypeName(const char** Namespace, const char** TypeName) noexcept;
public:
	static MonoDomain* Domain;

	CustomMono();
	virtual ~CustomMono();
	virtual void DrawUI() noexcept;
};