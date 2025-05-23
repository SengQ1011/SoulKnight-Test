//
// Created by QuzzS on 2025/3/15.
//

//CollisionComponent.cpp

#include "Components/CollisionComponent.hpp"

#include "Util/Image.hpp"

std::shared_ptr<Util::Image> CollisionComponent::s_RedColliderImage = nullptr;
std::shared_ptr<Util::Image> CollisionComponent::s_BlueColliderImage = nullptr;
std::shared_ptr<Util::Image> CollisionComponent::s_YellowColliderImage = nullptr;


void CollisionComponent::Init()
{
	auto owner = GetOwner<nGameObject>();
	if (!owner) return;

	//初始化ColliderVisibleBox
	SetColliderBoxColor("Red");
	m_ColliderVisibleBox->SetZIndex(0.1);
	m_ColliderVisibleBox->SetZIndexType(ZIndexType::UI);
}

void CollisionComponent::SetColliderBoxColor(const std::string &color) const // Blue - 未定義， Yellow - 碰撞, Red - 正常
{
	if (!s_RedColliderImage)
		s_RedColliderImage = std::make_shared<Util::Image>(RESOURCE_DIR "/RedCollider.png");
	if (!s_BlueColliderImage)
		s_BlueColliderImage = std::make_shared<Util::Image>(RESOURCE_DIR "/BlueCollider.png");
	if (!s_YellowColliderImage)
		s_YellowColliderImage = std::make_shared<Util::Image>(RESOURCE_DIR "/YellowCollider.png");


	if (color == "Red" && s_RedColliderImage)
		m_ColliderVisibleBox->SetDrawable(s_RedColliderImage);
	else if (color == "Blue" && s_BlueColliderImage)
		m_ColliderVisibleBox->SetDrawable(s_BlueColliderImage);
	else if (color == "Yellow" && s_YellowColliderImage)
		m_ColliderVisibleBox->SetDrawable(s_YellowColliderImage);
}

void CollisionComponent::Update()
{
	auto owner = GetOwner<nGameObject>();
	if (!owner)
		return;
	m_ColliderVisibleBox->Update();
}

Rect CollisionComponent::GetBounds() const
{
	glm::vec2 objectPosition = {0.0f, 0.0f};
	auto owner = GetOwner<nGameObject>();
	SetColliderBoxColor("Red"); //充值顔色
	if (owner)
	{
		objectPosition = owner->m_WorldCoord;
		glm::vec2 adjustedOffset = m_Offset;

		//處理可視化矩形大小和位置
		m_ColliderVisibleBox->SetInitialScale(m_Size);
		if (owner->m_Transform.scale.x < 0.0f)
		{
			adjustedOffset.x = -adjustedOffset.x; // 如果角色反向，X轴偏移需要镜像
		}
		//反向處理
		m_ColliderVisibleBox->m_WorldCoord = objectPosition + adjustedOffset;
		return {objectPosition + adjustedOffset, m_Size};
	}
	return {objectPosition + m_Offset, m_Size};
}

bool CollisionComponent::CanCollideWith(const std::shared_ptr<CollisionComponent> &other) const
{
	return m_CollisionMask & other->m_CollisionLayer; // 你的"圖層"在我的判定"範圍"之上， 所以我可以侵犯你(?
}

// void CollisionComponent::HandleCollision(CollisionInfo &info)
// {
// 	SetColliderBoxColor("Yellow"); //碰撞變色
// }

void CollisionComponent::HandleEvent(const EventInfo& eventInfo)
{
	LOG_DEBUG("OnEvent3");
	if (eventInfo.GetEventType() == EventType::Collision)
	{
		LOG_DEBUG("OnEvent4");
		const auto& collision = dynamic_cast<const CollisionEventInfo&>(eventInfo);
		SetColliderBoxColor("Yellow"); //碰撞變色
	}
}

std::vector<EventType> CollisionComponent::SubscribedEventTypes() const
{
	return {
		EventType::Collision,
	};
}

void CollisionComponent::SetTriggerStrategy(std::unique_ptr<ITriggerStrategy> triggerStrategy)
{
	m_TriggerStrategy = std::move(triggerStrategy);
}


void CollisionComponent::TryTrigger(const std::shared_ptr<nGameObject>& self, const std::shared_ptr<nGameObject>& other)
{
	if (!m_IsTrigger || !m_TriggerStrategy) return;

	m_CurrentTriggerTargets.insert(other);
	const bool wasTriggered = m_PreviousTriggerTargets.find(other) != m_PreviousTriggerTargets.end();

	if (!wasTriggered) m_TriggerStrategy->OnTriggerEnter(self, other);
	else m_TriggerStrategy->OnTriggerStay(self, other);
}

void CollisionComponent::FinishTriggerFrame(const std::shared_ptr<nGameObject>& self)
{
	if (!m_IsTrigger || !m_TriggerStrategy) return;
	for (auto& prev : m_PreviousTriggerTargets)
	{
		if (m_CurrentTriggerTargets.find(prev) != m_CurrentTriggerTargets.end()) continue;
		m_TriggerStrategy->OnTriggerExit(self, prev);
	}

	std::swap(m_PreviousTriggerTargets, m_CurrentTriggerTargets);
	m_CurrentTriggerTargets.clear();
}





