// ============================================================
// GPU Skinning System — path de performance (RTX / GL 3.3)
// Usa skinning.vs/.fs (CShaderGL). Uniform mat4 BoneMatrices[200].
//
// Fluxo:
//   Animation() → bones na CPU
//   Transform() → normals/luz na CPU; VÉRTICES pulados (GPU)
//   RenderVertexBufferSkinning() → VBO_BindPose + bones no VS
// ============================================================
#pragma once

#define GPU_SKINNING_MAX_BONES 200
#define GPU_SKINNING_MAX_MESH  64

class CGPUSkinning
{
public:
	static CGPUSkinning& Get();

	bool Init();
	void Shutdown();
	bool IsReady() const { return m_ready; }

	// Converte float[count][3][4] → mat4 column-major (sempre, sem cache por ponteiro)
	void UploadBones(float (*bones)[3][4], int count);

	// true se shader ativo (glUseProgram já chamado). false = GL limpo.
	// Lê modelview/projection no máximo 1x por epoch (BeginRender do BMD).
	bool BeginRender(int uMode, float alpha, bool translate, float scale, float* origin);
	void EndRender();

	// Restaura estado safe p/ UI fixed-function (RenderBitmap, fonte, HP bar, drops)
	static void ResetFixedFunctionState();

	void InvalidateMatrices()
	{
		m_matrixEpoch = -1;
	}

	// Após Transform(): se true, VertexTransform[] NÃO foi preenchido
	void SetVertsSkipped(bool skipped) { m_vertsSkipped = skipped; }
	bool VertsSkipped() const { return m_vertsSkipped; }

	void ResetMeshReadyFlags()
	{
		memset(m_meshVertsReady, 0, sizeof(m_meshVertsReady));
	}
	bool IsMeshVertsReady(int mesh) const
	{
		if (mesh < 0 || mesh >= GPU_SKINNING_MAX_MESH) return false;
		return m_meshVertsReady[mesh] != 0;
	}
	void SetMeshVertsReady(int mesh, bool ready)
	{
		if (mesh < 0 || mesh >= GPU_SKINNING_MAX_MESH) return;
		m_meshVertsReady[mesh] = ready ? 1 : 0;
	}

	// Pode desenhar com GPU? (shader + ossos)
	bool CanGpuDraw() const
	{
		return m_ready && m_boneCount > 0;
	}

	int BoneCount() const { return m_boneCount; }

	// true se ossos já foram enviados neste ciclo (Transform/Upload)
	bool BonesUploaded() const { return m_bonesUploaded; }

private:
	CGPUSkinning()
		: m_ready(false)
		, m_boneCount(0)
		, m_matrixEpoch(-1)
		, m_vertsSkipped(false)
		, m_bonesUploaded(false)
		, m_bonesDirty(false)
		, m_locModel(-1), m_locView(-1), m_locProj(-1)
		, m_locBones(-1), m_locTranslate(-1), m_locBodyScale(-1)
		, m_locBodyOrigin(-1), m_locMode(-1), m_locAlpha(-1), m_locTex(-1)
		, m_cachedShader(0)
	{
		memset(m_boneData, 0, sizeof(m_boneData));
		memset(m_cachedModelView, 0, sizeof(m_cachedModelView));
		memset(m_cachedProjection, 0, sizeof(m_cachedProjection));
		memset(m_meshVertsReady, 0, sizeof(m_meshVertsReady));
		// Identidade em todos os slots (ossos não usados não explodem o mesh)
		for (int b = 0; b < GPU_SKINNING_MAX_BONES; ++b)
		{
			float* d = &m_boneData[b * 16];
			d[0] = d[5] = d[10] = d[15] = 1.0f;
		}
	}
	~CGPUSkinning() { Shutdown(); }

	void CacheLocations(GLuint shader_id);

	bool  m_ready;
	int   m_boneCount;
	int   m_matrixEpoch;
	bool  m_vertsSkipped;
	bool  m_bonesUploaded;
	bool  m_bonesDirty; // true apos UploadBones ate o 1o glUniform no objeto
	unsigned char m_meshVertsReady[GPU_SKINNING_MAX_MESH];

	GLint m_locModel, m_locView, m_locProj;
	GLint m_locBones, m_locTranslate, m_locBodyScale, m_locBodyOrigin;
	GLint m_locMode, m_locAlpha, m_locTex;
	GLuint m_cachedShader;

	GLfloat m_cachedModelView[16];
	GLfloat m_cachedProjection[16];
	float m_boneData[GPU_SKINNING_MAX_BONES * 16];

	static CGPUSkinning s_instance;
};
