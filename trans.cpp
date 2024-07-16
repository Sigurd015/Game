//=============================================================================
//
// エフェクト処理 [effect.cpp]
// Author :
//
//=============================================================================
#include "trans.h"
#include "bg.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************

#define EFFECT_TEXTURE_SIZE_X (100) // テクスチャサイズ
#define EFFECT_TEXTURE_SIZE_Y (100) // 同上

#define EFFECT_TEXTURE_PATTERN_DIVIDE_X (7)															// アニメパターンのテクスチャ内分割数（X)
#define EFFECT_TEXTURE_PATTERN_DIVIDE_Y (1)															// アニメパターンのテクスチャ内分割数（Y)
#define EFFECT_ANIM_PATTERN_NUM (EFFECT_TEXTURE_PATTERN_DIVIDE_X * EFFECT_TEXTURE_PATTERN_DIVIDE_Y) // アニメーションパターン数
#define EFFECT_TIME_ANIMATION (1)																	// アニメーションの切り替わるカウント

#define EMISSION_FULL 0 // パーティクル全生成フラグ
#define EMISSION_RATE 5 // パーティクルの生成間隔(duration/EMISSION_RATEの数分エフェクトが出る)

#define EMISSION_WIDTH 50  // パーティクル生成範囲（横幅）
#define EMISSION_HEIGHT 50 // パーティクル生成範囲（高さ）

#define TEXTURE_MAX (1) // テクスチャの数

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static ID3D11Buffer *g_VertexBuffer = NULL;						  // 頂点情報
static ID3D11ShaderResourceView *g_Texture[TEXTURE_MAX] = {NULL}; // テクスチャ情報

static char *g_TexturName[] = {
	"data/TEXTURE/effect01.png",
};

