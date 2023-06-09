#include "GameScene.h"
#include "AxisIndicator.h"
#include "ImGuiManager.h"
#include "PrimitiveDrawer.h"
#include "TextureManager.h"
#include <cassert>

GameScene::GameScene() {}

GameScene::~GameScene() {}

void GameScene::Initialize() {

	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	m_fighterModel.reset(Model::CreateFromOBJ("fighter"));
	m_bulletModel.reset(Model::CreateFromOBJ("bullet"));
	m_skydomeModel.reset(Model::CreateFromOBJ("skydome"));
	m_cubeModel.reset(Model::CreateFromOBJ("cube"));
	TextureManager::Load("Reticle.png");

	m_collisionManager = std::make_unique<CollisionManager>();
	// レールカメラ
	m_railCamera = std::make_unique<RailCamera>();
	m_railCamera->Initalize({0.0f, 0.0f, -50.0f}, {});
	// プレイヤー
	m_player = std::make_shared<Player>();
	m_player->SetParent(&m_railCamera->GetWorldTransform());
	m_player->Initalize(
	    m_fighterModel, m_bulletModel, TextureManager::Load("ziki.png"));
	// 敵の弾
	m_enemyBulletManager = std::make_shared<EnemyBulletManager>();
	m_enemyBulletManager->SetPlayer(m_player);
	m_enemyBulletManager->Initalize(m_bulletModel);
	// 敵
	m_enemyManager = std::make_unique<EnemyManager>();
	m_enemyManager->SetPlayer(m_player);
	m_enemyManager->SetBulletManager(m_enemyBulletManager);
	m_enemyManager->Initalize(m_fighterModel);

	m_skydome = std::make_unique<Skydome>();
	m_skydome->Initalize(m_skydomeModel);

	// #ifdef _DEBUG
	m_debugCamera = std::make_unique<DebugCamera>(
	    dxCommon_->GetBackBufferWidth(), dxCommon_->GetBackBufferHeight());
	m_debugCamera;
	m_isDebugCameraActive = false;

	m_curViewProj = &m_railCamera->GetViewProjection();

	AxisIndicator::GetInstance()->SetVisible(true);
	AxisIndicator::GetInstance()->SetTargetViewProjection(m_curViewProj);

	PrimitiveDrawer::GetInstance()->SetViewProjection(m_curViewProj);

	// #endif
}

void GameScene::Update() {

	m_railCamera->Update();
	m_player->Update(*m_curViewProj);
	m_enemyManager->Update();
	m_enemyBulletManager->Update();
	m_skydome->Update();

	// コライダーをクリア
	m_collisionManager->ClearColliders();
	m_collisionManager->AddCollider(m_player.get());
	for (auto& enemy : m_enemyManager->GetEnemys()) {
		m_collisionManager->AddCollider(enemy.get());
	}
	for (auto& bullet : m_player->GetBullets()) {
		m_collisionManager->AddCollider(bullet.get());
	}
	for (auto& bullet : m_enemyBulletManager->GetBullets()) {
		m_collisionManager->AddCollider(bullet.get());
	}
	// すべてのコライダーの当たり判定を取る
	m_collisionManager->CheckAllCollisions();
	m_collisionManager->ClearColliders();

	// #ifdef _DEBUG
	if (input_->TriggerKey(DIK_TAB)) {
		m_isDebugCameraActive ^= true;
	}
	if (m_isDebugCameraActive) {
		m_debugCamera->Update();
		m_curViewProj = &m_debugCamera->GetViewProjection();
	} else {
		m_curViewProj = &m_railCamera->GetViewProjection();
	}

#pragma region デバッグ用ImGuiウィンドウ
	ImGui::SetNextWindowPos({0, 520}, ImGuiCond_Once);
	ImGui::SetNextWindowSize({300, 200}, ImGuiCond_Once);
	ImGui::Begin("Debug");
	ImGui::Checkbox("DebugCamera", &m_isDebugCameraActive);
	static bool axisVisible = true;
	ImGui::Checkbox("AxisVisible", &axisVisible);
	AxisIndicator::GetInstance()->SetVisible(axisVisible);
	ImGui::End();
#pragma endregion
	// #endif
}

void GameScene::Draw() {

	// コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#pragma region 背景スプライト描画
	// 背景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに背景スプライトの描画処理を追加できる
	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();
	// 深度バッファクリア
	dxCommon_->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
	// 3Dオブジェクト描画前処理
	Model::PreDraw(commandList);

	/// <summary>
	/// ここに3Dオブジェクトの描画処理を追加できる
	/// </summary>
	m_player->Draw(*m_curViewProj);
	m_enemyManager->Draw(*m_curViewProj);
	m_enemyBulletManager->Draw(*m_curViewProj);
	m_skydome->Draw(*m_curViewProj);
	m_railCamera->Draw();

	// 3Dオブジェクト描画後処理
	Model::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
	// 前景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに前景スプライトの描画処理を追加できる
	/// </summary>

	m_player->DrawUI();

	// スプライト描画後処理
	Sprite::PostDraw();

#pragma endregion
}
