// ============================================================
// GPU Skinning — implementação de performance (path full)
// ============================================================
#include "stdafx.h"
#include "GPUSkinning.h"
#include "CShaderGL.h"
#include "ZzzBMD.h"
#include "ZzzOpenglUtil.h"

CGPUSkinning CGPUSkinning::s_instance;

CGPUSkinning& CGPUSkinning::Get()
{
	return s_instance;
}

void CGPUSkinning::CacheLocations(GLuint shader_id)
{
	if (m_cachedShader == shader_id)
		return;

	m_cachedShader   = shader_id;
	m_locModel       = glGetUniformLocation(shader_id, "model");
	m_locView        = glGetUniformLocation(shader_id, "view");
	m_locProj        = glGetUniformLocation(shader_id, "projection");
	m_locBones       = glGetUniformLocation(shader_id, "BoneMatrices");
	m_locTranslate   = glGetUniformLocation(shader_id, "uTranslate");
	m_locBodyScale   = glGetUniformLocation(shader_id, "BodyScale");
	m_locBodyOrigin  = glGetUniformLocation(shader_id, "BodyOrigin");
	m_locMode        = glGetUniformLocation(shader_id, "uMode");
	m_locAlpha       = glGetUniformLocation(shader_id, "Alpha");
	m_locTex         = glGetUniformLocation(shader_id, "texture1");
}

bool CGPUSkinning::Init()
{
	if (m_ready)
		return true;

	GLuint skinId = gShaderGL->GetShaderSkinningId();
	if (skinId == 0)
	{
		OutputDebugStringA("[GPUSkinning] shader_skinning_id == 0, aguardando CShaderGL...\n");
		return false;
	}

	CacheLocations(skinId);
	m_boneCount     = 0;
	m_bonesUploaded = false;
	m_bonesDirty    = false;
	m_ready         = true;

	OutputDebugStringA("[GPUSkinning] OK — personagem/set animado na GPU; mapa/item classic\n");
	return true;
}

void CGPUSkinning::Shutdown()
{
	m_ready         = false;
	m_boneCount     = 0;
	m_bonesUploaded = false;
	m_bonesDirty    = false;
	m_cachedShader  = 0;
}

void CGPUSkinning::UploadBones(float (*bones)[3][4], int count)
{
	if (!bones || count <= 0)
	{
		m_bonesUploaded = false;
		m_boneCount     = 0;
		m_bonesDirty    = false;
		return;
	}

	// Sempre reconverte — BoneTransform global e reescrito por objeto
	m_boneCount = (count > GPU_SKINNING_MAX_BONES) ? GPU_SKINNING_MAX_BONES : count;

	for (int b = 0; b < m_boneCount; b++)
	{
		float* dst = &m_boneData[b * 16];

		dst[0]  = bones[b][0][0];
		dst[1]  = bones[b][1][0];
		dst[2]  = bones[b][2][0];
		dst[3]  = 0.0f;

		dst[4]  = bones[b][0][1];
		dst[5]  = bones[b][1][1];
		dst[6]  = bones[b][2][1];
		dst[7]  = 0.0f;

		dst[8]  = bones[b][0][2];
		dst[9]  = bones[b][1][2];
		dst[10] = bones[b][2][2];
		dst[11] = 0.0f;

		dst[12] = bones[b][0][3];
		dst[13] = bones[b][1][3];
		dst[14] = bones[b][2][3];
		dst[15] = 1.0f;
	}

	m_bonesUploaded = true;
	m_bonesDirty    = true; // 1o mesh do objeto envia; demais meshes reusam
}

bool CGPUSkinning::BeginRender(int uMode, float alpha, bool translate, float scale, float* origin)
{
	if (!m_ready)
	{
		if (!Init())
			return false;
	}

	if (!m_bonesUploaded || m_boneCount <= 0)
		return false;

	GLuint shader_id = gShaderGL->GetShaderSkinningId();
	if (shader_id == 0)
		return false;

	CacheLocations(shader_id);
	glUseProgram(shader_id);

	// 1x por objeto (g_GpuMatrixEpoch incrementa em BMD::BeginRender)
	extern int g_GpuMatrixEpoch;
	if (m_matrixEpoch != g_GpuMatrixEpoch)
	{
		glGetFloatv(GL_MODELVIEW_MATRIX,  m_cachedModelView);
		glGetFloatv(GL_PROJECTION_MATRIX, m_cachedProjection);
		m_matrixEpoch = g_GpuMatrixEpoch;
	}

	static const float s_identity[16] = {
		1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
	};

	if (m_locModel != -1) glUniformMatrix4fv(m_locModel, 1, GL_FALSE, s_identity);
	if (m_locView  != -1) glUniformMatrix4fv(m_locView,  1, GL_FALSE, m_cachedModelView);
	if (m_locProj  != -1) glUniformMatrix4fv(m_locProj,  1, GL_FALSE, m_cachedProjection);

	// Bones: so no 1o mesh apos UploadBones (mesmo personagem = varios meshes)
	if (m_bonesDirty && m_locBones != -1)
	{
		glUniformMatrix4fv(m_locBones, m_boneCount, GL_FALSE, m_boneData);
		m_bonesDirty = false;
	}

	if (m_locTranslate  != -1) glUniform1i(m_locTranslate, translate ? 1 : 0);
	if (m_locBodyScale  != -1) glUniform1f(m_locBodyScale, scale);
	if (m_locBodyOrigin != -1 && origin) glUniform3fv(m_locBodyOrigin, 1, origin);

	if (m_locMode  != -1) glUniform1i(m_locMode,  uMode);
	if (m_locAlpha != -1) glUniform1f(m_locAlpha, alpha);
	if (m_locTex   != -1) glUniform1i(m_locTex,   0);

	return true;
}

void CGPUSkinning::ResetFixedFunctionState()
{
	// Usar SO na troca 3D -> UI (BeginBitmap). NAO chamar a cada mesh (mata FPS).
	glBindVertexArray(0);
	glUseProgram(0);

	for (int i = 0; i < 4; ++i)
		glDisableVertexAttribArray(i);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glActiveTexture(GL_TEXTURE0);
	// NAO mexer em glTexParameteri aqui — forcar MIPMAP em textura sem mips
	// deixa itens/UI serrilhados ou pretos.
}

void CGPUSkinning::EndRender()
{
	// LEVE: desliga program/VAO.
	// Proximo mesh classic (fixed-function) ou UI precisa de attribs limpos.
	glBindVertexArray(0);
	glUseProgram(0);
	// Sem reset completo aqui (custa FPS se em todo mesh).
	// Classic path e BeginBitmap chamam ResetFixedFunctionState quando precisam.
}
