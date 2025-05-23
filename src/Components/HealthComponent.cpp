//
// Created by tjx20 on 3/28/2025.
//

#include "Components/HealthComponent.hpp"

#include "Components/AttackComponent.hpp"
#include "Components/MovementComponent.hpp"
#include "Components/StateComponent.hpp"
#include "Room/RoomCollisionManager.hpp"
#include "ObserveManager/TrackingManager.hpp"
#include "ObserveManager/EventManager.hpp"

#include "Scene/SceneManager.hpp"
#include "Util/Time.hpp"

#include "Attack/Attack.hpp"
#include "Creature/Character.hpp"
#include "Room/MonsterRoom.hpp"
#include "Structs/DeathEventInfo.hpp"
#include "Structs/TakeDamageEventInfo.hpp"


class StateComponent;
HealthComponent::HealthComponent(const int maxHp, const int maxArmor = 0, const int maxEnergy = 0) :
	Component(ComponentType::HEALTH), m_maxHp(maxHp), m_currentHp(maxHp), m_maxArmor(maxArmor),
	m_currentArmor(maxArmor), m_maxEnergy(maxEnergy), m_currentEnergy(maxEnergy)
{
}

void HealthComponent::Update() {
	const float deltaTime = Util::Time::GetDeltaTimeMs() / 1000.0f;
	if (m_maxArmor == 0)
		return;
	if (m_currentArmor < m_maxArmor) {
		m_armorRecoveryTimer += deltaTime;
		if (m_armorRecoveryTimer >= m_armorRecoveryInterval)
		{
			AddCurrentArmor(1);
			m_armorRecoveryTimer = 0.0f;
		}
	}
	// 減少所有來源的冷卻時間
	for (auto it = m_recentAttackSources.begin(); it != m_recentAttackSources.end(); ) {
		it->second -= deltaTime;
		if (it->second <= 0)
		{
			it = m_recentAttackSources.erase(it);
		}
		else
			++it;
	}
}

void HealthComponent::HandleEvent(const EventInfo &eventInfo)
{
	switch (eventInfo.GetEventType())
	{
	case EventType::Collision: // {}可以在case裏形成額外作用域，用來在裏面定義變數
	{
		const auto& collisionEventInfo = dynamic_cast<const CollisionEventInfo&>(eventInfo);
		HandleCollision(collisionEventInfo);
		break;
	}
	case EventType::TakeDamage:
	{
		const auto& dmgInfo = dynamic_cast<const TakeDamageEventInfo&>(eventInfo);
		TakeDamage(dmgInfo.damage);
	}
	default:
		break;
	}
}

std::vector<EventType> HealthComponent::SubscribedEventTypes() const
{
	return {
		EventType::Collision,
		EventType::TakeDamage,
	};
}

void HealthComponent::HandleCollision(const CollisionEventInfo& info){
	auto collisionObject = info.GetObjectB();
	if (!collisionObject) return;
	nGameObject* rawPtr = collisionObject.get();// 取出 raw pointer

	// 冷卻中就不處理
	if (m_recentAttackSources.count(rawPtr) > 0) return;

	// 判斷碰撞對象是不是攻擊==>因爲碰撞manager已經檢查是否為敵方子彈，所以不需要再判斷
	if (const auto attack = std::dynamic_pointer_cast<Attack>(info.GetObjectB()))
	{
		const int damage = attack->GetDamage();
		this->TakeDamage(damage);
		LOG_DEBUG("damage = {}==>", damage, collisionObject->GetName());
		// 只有當攻擊物件不會馬上消失，才進入冷卻判斷
		if (!attack->WillDisappearOnHit()) {
			m_recentAttackSources[rawPtr] = m_invincibleDuration;
		}

	}

	// collisionEnemy的碰撞傷害
	if (const auto character = std::dynamic_pointer_cast<Character>(info.GetObjectB())) {
		if (character->GetType() == CharacterType::ENEMY) {
			if (const auto collisionDamage = character->GetComponent<AttackComponent>(ComponentType::ATTACK)->GetCollisionDamage();
				collisionDamage > 0) {
				this->TakeDamage(collisionDamage);
				LOG_DEBUG("Enemy collision damage = {}", collisionDamage);
				}
		}
	}
}

void HealthComponent::TakeDamage(int damage)
{
	// 天賦：破甲保護
	if (m_breakProtection && damage > m_currentArmor && m_currentArmor > 0)
	{
		m_currentArmor = 0; // 只扣盔甲
		return;
	}
	int remainingDamage = std::max(0, damage - m_currentArmor);
	m_currentArmor = std::max(0, m_currentArmor - damage);
	m_currentHp = std::max(0, m_currentHp - remainingDamage);

	if (m_currentHp == 0)
	{
		OnDeath();
	}
}

void HealthComponent::OnDeath() const
{
	auto character = GetOwner<Character>();
	if (!character)
		return;
	auto stateComponent = character->GetComponent<StateComponent>(ComponentType::STATE);
	auto movementComp = character->GetComponent<MovementComponent>(ComponentType::MOVEMENT);
	// TODO:銷毀武器
	// if (const auto attackComp = character->GetComponent<AttackComponent>(ComponentType::ATTACK)) {
	// 	attackComp->RemoveAllWeapon();
	// }

	character->SetActive(false);
	stateComponent->SetState(State::DEAD);
	LOG_DEBUG("HealthComponent::OnDeath");
	if (movementComp)
		movementComp->SetDesiredDirection(glm::vec2(0.0f, 0.0f)); // 移動向量設爲0
	auto scene = SceneManager::GetInstance().GetCurrentScene().lock();
	scene->GetCurrentRoom()->GetManager<RoomCollisionManager>(ManagerTypes::ROOMCOLLISION)->UnregisterNGameObject(character);

	auto trackingManager = scene->GetCurrentRoom()->GetManager<TrackingManager>(ManagerTypes::TRACKING);
	if (character->GetType() == CharacterType::ENEMY)
	{
		trackingManager->RemoveEnemy(character);
		// TODO:因爲Lobby房間有小怪
		auto monsterRoom = std::dynamic_pointer_cast<MonsterRoom>(scene->GetCurrentRoom());
		if (monsterRoom) monsterRoom->OnEnemyDied();
		LOG_DEBUG("HealthComponent::remove ");
	}
	else
	{
		trackingManager->SetPlayer(nullptr);
		//const DeathEventInfo deathEventInfo{character};
		//EventManager::GetInstance().Notify(deathEventInfo);
	}

	if (character->GetType() == CharacterType::ENEMY) {
		if(auto aiComp = character->GetComponent<AIComponent>(ComponentType::AI))
		{
			aiComp->HideReadyAttackIcon();
		}
	}
}


