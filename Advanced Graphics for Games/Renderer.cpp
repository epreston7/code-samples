//Author: Elizabeth Preston
//Date: 18/11/19


#include "Renderer.h"
#include <cmath>
#include "../../nclgl/OBJMesh.h"

float lerp(float a, float b, float time) {
	return a + time * (b - a);
}

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	Tree::CreateCylinder();
	Tree::CreateCone();

	camera = new Camera(-35.0f,0.0,0.0, Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.0f, 2500.0f, RAW_WIDTH * HEIGHTMAP_X+2500));

	pos1.position = Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.0f, 2500.0f, RAW_WIDTH * HEIGHTMAP_X + 2500); pos1.pitch = -35.0f; pos1.yaw = 0.0f;
	pos2.position = Vector3(RAW_WIDTH * HEIGHTMAP_X + 2500, 2500.0f, RAW_WIDTH * HEIGHTMAP_X / 2.0f); pos2.pitch = -35.0f; pos2.yaw = 90.0f;
	pos3.position = Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.0f, 2500.0f, -2500); pos3.pitch = -35.0f; pos3.yaw = 180.0f;
	pos4.position = Vector3(-2500, 2500.0f, RAW_WIDTH * HEIGHTMAP_X / 2.0f); pos4.pitch = -35.0f; pos4.yaw = 270.0f;
	

	heightMap = new HeightMap(TEXTUREDIR"terrain.raw");
	quad = Mesh::GenerateQuad();
	processQuad = Mesh::GenerateQuad();
	splitScreenQuad1 = Mesh::GenerateRec1();
	splitScreenQuad2 = Mesh::GenerateRec2();

	light = new Light(Vector3((RAW_HEIGHT * HEIGHTMAP_X / 5.0f), 1500.0f, (RAW_HEIGHT * HEIGHTMAP_X / 5.0f)), Vector4(0.9f, 0.9f, 1.0f, 1), (RAW_WIDTH * HEIGHTMAP_X) + 4000);

	spotlight = new Spotlight(Vector3(2000, 700.0f, 200), Vector4(1.0,0.0,0.0,1.0), 5000,
							 Vector3(0,-1,0), 25.0f);
	
	hellData = new MD5FileData(MESHDIR"hellknight.md5mesh");
	hellNode = new MD5Node(*hellData);
	hellData->AddAnim(MESHDIR"idle2.md5anim");
	hellNode->PlayAnim(MESHDIR"idle2.md5anim"); 

	
	reflectShader = new Shader(SHADERDIR"PerPixelVertex.glsl", SHADERDIR"reflectFragment.glsl");
	skyboxShader = new Shader(SHADERDIR"skyboxVertex.glsl", SHADERDIR"skyboxFragment.glsl");
	lightShader = new Shader(SHADERDIR"BumpVertex.glsl", SHADERDIR"BumpFragment.glsl");
	hkShader = new Shader(SHADERDIR"shadowSceneVertex.glsl", SHADERDIR"shadowSceneFragment.glsl");
	hkShadowShader = new Shader(SHADERDIR"shadowVertex.glsl", SHADERDIR"shadowFragment.glsl");
	treeShader = new Shader(SHADERDIR"SceneVertex.glsl", SHADERDIR"SceneFragment.glsl");
	processShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"processFrag.glsl");
	sceneShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");


	if (!reflectShader->LinkProgram())
		return;
	if (!skyboxShader->LinkProgram())
		return;
	if (!lightShader->LinkProgram())
		return;
	if (!hkShader->LinkProgram())
		return;
	if (!treeShader->LinkProgram())
		return;
	if (!hkShadowShader->LinkProgram())
		return;
	if (!processShader->LinkProgram())
		return;
	if (!sceneShader->LinkProgram())
		return;

	quad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"slime.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"alienGround.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"alienGroundBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetSecTexture(SOIL_load_OGL_texture(TEXTUREDIR"alienTerrain.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetSecBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"alienTerrainBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"hell_ft.tga", TEXTUREDIR"hell_bk.tga",
		TEXTUREDIR"hell_up.tga", TEXTUREDIR"hell_dn.tga",
		TEXTUREDIR"hell_rt.tga", TEXTUREDIR"hell_lf.tga",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!cubeMap || !quad->GetTexture() || !heightMap->GetBumpMap() || !heightMap->GetTexture() || !heightMap->GetSecTexture())
		return;

	SetTextureRepeating(quad->GetTexture(), true);
	SetTextureRepeating(heightMap->GetTexture(), true);
	SetTextureRepeating(heightMap->GetBumpMap(), true);
	SetTextureRepeating(heightMap->GetSecTexture(), true);
	SetTextureRepeating(heightMap->GetSecBumpMap(), true);
	
	root = new SceneNode();
	root->SetLocalTransform(Matrix4::Translation(Vector3(900, 20, 2900)));

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			trees[i] = new Tree();
			trees[i]->SetLocalTransform(Matrix4::Translation(Vector3(200 * i, 0, 200*j))*Matrix4::Scale(Vector3(1,1+i,1)));
			root->AddChild(trees[i]);
		}
	}

	SetUpPostProcess();
	SetUpShadows();


	autoCamera = true; blur = false; split = false;
	time = 0.0f; mixer = 0.0f; waterRotate = 0.0f; transparency = 0.0f;
	timer = 0.0f; spotLightModX = 0.0, spotLightModZ = 0.0;
	init = true;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Renderer::~Renderer(void) {
	delete camera;
	delete heightMap;
	delete quad; delete processQuad; delete splitScreenQuad1; delete splitScreenQuad2;
	delete light; delete spotlight;
	delete reflectShader; delete skyboxShader; delete lightShader; delete hkShader;
	delete hkShadowShader; delete treeShader; delete processShader; delete sceneShader;
	glDeleteTextures(1, &shadowTex);
	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
	glDeleteFramebuffers(1, &shadowFBO);
	delete hellData; delete hellNode; delete root;
	Tree::DeleteTree();
	currentShader = 0;
}


