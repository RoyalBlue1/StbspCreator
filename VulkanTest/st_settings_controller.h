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
		__m128 maxProbeZ;
	private:
		StSettingsManager() {
			kmeansNodeCount = 8;
			cellSize = 128.f;
			brushProbeGenerationGridSize = 8.f;
			probeHeight = 64.f;
			maxProbeZ = _mm_set1_ps(2000.f);
		}
	};

}