static BOOL g_Load = FALSE;				   // 初期化を行ったかのフラグ
static TRANS effectWk[EFFECT_NUM]; // エフェクト構造体

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitTrans(void)
{
	ID3D11Device *pDevice = GetDevice();

	// テクスチャ生成
	for (int i = 0; i < TEXTURE_MAX; i++)
	{
		g_Texture[i] = NULL;
		D3DX11CreateShaderResourceViewFromFile(GetDevice(),
											   g_TexturName[i],
											   NULL,
											   NULL,
											   &g_Texture[i],
											   NULL);
	}

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	GetDevice()->CreateBuffer(&bd, NULL, &g_VertexBuffer);

	// 初期化処理
	for (int i = 0; i < EFFECT_NUM; i++)
	{
		effectWk[i].use = false;
		effectWk[i].elapsed = 0;
	}

	g_Load = TRUE;
	return S_OK;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitTrans(void)
{
	if (g_Load == FALSE)
		return;

	if (g_VertexBuffer)
	{
		g_VertexBuffer->Release();
		g_VertexBuffer = NULL;
	}

	for (int i = 0; i < TEXTURE_MAX; i++)
	{
		if (g_Texture[i])
		{
			g_Texture[i]->Release();
			g_Texture[i] = NULL;
		}
	}
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdateTrans(void)
{
	for (int i = 0; i < EFFECT_NUM; i++)
	{
		if (effectWk[i].use)
		{
			if (effectWk[i].isEnding)
			{
				effectWk[i].use = FALSE;
				continue;
			}

			// エミットが有効であればエフェクト作成処理を行う
			if (effectWk[i].isRemoveOnFinish == FALSE)
			{

				//// エフェクト作成レートの増加処理
				//if (effectWk[i].effectCount < EFFECT_NUM_PARTS)
				//	effectWk[i].emitCounter++;

				//// バッファに空きがあり、追加タイミングが来ていれば新たなエフェクトを追加する
				//while ((effectWk[i].effectCount < EFFECT_NUM_PARTS) && (effectWk[i].emitCounter > EMISSION_RATE))
				//{
				//	// 全体追加フラグがONであれば空き領域全てに新たなエフェクトを追加する
				//	if (EMISSION_FULL)
				//		effectWk[i].effectCount = EFFECT_NUM_PARTS;
				//	else
				//		effectWk[i].effectCount++;

				//	// エフェクト作成レートの初期化
				//	effectWk[i].emitCounter = 0;
				//}

				effectWk[i].elapsed++;

				// 時間超過
				if ((effectWk[i].duration != -1) && (effectWk[i].duration < effectWk[i].elapsed))
				{
					effectWk[i].isRemoveOnFinish = TRUE;
				}
			}
		}
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawTrans(void)
{
	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	GetDeviceContext()->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);

	// マトリクス設定
	SetWorldViewProjection2D();

	// プリミティブトポロジ設定
	GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// マテリアル設定
	MATERIAL material;
	ZeroMemory(&material, sizeof(material));
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);

	BG *bg = GetBG();

	for (int i = 0; i < EFFECT_NUM; i++)
	{
		if (effectWk[i].use == TRUE) // このエフェクトが使われている？
		{							 // Yes
			// テクスチャ設定
			GetDeviceContext()->PSSetShaderResources(0, 1, &g_Texture[0]);

			//for (int n = 0; n < effectWk[i].effectCount; n++)
			//{
			//	if (effectWk[i].pParticle[n].isFinish == 0)
			//	{
			//		// バレットの位置やテクスチャー座標を反映
			//		float px = effectWk[i].pParticle[n].pos.x - bg->pos.x; // エフェクトの表示位置X
			//		float py = effectWk[i].pParticle[n].pos.y - bg->pos.y; // エフェクトの表示位置Y
			//		float pw = EFFECT_TEXTURE_SIZE_X;					   // エフェクトの表示幅
			//		float ph = EFFECT_TEXTURE_SIZE_Y;					   // エフェクトの表示高さ

			//		px -= EFFECT_TEXTURE_SIZE_X / 4;
			//		py -= EFFECT_TEXTURE_SIZE_Y / 4;

			//		float tw = 1.0f / EFFECT_TEXTURE_PATTERN_DIVIDE_X;												 // テクスチャの幅
			//		float th = 1.0f / EFFECT_TEXTURE_PATTERN_DIVIDE_Y;												 // テクスチャの高さ
			//		float tx = (float)(effectWk[i].pParticle[n].PatternAnim % EFFECT_TEXTURE_PATTERN_DIVIDE_X) * tw; // テクスチャの左上X座標
			//		float ty = (float)(effectWk[i].pParticle[n].PatternAnim / EFFECT_TEXTURE_PATTERN_DIVIDE_X) * th; // テクスチャの左上Y座標

			//		// １枚のポリゴンの頂点とテクスチャ座標を設定
			//		SetSpriteColorRotation(g_VertexBuffer,
			//							   px, py, pw, ph,
			//							   tx, ty, tw, th,
			//							   XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			//							   0.0f);

			//		// ポリゴン描画
			//		GetDeviceContext()->Draw(4, 0);
			//	}
			//}
		}
	}
}

//=============================================================================
// エフェクトのセット
//
// int duration		エフェクト発生源の生存時間
//=============================================================================
void SetTrans(float x, float y, int duration)
{
	// もし未使用のエフェクトが無かったら実行しない( =これ以上表示できないって事 )
	for (int i = 0; i < EFFECT_NUM; i++)
	{
		if (effectWk[i].use == false) // 未使用状態のエフェクトを見つける
		{
			effectWk[i].use = true;
			effectWk[i].isEnding = false;
			effectWk[i].isRemoveOnFinish = FALSE;

			effectWk[i].duration = duration;
			effectWk[i].pos.x = x; // 座標をセット
			effectWk[i].pos.y = y; // 座標をセット

			effectWk[i].effectCount = 0;
			effectWk[i].elapsed = 0;
			effectWk[i].emitCounter = EMISSION_RATE;
			effectWk[i].numFinish = 0;

			return; // 1個セットしたので終了する
		}
	}
}