void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	hellNode->Update(msec);
	root->Update(msec);
	
	timer += 0.0025;
	waterRotate += msec / 500.0f;
	spotLightModX = pow(sin(10 * timer + 1),2);
	spotLightModZ = pow(cos(10 * timer),2);

	if (autoCamera)
		AutoCameraPath(timer);

	if (time == HEIGHTMAP_Y) {
		mixer += 0.001;
		if (mixer == 0.8f)
			return;
		else if (mixer > 0.8f)
			mixer = 0.8f;
		return;
	}
	else if (time > HEIGHTMAP_Y)
		time = HEIGHTMAP_Y;
	else
		time += msec * 0.0001;
}


void Renderer::RenderScene() {
	DrawScene();

	if(blur)
	DrawPostProcess();

	PresentScene();
	SwapBuffers();
}


void Renderer::DrawScene() {
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	DrawShadowScene();
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	viewMatrix = camera->BuildViewMatrix();
	DrawSkybox();
	DrawCombinedScene();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	SetCurrentShader(processShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();
	glDisable(GL_DEPTH_TEST);
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);
		processQuad->SetTexture(bufferColourTex[0]);
		processQuad->Draw();

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		processQuad->SetTexture(bufferColourTex[1]);
		processQuad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(sceneShader);

	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	if (split) {
		splitScreenQuad1->SetTexture(bufferColourTex[0]);
		splitScreenQuad2->SetTexture(bufferColourTex[0]);
		splitScreenQuad1->Draw();
		splitScreenQuad2->Draw();
	}
	else {
		processQuad->SetTexture(bufferColourTex[0]);
		processQuad->Draw();
	}

	glUseProgram(0);
}

