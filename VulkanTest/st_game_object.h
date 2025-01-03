#pragma once

#include "st_model.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>

namespace st {


	struct Transform3dComponent {
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f, 1.f ,1.f};
		glm::vec3 rotation{};
        glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	class StGameObject {
	public:
		using id_t = unsigned int;

		static StGameObject createGameObject() {
			static id_t currentId = 0;
			return StGameObject{ currentId++ };
		}

		StGameObject(const StGameObject&) = delete;
		StGameObject& operator=(const StGameObject&) = delete;
		StGameObject(StGameObject&&) = default;
		StGameObject& operator=(StGameObject&&) = default;


		id_t getId() { return id; };

		std::shared_ptr<StModel> model{};
		glm::vec3 color{ 1.0f,1.0f,1.0f };
		Transform3dComponent transform3d{};

	private:
		StGameObject(id_t objId) :id{ objId } {};
		id_t id;
	};
}