#include "CourseworkGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/PositionConstraint.h"


using namespace NCL;
using namespace CSC8503;


CourseworkGame::CourseworkGame() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	state = MENU;
	forceMagnitude = 10.0f;
	useGravity = true;
	physics->UseGravity(useGravity);
	inPlayerMode = true;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);
	physics->SetGravity(Vector3(0, -12, 0));
	InitialiseAssets();

}

void CourseworkGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh", &cubeMesh);
	loadFunc("sphere.msh", &sphereMesh);
	loadFunc("CenteredGoose.msh", &gooseMesh);
	loadFunc("CharacterA.msh", &keeperMesh);
	loadFunc("CharacterM.msh", &charA);
	loadFunc("CharacterF.msh", &charB);
	loadFunc("Apple.msh", &appleMesh);

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.jpg");
	islandTex = (OGLTexture*)TextureLoader::LoadAPITexture("moss.jpg");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	
	InitCamera();
	InitWorld();
}

CourseworkGame::~CourseworkGame() {
	delete cubeMesh;
	delete sphereMesh;
	delete gooseMesh;
	delete basicTex;
	delete islandTex;
	delete basicShader;
	delete physics;
	delete renderer;
	delete world;
	delete stateMachine;
	delete highScore;
	delete server;
	delete client;
	delete stateIdle;
	delete stateChase;
}

void CourseworkGame::UpdateGameplay(float dt) {
	switch (state) {
	case MENU: UpdateMenu(); break;
	case PLAY: UpdateGame(dt); break;
	case SERVER: UpdateServerGame(dt); break;
	case CLIENT: UpdateClientGame(dt); break;
	case FINAL_SCREEN: UpdateFinalScreen(); break;
	case EXIT: exitGame = true; break;
	}

	Debug::FlushRenderables();
	renderer->Render();

}

void CourseworkGame::UpdateMenu() {
	NetworkBase::Destroy();
	NetworkBase::Initialise();
	InitWorld();
	Debug::Print("1. Single Player Game", Vector2(300, 650));
	Debug::Print("2. Server Player", Vector2(300, 625));
	Debug::Print("3. Client Player", Vector2(300, 600));
	Debug::Print("4. Exit Game", Vector2(300, 575));
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1)) {
		state=PLAY;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM2)) {
		SetupServer();
		state = SERVER;
		
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM3)) {
		SetupClient();
		state = CLIENT;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM4)) {
		state=EXIT;
	}
}

void CourseworkGame::UpdatePauseGame(float dt) {
	renderer->Update(dt);
}
void CourseworkGame::UpdateServerGame(float dt) {
	UpdateGame(dt);
	server->UpdateServer();
}

void CourseworkGame::UpdateClientGame(float dt) {
	UpdateGame(dt);
	client->SendPacket(PositionPacket(player->GetTransform().GetWorldPosition().x, player->GetTransform().GetWorldPosition().y, player->GetTransform().GetWorldPosition().z));
	client->SendPacket(ScorePacket(player->GetScore()));
	client->UpdateClient();
}

void CourseworkGame::UpdateFinalScreen() {
	int finalScore = max(0,(player->GetScore() - numOfApples + player->GetAppleCount()));
	Debug::Print("Final Score:" + std::to_string(finalScore), Vector2(300, 650));
	Debug::Print("1. Play Again?", Vector2(300, 620));
	Debug::Print("2. Exit", Vector2(300, 600));
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1)) {
		state = MENU;
		Sleep(200);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM2)) {
		state = EXIT;
	}

}

void CourseworkGame::SetupClient() {

	int port = NetworkBase::GetDefaultPort();
	client = new GameClient();
	client->RegisterPacketHandler(String_Message, &clientReceiver);
	client->RegisterPacketHandler(Position_data, &clientReceiver);
	client->RegisterPacketHandler(Score, &clientReceiver);
	bool canConnect = client->Connect(127, 0, 0, 1, port);
}

void CourseworkGame::SetupServer() {
	
	int port = NetworkBase::GetDefaultPort();
	server = new GameServer(port, 1);
	server->RegisterPacketHandler(String_Message, &serverReceiver);
	server->RegisterPacketHandler(Position_data, &serverReceiver);
	server->RegisterPacketHandler(Score, &serverReceiver);
}


