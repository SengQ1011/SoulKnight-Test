//
// Created by tjx20 on 3/26/2025.
//

#include "BulletManager.hpp"
#include <execution>
#include "Scene/SceneManager.hpp"

void BulletManager::spawnBullet(const CharacterType type, const std::string& bulletImagePath, const Util::Transform& transform, glm::vec2 direction, float size, float speed, int damage) {
	auto bullet = std::make_shared<Bullet>(type, bulletImagePath, transform, direction, size, speed, damage);
	bullet->PostInitialize(); // 只初始化碰撞組件，不處理渲染
	// 加入渲染樹
	auto currentScene = SceneManager::GetInstance().GetCurrentScene().lock();
	currentScene->GetRoot().lock()->AddChild(bullet);  // 由 BulletManager 的 shared_ptr 加入
	currentScene->GetCamera().lock()->AddChild(bullet);

	// 注冊到碰撞管理器
	currentScene->GetManager<RoomCollisionManager>(ManagerTypes::ROOMCOLLISION)->RegisterNGameObject(bullet);

	m_Bullets.push_back(bullet);
}

void BulletManager::Update() {
	float deltaTime = Util::Time::GetDeltaTimeMs();
	if (m_Bullets.empty()) return;
	// 並行更新
	if (m_Bullets.size() > 50) {  // 仅在大数据时使用并行
		std::for_each(std::execution::par, m_Bullets.begin(), m_Bullets.end(),
			[deltaTime](auto& bullet) { bullet->UpdateLocation(deltaTime); });
	} else {
		std::for_each(std::execution::seq, m_Bullets.begin(), m_Bullets.end(),
			[deltaTime](auto& bullet) { bullet->UpdateLocation(deltaTime); });
	}

	// 移除碰撞的子彈
	auto currentScene = SceneManager::GetInstance().GetCurrentScene().lock();
	m_Bullets.erase(std::remove_if(m_Bullets.begin(), m_Bullets.end(),
		[currentScene](const std::shared_ptr<Bullet>& bullet) {
			if (bullet->ShouldRemove()) {
				currentScene->GetRoot().lock()->RemoveChild(bullet);
				currentScene->GetCamera().lock()->RemoveChild(bullet);
				currentScene->GetManager<RoomCollisionManager>(ManagerTypes::ROOMCOLLISION)->UnregisterNGameObject(bullet);

				//TODO:還沒重置或刪除子彈喔 (凱成：我先關閉碰撞顯示）
				auto colliderBullet = bullet->GetComponent<CollisionComponent>(ComponentType::COLLISION)->GetVisibleBox();
				currentScene->GetRoot().lock()->RemoveChild(colliderBullet);
				currentScene->GetCamera().lock()->RemoveChild(colliderBullet);
				currentScene->GetManager<RoomCollisionManager>(ManagerTypes::ROOMCOLLISION)->UnregisterNGameObject(colliderBullet);

				return true;
			}
			return false;
		}),
	m_Bullets.end());
}