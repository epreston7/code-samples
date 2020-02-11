#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/Goose.h"
#include "../CSC8503Common/Item.h"
#include "../CSC8503Common/AIObject.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "NetworkedGame.h"
#include "../CSC8503Common/GameServer.h"
#include "../CSC8503Common/GameClient.h"
#include "../CSC8503Common/NetworkBase.h"

enum GameState {
	MENU, PLAY, SERVER, CLIENT, FINAL_SCREEN, EXIT, PAUSE
};

namespace NCL {
	namespace CSC8503 {
		class CourseworkGame;
		class CourseworkPacketReceiver :public PacketReceiver {
		public:
			CourseworkPacketReceiver(string name, CourseworkGame* currentGame) {
				this->name = name;
				this->game = currentGame;
			}
			void ReceivePacket(int type, GamePacket* payload, int source);
	
		protected:
			string name;
			CourseworkGame* game;
		};

		class CourseworkGame {
		public:
			CourseworkGame();
			~CourseworkGame();

			virtual void UpdateGameplay(float dt);
			virtual void UpdateGame(float dt);
			virtual void UpdateMenu();
			virtual void UpdateFinalScreen();
			virtual void UpdateClientGame(float dt);
			virtual void UpdateServerGame(float dt);
			virtual void UpdatePauseGame(float dt);
			void UpdateServerplayer(Vector3 newPos);
			int GetHighscore() { return *highScore; }
			void SetHighscore(int s) { *highScore = s; }

			bool exitGame=false;

		protected:
			void InitialiseAssets();
			void InitCamera();
			void UpdateKeys();
			void InitWorld();

			void BridgeConstraintTest();
			void SimpleGJKTest();

			bool SelectObject();
			void MovePlayer(Goose* p);
			void UpdateHeldItem();
			void SetupServer();
			void SetupClient();

			
			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, bool hollow = false, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			Goose* AddGooseToWorld(const Vector3& position);

			AIObject* AddParkKeeperToWorld(const Vector3& position);
			AIObject* AddCharacterToWorld(const Vector3& position);
			Item* AddAppleToWorld(const Vector3& position);
			Item* AddFlowerToWorld(const Vector3& position, float radius);

			void AddTreeToWorld(const Vector3& position);
			void AddSecretFloor(const Vector3& position);
			void AddStairsToWorld();
			void InitialiseItems();
			void AddHouseToWorld();
			

			void TestPathfinding(float timer, Vector3 pos);
			void DisplayPathfinding();
			void UpdateAIStateMachine();
			void UpdateAI(float timer);

			GameTechRenderer* renderer;
			PhysicsSystem* physics;
			GameWorld* world;

			bool useGravity;
			bool inSelectionMode;
			bool inPlayerMode;
			bool pauseGame=false;
			bool inMenu = true;

			float forceMagnitude;
			float timer;
			int score = 0;
			int numOfApples = 7;

			GameObject* selectionObject = nullptr;
			GameObject* objInVision = nullptr;

			Goose* player = nullptr;
			Goose* otherPlayer = nullptr;
			AIObject* keeper = nullptr;
			AIObject* character = nullptr;
			GameObject* lake = nullptr;
			GameObject* lava = nullptr;
			GameObject* island = nullptr;
			GameObject* floor = nullptr;
			GameObject* cave = nullptr;
			GameObject* teleporter1 = nullptr;
			GameObject* teleporter2 = nullptr;
			GameObject* blocks[3];
			OGLMesh* cubeMesh = nullptr;
			OGLMesh* sphereMesh = nullptr;
			OGLTexture* basicTex = nullptr;
			OGLTexture* islandTex = nullptr;
			OGLShader* basicShader = nullptr;

			OGLMesh* gooseMesh = nullptr;
			OGLMesh* keeperMesh = nullptr;
			OGLMesh* appleMesh = nullptr;
			OGLMesh* charA = nullptr;
			OGLMesh* charB = nullptr;

			StateMachine* stateMachine = new StateMachine();
			GenericState* stateIdle;
			GenericState* stateChase;
			AIState currentState = IDLE;

			void SetUpAIStates();
			Vector3 distVec;
			float dist;
			AIState state1 =IDLE;

			NavigationGrid* grid = new NavigationGrid("TestGrid1.txt");
			Vector3 offset = Vector3(-100, 0, -100);
			GameState state;
			Vector3 currentTarget;
			int* highScore= new int(0);
			GameServer* server = nullptr;
			GameClient* client = nullptr;
			CourseworkPacketReceiver serverReceiver = CourseworkPacketReceiver("Server", this);
			CourseworkPacketReceiver clientReceiver = CourseworkPacketReceiver("Client", this);
			vector<Vector3> testNodes;
			Vector3 lockedOffset = Vector3(0, 30, -40);
		};
	}
}