void CourseworkGame::UpdateGame(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
		pauseGame = !pauseGame;
	}

	if (!pauseGame) {
		timer -= (dt);
		UpdateKeys();
		UpdateHeldItem();
		UpdateAI(timer);
		UpdateAIStateMachine();
		physics->Update(dt);

	}

		Debug::Print(std::to_string((int)timer+1) + " seconds left", Vector2(900, 850));
		Debug::Print("Current score: " + std::to_string(player->GetScore()), Vector2(865, 875));
		Debug::Print(std::to_string(numOfApples - player->GetAppleCount()) + " apples remain", Vector2(10, 875));

		if (pauseGame && !inSelectionMode)
			world->GetMainCamera()->UpdateCamera(dt);

		if (pauseGame) {
			Debug::Print("Game is (P)aused", Vector2(10, 20));
		}
		else {
			Debug::Print("(P)ause Game", Vector2(10, 20));
		}
		
		SelectObject();

		distVec = player->GetTransform().GetWorldPosition() - character->GetTransform().GetWorldPosition();
		distVec.y = 0;
		dist = distVec.Length();
		
		renderer->Update(dt);
		world->UpdateWorld(dt);


		if (numOfApples - player->GetAppleCount() == 0 || (int)timer <0) {
			state = FINAL_SCREEN;
		}
}

void CourseworkGame::TestPathfinding(float timer, Vector3 pos) {
	NavigationPath outPath;

	Vector3 startPos=keeper->GetTransform().GetWorldPosition();
	Vector3 endPos = pos;

	if ((int)timer%2 == 0) {
		testNodes.clear();
		bool found = grid->FindPath(startPos, endPos, outPath);

		if (!found)
			return;

		Vector3 pos;
		while (outPath.PopWaypoint(pos))
			testNodes.push_back(pos);

		currentTarget = testNodes.front();
		testNodes.erase(testNodes.begin());
	}
}

void CourseworkGame::DisplayPathfinding() {
	if (testNodes.empty())
		return;

	for (int i = 0; i < testNodes.size() - 1; i++) {
		Vector3 a = testNodes[i] + Vector3(0, 2.1, 0);
		Vector3 b = testNodes[i + 1] + Vector3(0, 2.1, 0);
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}

void CourseworkGame::UpdateAI(float timer) {
	Vector3 gooseVector = player->GetTransform().GetWorldPosition() - keeper->GetTransform().GetWorldPosition();
	gooseVector.y = 0;

	if (player->GetHeldItem() != nullptr) {
		if (gooseVector.Length() >= 12)
		{
			TestPathfinding(timer, player->GetTransform().GetWorldPosition());
//			DisplayPathfinding();


			if (testNodes.size() > 0) {
				Vector3 direction = currentTarget - keeper->GetTransform().GetWorldPosition();
				direction.y = 0;

				if (direction.Length() < 0.5) {
					currentTarget = testNodes.front();
					testNodes.erase(testNodes.begin());
				}

				direction.Normalise();
				keeper->GetPhysicsObject()->AddForceAtPosition(direction * 25, keeper->GetTransform().GetWorldPosition() + Vector3(1, 0, 0));
			}
		}
		else {
			keeper->GetPhysicsObject()->AddForceAtPosition(gooseVector.Normalised() * 25, keeper->GetTransform().GetWorldPosition() + Vector3(1, 0, 0));
		}
	}
	else {
		keeper->GetTransform().SetWorldPosition(keeper->GetSpawnPosition());
	}
}

void CourseworkGame::UpdateHeldItem() {
	Vector3 offset = player->GetTransform().GetLocalOrientation() * Vector3(0,1,1.75);
	if (player->GetHeldItem() == nullptr)
		return;
	else{
		player->GetHeldItem()->GetTransform().SetWorldPosition(player->GetTransform().GetWorldPosition()+offset);
	}
}

void CourseworkGame::UpdateServerplayer(Vector3 newPos) {
	player->GetTransform().SetWorldPosition(newPos);
}


void CourseworkGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera();
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (state != SERVER) {
		MovePlayer(player);
	}
}

