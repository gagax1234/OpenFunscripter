#pragma once
#include <cstdint>

#include "OFS_TCodeProducer.h"
#include "OFS_Util.h"
#include "FunscriptAction.h"


class TCodePlayer {
	std::string loadPath;
public:
	int current_port = 0;
	int port_count = 0;
	struct sp_port** port_list = nullptr;
	struct sp_port* port = nullptr;

	int32_t tickrate = 250;
	int32_t delay = 0;

	TCodeChannels tcode;
	TCodeProducer prod;
	float lastPausedTimeMs = 0.f;

	TCodePlayer();
	~TCodePlayer();
	
	bool openPort(struct sp_port* port) noexcept;
	void loadSettings(const std::string& path) noexcept;
	void save() noexcept;

	void DrawWindow(bool* open, float currentTimeMs) noexcept;

	void setScripts(std::vector<std::weak_ptr<const Funscript>>&& scripts) noexcept;
	void play(float currentTimeMs, std::vector<std::weak_ptr<const Funscript>>&& scripts) noexcept;
	void stop() noexcept;
	void sync(float currentTimeMs, float speed) noexcept;
	void reset() noexcept;

	template <class Archive>
	inline void reflect(Archive& ar) {
		OFS_REFLECT(tcode, ar);
		OFS_REFLECT(tickrate, ar);
		OFS_REFLECT(delay, ar);
		OFS_REFLECT_NAMED("SplineMode", TCodeChannel::SplineMode, ar);
		OFS_REFLECT_NAMED("RemapToFullRange", TCodeChannel::RemapToFullRange, ar);
	}
};
