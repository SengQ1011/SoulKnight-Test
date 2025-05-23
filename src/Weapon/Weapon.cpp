//
// Created by tjx20 on 3/10/2025.
//

#include "Weapon/Weapon.hpp"
#include "Util/Image.hpp"

Weapon::Weapon(const std::string& ImagePath, const std::string& name, int damage, int energy, float criticalRate, int offset, float attackInterval)
	: m_ImagePath(ImagePath), m_weaponName(name), m_damage(damage), m_energy(energy), m_criticalRate(criticalRate), m_offset(offset), m_attackInterval(attackInterval)
{
	SetImage(ImagePath);
	this->SetZIndexType(ZIndexType::OBJECTLOW);
}

void Weapon::UpdateCooldown(float deltaTime) {
	if (lastAttackTime > 0) {
		lastAttackTime -= deltaTime;
	}
}

bool Weapon::CanAttack() {
	return lastAttackTime <= 0;
}

void Weapon::SetImage(const std::string& ImagePath) {
	m_ImagePath = ImagePath;
	m_Drawable = std::make_shared<Util::Image>(m_ImagePath);
}