bool CourseworkGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Z)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
	}
	if (pauseGame&&inSelectionMode) {
		renderer->DrawString("Press Z to exit selection!", Vector2(10, 0));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {
				selectionObject = nullptr;
			}
			if (objInVision) {
				objInVision = nullptr;
			}
			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				std::cout << selectionObject->GetName() << std::endl;
				std::cout << selectionObject->GetTransform().GetWorldPosition() << std::endl;
				std::cout << selectionObject->GetTransform().GetWorldOrientation() << std::endl;
				if (selectionObject->GetName() == "Edith") {
					std::cout << state1 << std::endl;
				}

			return true;
			}
			else {
				return false;
			}
		}
	}
	else if(pauseGame &&!inSelectionMode){
		renderer->DrawString("Press Z to select objects", Vector2(10, 0));
	}
	return false;
}

void CourseworkGame::MovePlayer(Goose* player) {

	Vector3 playerPos = player->GetTransform().GetWorldPosition();
	Vector3 camPos = playerPos + lockedOffset;

	Matrix4 temp = Matrix4::BuildViewMatrix(camPos, playerPos, Vector3(0, 1, 0));

	Matrix4 modelMat = temp.Inverse();

	if (!pauseGame) {

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler();

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x + 10);
		world->GetMainCamera()->SetYaw(angles.y);
	}

	Vector3 playerFrwd = player->GetTransform().GetWorldOrientation() * Vector3(0, 0, 1);
	Vector3 cameraRight = Vector3(modelMat.GetColumn(0)).Normalised();
	Vector3 cameraFwd = Vector3(modelMat.GetColumn(1)).Normalised();


	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {

		player->GetPhysicsObject()->AddForce(cameraRight * -20);
		Quaternion newDir = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 90.0f);

		player->GetTransform().SetLocalOrientation( Quaternion::Slerp(player->GetTransform().GetLocalOrientation(), newDir, 0.33f));
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		player->GetPhysicsObject()->AddForce(cameraRight * 20);
		Quaternion newDir = Quaternion::AxisAngleToQuaterion(Vector3(0, 1 , 0), -90.0f);

		player->GetTransform().SetLocalOrientation(Quaternion::Slerp(player->GetTransform().GetLocalOrientation(), newDir, 0.33f));
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		player->GetPhysicsObject()->AddForce(cameraFwd * 20);
		Quaternion newDir = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 0.0f);

		player->GetTransform().SetLocalOrientation(Quaternion::Slerp(player->GetTransform().GetLocalOrientation(), newDir, 0.33f));

	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		player->GetPhysicsObject()->AddForce(cameraFwd * -20);
		Quaternion newDir = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 180.0f);

		player->GetTransform().SetLocalOrientation(Quaternion::Slerp(player->GetTransform().GetLocalOrientation(), newDir, 0.33f));
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::H)) {
		if (player->GetHeldItem() == nullptr)
			PlaySound(TEXT("honk.wav"), NULL, SND_ASYNC);
		else {
			GameObject* heldObj = player->GetHeldItem();
			heldObj->GetPhysicsObject()->SetSleepObject(false);

			heldObj->GetPhysicsObject()->ApplyLinearImpulse((playerFrwd.Normalised())*10);
			player->SetHeldItem(nullptr);
		}
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::SPACE)) {
		Ray jumpRay(player->GetTransform().GetWorldPosition(), player->GetTransform().GetWorldPosition() + Vector3(0, -10, 0));
		RayCollision jumpCollision;
 		if (world->Raycast(jumpRay, jumpCollision, true)) {
			if (jumpCollision.rayDistance < 0.2)
				player->GetPhysicsObject()->AddForce(Vector3(0, 100 * physics->GetGravity().y, 0));
		}

	}
}

void CourseworkGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.5f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
}

void CourseworkGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	timer = 180;
	floor = AddFloorToWorld(Vector3(0, 0, 0));
	floor->GetRenderObject()->SetColour(Vector4(0.565, 0.933, 0.565,1));

	AddSecretFloor(Vector3(0, 100, 0));
	BridgeConstraintTest();
	
	InitialiseItems();
	AddHouseToWorld();
	AddStairsToWorld();

	GameObject* isle = AddSphereToWorld(Vector3(-80, -8, 80), 15.0, false, 0.0f);
	isle->GetRenderObject()->SetColour(Vector4(0.565, 0.933, 0.565, 1));
	isle->SetLayer(STATIC);
	
	AddTreeToWorld(Vector3(30, 4, -40));

	AddTreeToWorld(Vector3(-20, 4, -63));
	AddTreeToWorld(Vector3(-12, 4, -88));
	AddTreeToWorld(Vector3(-2, 4, -74));
	AddTreeToWorld(Vector3(12, 4, -75));
	AddTreeToWorld(Vector3(24, 4, -82));
	AddTreeToWorld(Vector3(2, 4, -84));
	AddTreeToWorld(Vector3(-2, 4, -60));
	AddTreeToWorld(Vector3(-22, 4, -90));
	AddTreeToWorld(Vector3(-83,8,80));

	GameObject* cornerLake = AddCubeToWorld(Vector3(-80, 2, 80), Vector3(20, 1, 20), 0.0f);
	cornerLake->GetRenderObject()->SetColour(Vector4(0.118, 0.565, 1.000, 1));
	cornerLake->SetBoundingVolume(nullptr);

	GameObject* cornerLakeColl = AddCubeToWorld(Vector3(-80, 1.75, 80), Vector3(20, 0.5, 20), 0.0f);
	cornerLakeColl->SetLayer(WATER);
	cornerLakeColl->GetRenderObject()->SetColour(Vector4(0.118, 0.565, 1.000, 1));


	teleporter1 = AddCubeToWorld(Vector3(-80, 0.25, -80), Vector3(2, 2, 2), 0.0f);
	teleporter1->GetRenderObject()->SetColour(Vector4(1.000, 0.271, 0.000, 0.5));
	teleporter1->SetName("teleporter1");

	teleporter2 = AddCubeToWorld(Vector3(20, 100.25, -20), Vector3(2, 2, 2), 0.0f);
	teleporter2->GetRenderObject()->SetColour(Vector4(1.000, 0.271, 0.000, 0.5));
	teleporter2->SetName("teleporter2");

	cave = AddSphereToWorld(Vector3(-80,2,-80), 15.0f, false, 0.0f);
	cave->SetBoundingVolume(nullptr);
	cave->GetRenderObject()->SetColour(Vector4(0.663, 0.663, 0.663, 1.0));

	GameObject* fakeLake = AddCubeToWorld(Vector3(0, 2, 0), Vector3(27, 1, 27), 0.0f);
	fakeLake->GetRenderObject()->SetColour(Vector4(0.118, 0.565, 1.000, 1));
	fakeLake->SetBoundingVolume(nullptr);

	lake = AddCubeToWorld(Vector3(0, 1.75, 0), Vector3(27, 0.5, 27), 0.0f);
	lake->SetName("Lake");
	lake->SetLayer(WATER);
	lake->GetRenderObject()->SetColour(Vector4(0.118, 0.565, 1.000, 1));


	island = AddSphereToWorld(Vector3(0,-10,0),16.0f,false,0.0f);
	island->SetRenderObject(new RenderObject(&island->GetTransform(), sphereMesh, islandTex, basicShader));
	island->SetName("Island");
	island->SetLayer(ISLAND);
	
	AddTreeToWorld(Vector3(32.5, 5, 32.5));

	keeper = AddParkKeeperToWorld(Vector3(65, 5, 35));

	character = AddCharacterToWorld(Vector3(30, 5, -40));

	player = AddGooseToWorld(Vector3(0, 3, 0));
	player->SetLayer(PLAYER);

	
	otherPlayer = AddGooseToWorld(Vector3(-80, 3, 80));
	otherPlayer->SetLayer(PLAYER);
	
	SetUpAIStates();
	UpdateGame(0);
}

