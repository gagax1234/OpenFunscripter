#pragma once

#include "nlohmann/json.hpp"

#include <sstream>
#include <vector>
#include <string>

#include "ScriptSimulator.h"

#include "OFS_Reflection.h"
#include "OFS_Util.h"
#include "imgui.h"

#include "OFS_ScriptPositionsOverlays.h"

constexpr const char* CurrentSettingsVersion = "1";
class OpenFunscripterSettings
{
public:
	struct RecentFile {
		std::string name;
		std::string video_path;
		std::string script_path;
		template <class Archive>
		inline void reflect(Archive& ar) {
			OFS_REFLECT(name, ar);
			OFS_REFLECT(video_path, ar);
			OFS_REFLECT(script_path, ar);
		}
	};
private:
	struct ScripterSettingsData {
		std::string config_version = CurrentSettingsVersion;
		std::string last_path;
		std::string font_override;

		int32_t default_font_size = 18;
		int32_t fast_step_amount = 6;
		bool always_show_bookmark_labels = false;
		bool draw_video= true;
		bool show_simulator = true;
		bool show_simulator_3d = false;
		bool show_statistics = true;
		bool show_history = true;
		bool show_special_functions = false;
		bool show_action_editor = false;
		bool force_hw_decoding = false;
		bool mirror_mode = false;
		bool show_tcode = false;

		int32_t	vsync = 0;
		int32_t framerateLimit = 150;

		int32_t action_insert_delay_ms = 0;

		int32_t currentSpecialFunction = 0; // SpecialFunctions::RANGE_EXTENDER;

		int32_t buttonRepeatIntervalMs = 100;

		struct HeatmapSettings {
			int32_t defaultWidth = 2000;
			int32_t defaultHeight = 50;
			std::string defaultPath = "./";
			template <class Archive>
			inline void reflect(Archive& ar) {
				OFS_REFLECT(defaultWidth, ar);
				OFS_REFLECT(defaultHeight, ar);
				OFS_REFLECT(defaultPath, ar);
			}
		} heatmapSettings;

#ifdef OFS_MONO_SUPPORT
		struct MonoScriptingSettings
		{
			std::string MonoPath = "C:\\Program Files\\Mono\\";
			template <class Archive>
			inline void reflect(Archive& ar) {
				OFS_REFLECT(MonoPath, ar)
			}
		} monoSettings;
#endif

		std::vector<RecentFile> recentFiles;
		ScriptSimulator::SimulatorSettings* simulator;
		template <class Archive>
		inline void reflect(Archive& ar)
		{
			OFS_REFLECT(config_version, ar);
			// checks configuration version and cancels if it doesn't match
			if (config_version != CurrentSettingsVersion) { 
				LOGF_WARN("Settings version: \"%s\" didn't match \"%s\". Settings are reset.", config_version.c_str(), CurrentSettingsVersion);
				config_version = CurrentSettingsVersion;
				return; 
			}
			OFS_REFLECT(last_path, ar);
			OFS_REFLECT(always_show_bookmark_labels, ar);
			OFS_REFLECT(draw_video, ar);
			OFS_REFLECT(show_simulator, ar);
			OFS_REFLECT(show_simulator_3d, ar);
			OFS_REFLECT(show_statistics, ar);
			OFS_REFLECT(show_history, ar);
			OFS_REFLECT(show_special_functions, ar);
			OFS_REFLECT(show_action_editor, ar);
			OFS_REFLECT(default_font_size, ar);
			OFS_REFLECT(fast_step_amount, ar);
			OFS_REFLECT(force_hw_decoding, ar);
			OFS_REFLECT(recentFiles, ar);
			OFS_REFLECT(heatmapSettings, ar);
			OFS_REFLECT(mirror_mode, ar);
			OFS_REFLECT(action_insert_delay_ms, ar);
			OFS_REFLECT(currentSpecialFunction, ar);
			OFS_REFLECT(vsync, ar);
			OFS_REFLECT(framerateLimit, ar);
			OFS_REFLECT(buttonRepeatIntervalMs, ar);
			OFS_REFLECT(font_override, ar);
			OFS_REFLECT(show_tcode, ar);
			OFS_REFLECT_PTR(simulator, ar);
			OFS_REFLECT_NAMED("SplineMode", BaseOverlay::SplineMode, ar);

#ifdef OFS_MONO_SUPPORT
			OFS_REFLECT(monoSettings, ar);
#endif
		}
	} scripterSettings;

	std::string config_path;

	const char* ConfigStr = "config";
	nlohmann::json configObj;
	nlohmann::json& config() noexcept { return configObj[ConfigStr]; }

	void save_config();
	void load_config();
public:
	OpenFunscripterSettings(const std::string& config);
	ScripterSettingsData& data() noexcept { return scripterSettings; }
	void saveSettings();

	inline void addRecentFile(RecentFile& recentFile) noexcept {
		auto it = std::find_if(scripterSettings.recentFiles.begin(), scripterSettings.recentFiles.end(),
			[&](auto& file) {
				return file.video_path == recentFile.video_path && file.script_path == recentFile.script_path;
		});
		if (it != scripterSettings.recentFiles.end()) {
			scripterSettings.recentFiles.erase(it);
		}
		scripterSettings.recentFiles.push_back(recentFile);
		if (scripterSettings.recentFiles.size() > 5) {
			scripterSettings.recentFiles.erase(scripterSettings.recentFiles.begin());
		}
	}

	bool ShowWindow = false;
	bool ShowPreferenceWindow();
};