void Renderer::DrawTree(Shader* shader) {
	SetCurrentShader(shader);
	modelMatrix.ToIdentity();
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"),0);
	DrawNode(root);
	glUseProgram(0);
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 LocalTransform = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, (float*)& LocalTransform);
		glUniform4fv(glGetUniformLocation(currentShader->GetProgram(), "nodeColour"), 1, (float*)& n->GetColour());
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), (int)n->GetMesh()->GetTexture());
		n->Draw(*this);
	}

	for (vector<SceneNode*>::const_iterator it = n->GetStartChild(); it != n->GetEndChild(); it++) {
		DrawNode(*it);
	}
}

void Renderer::DrawSkybox() {
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();
	glUseProgram(0);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
}


void Renderer::DrawHeightmap() {

	SetCurrentShader(lightShader);
	
	SetShaderLight(*light);
	SetShaderSpotlight(*spotlight);

	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "hmMod"), time);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "mixer"), mixer);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "spotLightModX"), spotLightModX);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "spotLightModZ"), spotLightModZ);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "secondTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 2);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "secondBumpTex"), 4);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 5);

	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	modelMatrix.ToIdentity();

	Matrix4 tempMatrix = textureMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *&tempMatrix.values);
	UpdateShaderMatrices();
	heightMap->Draw();
	glUseProgram(0);
}

void Renderer::DrawWater() {
	SetCurrentShader(reflectShader);
	SetShaderLight(*light);
	SetShaderSpotlight(*spotlight);

	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "spotLightModX"), spotLightModX);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "spotLightModZ"), spotLightModZ);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex"), 3);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "waterTransparency"), mixer);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	float heightX = (RAW_WIDTH * HEIGHTMAP_X / 2.0f);
	float heightY = 256 * HEIGHTMAP_Y / 3.7f;
	float heightZ = (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f);

	modelMatrix =
		Matrix4::Translation(Vector3((3.0f/2.0f)*heightX , heightY, heightZ/1.25)) *
		Matrix4::Scale(Vector3(heightX /4, 1, heightZ / 3.5)) *
		Matrix4::Rotation(90.0f, Vector3(1.0f, 0, 0));

	textureMatrix = Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) * Matrix4::Rotation(waterRotate, Vector3(0.0, 0.0, 1.0));

	UpdateShaderMatrices();

	quad->Draw();

	glUseProgram(0);

}