void CourseworkGame::AddStairsToWorld() {
	GameObject* stairs1 = AddCubeToWorld(Vector3(75, 5.5, -90), Vector3(15, 3.5, 10), 0.0f);
	stairs1->GetRenderObject()->SetColour(Vector4(0.698, 0.133, 0.133, 1.0));
	stairs1->SetLayer(STATIC);
	GameObject* stairs2 = AddCubeToWorld(Vector3(75, 4.5, -77), Vector3(15, 2.5, 3), 0.0f);
	stairs2->GetRenderObject()->SetColour(Vector4(0.698, 0.133, 0.133, 1.0));
	stairs2->SetLayer(STATIC);
	GameObject* stairs3 = AddCubeToWorld(Vector3(75, 3.5, -71), Vector3(15, 1.5, 3), 0.0f);
	stairs3->GetRenderObject()->SetColour(Vector4(0.698, 0.133, 0.133, 1.0));
	stairs3->SetLayer(STATIC);
	GameObject* stairs4 = AddCubeToWorld(Vector3(75, 2.5, -65), Vector3(15, 0.5, 3), 0.0f);
	stairs4->GetRenderObject()->SetColour(Vector4(0.698, 0.133, 0.133, 1.0));
	stairs4->SetLayer(STATIC);

	GameObject* fence1 = AddCubeToWorld(Vector3(75, 11, -99), Vector3(15, 2, 0.5), 0.0f);
	fence1->GetRenderObject()->SetColour(Vector4(0.545, 0.271, 0.075, 1.0));
	fence1->SetLayer(STATIC);
	GameObject* fence2 = AddCubeToWorld(Vector3(90, 11, -90), Vector3(0.5, 2, 9), 0.0f);
	fence2->GetRenderObject()->SetColour(Vector4(0.545, 0.271, 0.075, 1.0));
	fence2->SetLayer(STATIC);
}
void CourseworkGame::InitialiseItems() {
	AddAppleToWorld(Vector3(35, 3, 0));
	AddAppleToWorld(Vector3(-30, 3, 50));
	AddAppleToWorld(Vector3(68, 3, 10));
	AddAppleToWorld(Vector3(50, 3, -20));
	AddAppleToWorld(Vector3(-29, 3, -58));
	AddAppleToWorld(Vector3(-55, 3, -12));
	AddAppleToWorld(Vector3(75, 12, -90)); 

	AddFlowerToWorld(Vector3(42, 3, 80),0.7);
	AddFlowerToWorld(Vector3(45, 3, 56.5), 0.7);
	AddFlowerToWorld(Vector3(46, 3, 70), 0.7);
	AddFlowerToWorld(Vector3(43, 3, 50.5), 0.7);
}

Item* CourseworkGame::AddFlowerToWorld(const Vector3& position, float radius) {
	Item* flower = new Item("flower");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	flower->SetBoundingVolume((CollisionVolume*)volume);
	flower->GetTransform().SetWorldScale(sphereSize);
	flower->GetTransform().SetWorldPosition(position);

	flower->SetRenderObject(new RenderObject(&flower->GetTransform(), sphereMesh, nullptr, basicShader));
	flower->SetPhysicsObject(new PhysicsObject(&flower->GetTransform(), flower->GetBoundingVolume()));

	flower->GetPhysicsObject()->SetInverseMass(10.f);
	flower->GetPhysicsObject()->InitHollowSphereInertia();

	world->AddGameObject(flower);

	flower->SetLayer(HELD_ITEM);
	flower->SetPoints(2);
	flower->GetRenderObject()->SetColour(Vector4(0.576, 0.439, 0.859, 1.0));
	return flower;
}

