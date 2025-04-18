//
// Created by tjx20 on 3/26/2025.
//

#ifndef GUN_HPP
#define GUN_HPP

#include "Weapon/Weapon.hpp"

class GunWeapon final : public Weapon {
public:
	explicit GunWeapon(const std::string& ImagePath, const std::string& bulletImagePath,const std::string& name, int damage, int energy, float criticalRate, int offset, float attackInterval, float size, float speed);
	~GunWeapon() override = default;

	void attack(int damage) override;
	std::shared_ptr<Weapon> Clone() const override {
		return std::make_shared<GunWeapon>(*this);
	}

private:
	std::string m_bulletImagePath;	// 子彈照片
	float m_bulletSize;
	float m_bulletSpeed;
};

#endif //GUN_HPP
