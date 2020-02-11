#pragma once
#include "GameObject.h"
#include "Debug.h"
namespace NCL {
	namespace CSC8503 {
		class Goose :
			public GameObject {
		public:
			Goose(string name = "") :GameObject(name) {}
			~Goose() {};

			int GetScore() { return currentScore; }
			int GetAppleCount() { return appleCount; }

			void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetLayer() == HELD_ITEM && heldItem == nullptr) {
					this->SetHeldItemSpawn(otherObject->GetTransform().GetWorldPosition());
					
					this->SetHeldItem(otherObject);
					otherObject->GetPhysicsObject()->SetSleepObject(true);
				}
				if (otherObject->GetLayer() == ISLAND && heldItem != nullptr) {
					currentScore += this->GetHeldItem()->GetPoints();
					if (this->GetHeldItem()->GetName() == "tasty apple")
						appleCount++;

					this->GetHeldItem()->GetTransform().SetWorldPosition(Vector3(0, -20, 0));
					this->SetHeldItem(nullptr);

				}
				if (otherObject->GetLayer() == WATER) {
					this->GetPhysicsObject()->SetLinearVelocity(this->GetPhysicsObject()->GetLinearVelocity() * 0.85);
					//spring usage here or in the physics
					float k = 3.0f;
					Vector3 force = Vector3(0, -k, 0);
					this->GetPhysicsObject()->ApplyLinearImpulse(-force);
					
				}

				
				if (otherObject->GetName() == "teleporter1") {
					//if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::TAB))
						this->GetTransform().SetWorldPosition(Vector3(0, 102, 0));
				}
				if (otherObject->GetName() == "teleporter2") {
					//if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::TAB))
					this->GetTransform().SetWorldPosition(Vector3(15, 3, 0));
				}

			}

			void OnCollisionEnd(GameObject* otherObject) override {
				if (otherObject->GetLayer() == WATER) {
					this->GetPhysicsObject()->SetLinearVelocity(this->GetPhysicsObject()->GetLinearVelocity());

				}
			}
		protected:
			float currentScore = 0;
			float appleCount = 0;
		};
	}
}