void CourseworkGame::AddHouseToWorld() {

	GameObject* house = AddCubeToWorld(Vector3(75, 10, 75), Vector3(20, 10, 20), 0.0f);
	house->GetRenderObject()->SetColour(Vector4(0.824, 0.706, 0.549, 1.0));
	house->SetLayer(STATIC);

	GameObject* roof = AddCubeToWorld(Vector3(75, 20, 75), Vector3(19, 2, 19), 0.0f);
	roof->GetRenderObject()->SetColour(Vector4(0.545, 0.271, 0.075, 1.0));
	roof->SetLayer(STATIC);

	GameObject* leftBushes = AddCubeToWorld(Vector3(92.5,3.5,45),Vector3(2.5,3.5,10),0.0f);
	leftBushes->GetRenderObject()->SetColour(Vector4(0.133, 0.545, 0.133,1.0));
	leftBushes->SetLayer(STATIC);

	GameObject* backBushes = AddCubeToWorld(Vector3(45,3.5,92.5),Vector3(10,3.5,2.5),0.0f);
	backBushes->GetRenderObject()->SetColour(Vector4(0.133, 0.545, 0.133, 1.0));
	backBushes->SetLayer(STATIC);

	GameObject* rightBushes = AddCubeToWorld(Vector3(37.5,3.5,65),Vector3(2.5, 3.5,25),0.0f);
	rightBushes->GetRenderObject()->SetColour(Vector4(0.133, 0.545, 0.133, 1.0));
	rightBushes->SetLayer(STATIC);

	GameObject* frontBushes = AddCubeToWorld(Vector3(47.5, 3.5, 37.5), Vector3(12.5, 3.5, 2.5), 0.0f);
	frontBushes->GetRenderObject()->SetColour(Vector4(0.133, 0.545, 0.133, 1.0));
	frontBushes->SetLayer(STATIC);

	GameObject* grass = AddCubeToWorld(Vector3(45,1.5,65),Vector3(4,1.5,20),0.0f);
	grass->GetRenderObject()->SetColour(Vector4(0.604, 0.804, 0.196, 1.0));
	grass->SetBoundingVolume(nullptr);
	
	GameObject* bush = AddSphereToWorld(Vector3(45, 2, 65), 3.0, false, 0.0f);
	bush->GetRenderObject()->SetColour(Vector4(0.133, 0.545, 0.133, 1.0));
	bush->SetBoundingVolume(nullptr);

}
GameObject* CourseworkGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("floor");
	floor->SetLayer(STATIC);

	Vector3 floorSize = Vector3(100, 2, 100);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, nullptr, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

void CourseworkGame::AddSecretFloor(const Vector3& position) {
	GameObject* secretFloor = AddFloorToWorld(position);
	secretFloor->GetRenderObject()->SetColour(Vector4(0.565, 0.933, 0.565, 1));

	Item* goldenApple = AddAppleToWorld(position+Vector3(10,50,10));
	goldenApple->SetPoints(10);
	goldenApple->GetRenderObject()->SetColour(Vector4(1.000, 0.843, 0.000,1.0));

	GameObject* gooseStatue = AddGooseToWorld(position + Vector3(0,12,0));
	Quaternion newDir = gooseStatue->GetTransform().GetLocalOrientation()*Quaternion::EulerAnglesToQuaternion(0, 180, 0);
	gooseStatue->GetTransform().SetLocalOrientation(newDir);
	gooseStatue->GetPhysicsObject()->SetSleepObject(true);

	gooseStatue->GetTransform().SetWorldScale(Vector3(10, 10, 10));
}

void CourseworkGame::AddTreeToWorld(const Vector3& position) {
	GameObject* trunk = AddCubeToWorld(position,Vector3(0.7,5,0.7),0.0f);
	GameObject* leaves = AddSphereToWorld(position+Vector3(0,8,0),4.0f,false,0.0f);
	trunk->SetLayer(STATIC);
	leaves->SetLayer(STATIC);
	trunk->GetRenderObject()->SetColour(Vector4(0.627, 0.322, 0.176, 1));
	leaves->GetRenderObject()->SetColour(Vector4(0.196, 0.804, 0.196,1));
}

GameObject* CourseworkGame::AddSphereToWorld(const Vector3& position, float radius, bool hollow, float inverseMass) {
	GameObject* sphere = new GameObject("sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, nullptr, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);

	if (hollow) {
		sphere->GetPhysicsObject()->InitHollowSphereInertia();
	}
	else {
		sphere->GetPhysicsObject()->InitSphereInertia();
	}
	world->AddGameObject(sphere);

	return sphere;
}

GameObject* CourseworkGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject("cube");

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, nullptr, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

Goose* CourseworkGame::AddGooseToWorld(const Vector3& position)
{
	float size = 1.0f;
	float inverseMass = 0.75f;

	Goose* goose = new Goose("brian the goose");
	
	SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(size, size, size));
	goose->GetTransform().SetWorldPosition(position);


	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(goose);

	return goose;
}