void Renderer::SetUpPostProcess() {
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &processFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) {

		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SetUpShadows() {
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

	glDrawBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawHellKnight(Shader* shader) {
	SetCurrentShader(shader);
	SetShaderLight(*light);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 2);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 5);

	modelMatrix = Matrix4::Translation(Vector3(2020.5,35,4500.1))*Matrix4::Scale(Vector3(3,3,3))*Matrix4::Rotation(180,Vector3(0,1,0));
	Matrix4 tempMatrix = textureMatrix * modelMatrix;
	UpdateShaderMatrices();
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *&tempMatrix.values);
	hellNode->Draw(*this);

	modelMatrix = Matrix4::Translation(Vector3(2680.5, 30, 4100.1)) * Matrix4::Scale(Vector3(3, 3, 3)) * Matrix4::Rotation(100, Vector3(0, 1, 0));
	tempMatrix = textureMatrix * modelMatrix;
	UpdateShaderMatrices();
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *&tempMatrix.values);
	hellNode->Draw(*this);

	glUseProgram(0);
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0,0,SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);


	viewMatrix = Matrix4::BuildViewMatrix(light->getPosition(), Vector3(2020.5, 100, 4500.1));
	textureMatrix = biasMatrix * (projMatrix * viewMatrix);

	DrawHellKnight(hkShadowShader);
	DrawTree(hkShadowShader);

	glUseProgram(0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawCombinedScene() {
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	viewMatrix = camera->BuildViewMatrix();
	DrawHeightmap();
	DrawWater();
	DrawHellKnight(hkShader);
	DrawTree(treeShader);

	glUseProgram(0);
}



void Renderer::SwitchCamera(int pos) {

	switch (pos) {
	case 0:
		break;
	case 1:
		camera->SetPitch(pos1.pitch);
		camera->SetYaw(pos1.yaw);
		camera->SetPosition(pos1.position);
		break;
	case 2:
		camera->SetPitch(pos2.pitch);
		camera->SetYaw(pos2.yaw);
		camera->SetPosition(pos2.position);
		break;
	case 3:
		camera->SetPitch(pos3.pitch);
		camera->SetYaw(pos3.yaw);
		camera->SetPosition(pos3.position);
		break;
	case 4:
		camera->SetPitch(pos4.pitch);
		camera->SetYaw(pos4.yaw);
		camera->SetPosition(pos4.position);
		break;
	default:
		break;
	}
}


void Renderer::AutoCameraPath(float &timer) {
	float lerpVal;
	Vector3 currentPos;
	Vector3 nextPos;
	float currentYaw;
	float nextYaw;
	float currentPitch;
	float nextPitch;


	if ((int)(timer) % 4 + 1 == 1) {
		float lerpVal = timer-(int)timer;
		currentPos = pos1.position;
		nextPos = pos2.position;
		currentYaw = pos1.yaw;
		nextYaw = pos2.yaw;
		currentPitch = pos1.pitch;
		nextPitch = pos2.pitch;

		camera->SetPosition(Vector3(lerp(currentPos.x, nextPos.x, lerpVal), lerp(currentPos.y, nextPos.y, lerpVal), lerp(currentPos.z, nextPos.z, lerpVal)));
		camera->SetYaw(lerp(currentYaw, nextYaw, lerpVal));
		camera->SetPitch(lerp(currentPitch, nextPitch, lerpVal));
	}

	else if ((int)(timer) % 4 + 1 == 2) {
		lerpVal = timer - (int)timer;
		currentPos = pos2.position;
		nextPos = pos3.position;
		currentYaw = pos2.yaw;
		nextYaw = pos3.yaw;
		currentPitch = pos2.pitch;
		nextPitch = pos3.pitch;

		camera->SetPosition(Vector3(lerp(currentPos.x, nextPos.x, lerpVal), lerp(currentPos.y, nextPos.y, lerpVal), lerp(currentPos.z, nextPos.z, lerpVal)));
		camera->SetYaw(lerp(currentYaw, nextYaw, lerpVal));
		camera->SetPitch(lerp(currentPitch, nextPitch, lerpVal));

	}

	else if ((int)(timer) % 4 + 1 == 3) {
		lerpVal = timer - (int)timer;
		currentPos = pos3.position;
		nextPos = pos4.position;
		currentYaw = pos3.yaw;
		nextYaw = pos4.yaw;
		currentPitch = pos3.pitch;
		nextPitch = pos4.pitch;

		camera->SetPosition(Vector3(lerp(currentPos.x, nextPos.x, lerpVal), lerp(currentPos.y, nextPos.y, lerpVal), lerp(currentPos.z, nextPos.z, lerpVal)));
		camera->SetYaw(lerp(currentYaw, nextYaw, lerpVal));
		camera->SetPitch(lerp(currentPitch, nextPitch, lerpVal));

	}
	else if ((int)(timer)%4 +1 == 4) {
		lerpVal = timer - (int)timer;
		currentPos = pos4.position;
		nextPos = pos1.position;
		currentYaw = pos4.yaw;
		nextYaw = 360;
		currentPitch = pos4.pitch;
		nextPitch = pos1.pitch;

		camera->SetPosition(Vector3(lerp(currentPos.x, nextPos.x, lerpVal), lerp(currentPos.y, nextPos.y, lerpVal), lerp(currentPos.z, nextPos.z, lerpVal)));
		camera->SetYaw(lerp(currentYaw, nextYaw, lerpVal));
		camera->SetPitch(lerp(currentPitch, nextPitch, lerpVal));
	}
}