#pragma once



namespace st {

	class StSettingsManager {
	public:
		static StSettingsManager& getManager() {
			static StSettingsManager settingsManager{};
			return settingsManager;
		}

		size_t kmeansNodeCount;
		float cellSize;
		float brushProbeGenerationGridSize;
		float probeHeight;
		float maxProbeZ;
	private:
		StSettingsManager() {
			kmeansNodeCount = 16;
			cellSize = 128.f;
			brushProbeGenerationGridSize = 8.f;
			probeHeight = 64.f;
			maxProbeZ = 2000.f;
		}
	};

}