AIObject* CourseworkGame::AddParkKeeperToWorld(const Vector3& position)
{
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	AIObject* keeper = new AIObject("willy the keeper");
	keeper->SetLayer(AI);
	keeper->SetSpawnPosition(position);

	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	keeper->SetBoundingVolume((CollisionVolume*)volume);

	keeper->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	keeper->GetTransform().SetWorldPosition(position);
	keeper->GetTransform().SetLocalOrientation(keeper->GetTransform().GetLocalOrientation() * Quaternion::EulerAnglesToQuaternion(0, 180, 0));
	keeper->SetRenderObject(new RenderObject(&keeper->GetTransform(), keeperMesh, nullptr, basicShader));
	keeper->SetPhysicsObject(new PhysicsObject(&keeper->GetTransform(), keeper->GetBoundingVolume()));

	keeper->GetPhysicsObject()->SetInverseMass(inverseMass);
	keeper->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(keeper);

	return keeper;
}

AIObject* CourseworkGame::AddCharacterToWorld(const Vector3& position) {
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	AIObject* character = new AIObject("Edith");
	character->SetLayer(AI);
	float r = rand() / (float)RAND_MAX;


	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(character);

	return character;
}

Item* CourseworkGame::AddAppleToWorld(const Vector3& position) {
	Item* apple = new Item("tasty apple");
	apple->SetLayer(HELD_ITEM);
	apple->SetPoints(1);

	SphereVolume* volume = new SphereVolume(0.7f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(Vector3(2, 2, 2));
	apple->GetTransform().SetWorldPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader));
	apple->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

void CourseworkGame::SetUpAIStates() {


	StateFunc IdleFunction = [](void* data) {
		AIState* realState = (AIState*)data;
		(*realState) = IDLE;
	};
	StateFunc ChaseFunction = [](void* data) {
		AIState* realState = (AIState*)data;
		(*realState) =CHASE;
	};

	stateIdle = new GenericState(IdleFunction, (void*)&currentState);
	stateChase = new GenericState(ChaseFunction, (void*)&currentState);


	stateMachine->AddState(stateIdle);
	stateMachine->AddState(stateChase);


	GenericTransition<float&, int >* transitionIdletoChase = new GenericTransition<float&, int>(GenericTransition<float&, int>::LessThanTransition, dist, 10, stateIdle, stateChase);
	GenericTransition<float&, int >* transitionChaseToIdle = new GenericTransition<float&, int>(GenericTransition<float&, int>::GreaterThanTransition, dist, 15, stateChase, stateIdle);

	stateMachine->AddTransition(transitionIdletoChase);
	stateMachine->AddTransition(transitionChaseToIdle);
}

void CourseworkGame::UpdateAIStateMachine() {
	stateMachine->Update();

	if(currentState==CHASE){
		character->GetPhysicsObject()->AddForceAtPosition(distVec.Normalised()*20, character->GetTransform().GetWorldPosition()+Vector3(2,0,0));
	}

}

void CourseworkGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(2, 0.5, 2);
	Vector3 middleSize = Vector3(2, 0.5, 2);

	float	invCubeMass = 0.5;
	int		numLinks = 3;
	float	maxDistance = 10;
	float	cubeDistance = 6;

	Vector3 startPos = Vector3(-30, 11, 70);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 3)*cubeDistance,0,0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		blocks[i] = AddCubeToWorld(startPos + Vector3((i + 2) * cubeDistance, 0, 0), middleSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, blocks[i], maxDistance);
		world->AddConstraint(constraint);
		previous = blocks[i];
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void CourseworkGame::SimpleGJKTest() {
	Vector3 dimensions = Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor = AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}

void CourseworkPacketReceiver::ReceivePacket(int type, GamePacket* payload, int source) {
	if (type == String_Message) {
		StringPacket* realPacket = (StringPacket*)payload;
		string msg = realPacket->GetStringFromData();
	}
	if (type == Position_data) {
		PositionPacket* realPacket = (PositionPacket*)payload;
		Vector3 updatedPos = Vector3(realPacket->pX, realPacket->pY, realPacket->pZ);
		game->UpdateServerplayer(updatedPos);
	}

	if (type == Score) {
		ScorePacket* realPacket = (ScorePacket*)payload;
		int playerScore = realPacket->score;
		if (game->GetHighscore() < playerScore) {
			game->SetHighscore(playerScore);
		}
	}
}