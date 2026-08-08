///////////////////////////////////////////////////////////////////////////////
// Terrain ���� �Լ�
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <math.h>
#include <algorithm>
#include <climits>
using std::min;
#include "ZzzOpenglUtil.h"
#include "ZzzBMD.h"
#include "ZzzLodTerrain.h"
#include "zzzpath.h"
#include "ZzzTexture.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzEffect.h"
#include "./Utilities/Log/ErrorReport.h"
#include "CSChaosCastle.h"
#include "GMBattleCastle.h"
#include "CMVP1stDirection.h"
#include "CDirection.h"
#include "MapManager.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include "w_MapHeaders.h"
#include "NewUISystem.h"
#include "GMKarutan1.h"
#include "CameraManager.h"
#include "CShaderGL.h"

#ifdef PJH_NEW_SERVER_SELECT_MAP
#include "CameraMove.h"
#endif //PJH_NEW_SERVER_SELECT_MAP
#include "CGMProtect.h"
#include "MuCrypto/MuCrypto.h"


//-------------------------------------------------------------------------------------------------------------

// =============================================================================
// FIX PASSO 3.5: Terreno convertido de glBegin/glEnd + glEnableClientState
// para glVertexAttribPointer + glDrawArrays (OpenGL 3.3 Core Profile)
// =============================================================================

inline void DrawTerrainFan4(
    float* v0, float* v1, float* v2, float* v3,
    float* t0, float* t1, float* t2, float* t3,
    float* c0, float* c1, float* c2, float* c3,
    bool alpha = false)
{
    float verts[4][3] = {
        {v0[0], v0[1], v0[2]},
        {v1[0], v1[1], v1[2]},
        {v2[0], v2[1], v2[2]},
        {v3[0], v3[1], v3[2]}
    };
    float texs[4][2] = {
        {t0[0], t0[1]}, {t1[0], t1[1]}, {t2[0], t2[1]}, {t3[0], t3[1]}
    };
    float cols[4][4] = {
        {c0[0], c0[1], c0[2], alpha ? c0[3] : 1.0f},
        {c1[0], c1[1], c1[2], alpha ? c1[3] : 1.0f},
        {c2[0], c2[1], c2[2], alpha ? c2[3] : 1.0f},
        {c3[0], c3[1], c3[2], alpha ? c3[3] : 1.0f}
    };

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texs);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, cols);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

int  TerrainFlag;
bool ActiveTerrain = false;
bool TerrainGrassEnable = true;
bool DetailLowEnable = false;

char            LodBuffer[64 * 64];
vec3_t          TerrainNormal[TERRAIN_SIZE * TERRAIN_SIZE];
vec3_t          PrimaryTerrainLight[TERRAIN_SIZE * TERRAIN_SIZE];
vec3_t          BackTerrainLight[TERRAIN_SIZE * TERRAIN_SIZE];
vec3_t          TerrainLight[TERRAIN_SIZE * TERRAIN_SIZE];
float           PrimaryTerrainHeight[TERRAIN_SIZE * TERRAIN_SIZE];
float           BackTerrainHeight[TERRAIN_SIZE * TERRAIN_SIZE];
unsigned char   TerrainMappingLayer1[TERRAIN_SIZE * TERRAIN_SIZE];
unsigned char   TerrainMappingLayer2[TERRAIN_SIZE * TERRAIN_SIZE];
float           TerrainMappingAlpha[TERRAIN_SIZE * TERRAIN_SIZE];
float           TerrainGrassTexture[TERRAIN_SIZE * TERRAIN_SIZE];
float           TerrainGrassWind[TERRAIN_SIZE * TERRAIN_SIZE];
float			g_fTerrainGrassWind1[TERRAIN_SIZE * TERRAIN_SIZE];

WORD            TerrainWall[TERRAIN_SIZE * TERRAIN_SIZE];

float           SelectXF;
float           SelectYF;
float           WaterMove;
int             CurrentLayer;

float           g_fSpecialHeight = 1200.f;



bool  SelectFlag;

int    TerrainIndex1;
int    TerrainIndex2;
int    TerrainIndex3;
int    TerrainIndex4;
int    Index0;
int    Index1;
int    Index2;
int    Index3;
int    Index01;
int    Index12;
int    Index23;
int    Index30;
int    Index02;
vec3_t TerrainVertex[4];
vec3_t TerrainVertex01;
vec3_t TerrainVertex12;
vec3_t TerrainVertex23;
vec3_t TerrainVertex30;
vec3_t TerrainVertex02;
float  TerrainTextureCoord[4][2];
float  TerrainTextureCoord01[2];
float  TerrainTextureCoord12[2];
float  TerrainTextureCoord23[2];
float  TerrainTextureCoord30[2];
float  TerrainTextureCoord02[2];
float  TerrainMappingAlpha01;
float  TerrainMappingAlpha12;
float  TerrainMappingAlpha23;
float  TerrainMappingAlpha30;
float  TerrainMappingAlpha02;

#ifdef DYNAMIC_FRUSTRUM
FrustrumMap_t	g_FrustrumMap;
#endif //DYNAMIC_FRUSTRUM

const float g_fMinHeight = -500.f;
const float g_fMaxHeight = 1000.f;

extern  short   g_shCameraLevel;
extern  float CameraDistanceTarget;
extern  float CameraDistance;

static  float   g_fFrustumRange = -40.f;

// Bias global de renderização (port do 6.Main_UP52_Full).
// Valores negativos aumentam o alcance de TODOS os testes de frustum 2D de forma
// uniforme (terreno, blocos e objetos), eliminando o pop quando câmera rotaciona.
// NÃO expande FrustrumBound* => InitTerrainLight/água/vento custam o mesmo.
float   g_fRenderDistanceBias = -80.f;

float GetTerrainFrustumRange()
{
	static float sRange = 999.f;
	if (sRange == 999.f)
	{
		int val = GetPrivateProfileIntA("Performance2", "FrustumRange", -40, ".\\config.ini");
		sRange = (float)val;
	}
	return sRange;
}


inline int TERRAIN_INDEX(int x, int y)
{
	return (y)*TERRAIN_SIZE + (x);
}

inline int TERRAIN_INDEX_REPEAT(int x, int y)
{
	return (y & TERRAIN_SIZE_MASK) * TERRAIN_SIZE + (x & TERRAIN_SIZE_MASK);
}

inline WORD TERRAIN_ATTRIBUTE(float x, float y)
{
	int xf = (int)(x / TERRAIN_SCALE);
	int yf = (int)(y / TERRAIN_SCALE);
	return TerrainWall[(yf)*TERRAIN_SIZE + (xf)];
}

void InitTerrainMappingLayer()
{
	for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
	{
		TerrainMappingLayer1[i] = 0;
		TerrainMappingLayer2[i] = 255;
		TerrainMappingAlpha[i] = 0.f;
		TerrainGrassTexture[i] = (float)((rand() % 4) / 4.f);
	}
}

void ExitProgram()
{
	gwinhandle->SendWindowMessage(GlobalText[11], false);
	gwinhandle->SendNowMessage(WM_DESTROY, 0, 0);
}

int OpenTerrainAttribute(char* FileName)
{
	FILE* fp = fopen(FileName, "rb");

	if (fp == NULL)
	{
		return (-1);
	}

	fseek(fp, 0, SEEK_END);

	unsigned int FileSize = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	BYTE Enc[4];

	fread(&Enc, 4u, 1u, fp);

	std::vector<BYTE> FileBuffer;

	if (Enc[0] != 'A' || Enc[1] != 'T' || Enc[2] != 'T' || Enc[3] != 1)
	{
		BYTE* EncData = new BYTE[FileSize];

		fseek(fp, 0, SEEK_SET);

		fread(EncData, 1u, FileSize, fp);

		fclose(fp);

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		MapFileDecrypt(FileBuffer.data(), EncData, FileSize);

		SAFE_DELETE_ARRAY(EncData);
	}
	else
	{
		FileSize -= 4;

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		fread(FileBuffer.data(), 1, FileSize, fp);

		fclose(fp);

		MuFileDecrypt2(FileBuffer);
	}

	BYTE* blockchain = FileBuffer.data();
	FileSize = FileBuffer.size();

	if (FileSize != 131076 && FileSize != 65540)
	{
		return (-1);
	}

	bool extAtt = false;

	if (FileSize == 131076)
	{
		extAtt = true;
	}

	BYTE Version, Width, Height;

	BuxConvert(blockchain, FileSize);

	Version = blockchain[0];
	int iMap = blockchain[1];
	Width = blockchain[2];
	Height = blockchain[3];

	bool Error = false;

	if (Version || Width != 255 || Height != 255)
	{
		Error = true;
	}

	if (extAtt)
	{
		memcpy(TerrainWall, &blockchain[4], TERRAIN_SIZE * TERRAIN_SIZE * sizeof(WORD));
	}
	else
	{
		BYTE* TWall = new BYTE[TERRAIN_SIZE * TERRAIN_SIZE];
		memcpy(TWall, &blockchain[4], TERRAIN_SIZE * TERRAIN_SIZE);

		for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; ++i)
		{
			TerrainWall[i] = TWall[i];
		}
		SAFE_DELETE_ARRAY(TWall);
	}

	switch (gMapManager->currentMap)
	{
	case WD_0LORENCIA:
		if (TerrainWall[123 * 256 + 135] != 5)
			Error = true;
		break;
	case WD_1DUNGEON:
		if (TerrainWall[120 * 256 + 227] != 4)
			Error = true;
		break;
	case WD_2DEVIAS:
		if (TerrainWall[55 * 256 + 208] != 5)
			Error = true;
		break;
	case WD_3NORIA:
		if (TerrainWall[119 * 256 + 186] != 5)
			Error = true;
		break;
	case WD_4LOSTTOWER:
		if (TerrainWall[75 * 256 + 193] != 5)
			Error = true;
		break;
	}

	for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
	{
		TerrainWall[i] = TerrainWall[i] & 0xFF;
		if ((BYTE)TerrainWall[i] >= 128)
			Error = true;
	}

	if (Error)
	{
		ExitProgram();
		return (-1);
	}

	return iMap;
}

bool SaveTerrainAttribute(char* FileName, int iMap)
{
	FILE* fp = fopen(FileName, "wb");

	BYTE Version = 0;
	BYTE Width = 255;
	BYTE Height = 255;
	fwrite(&Version, 1, 1, fp);
	fwrite(&iMap, 1, 1, fp);
	fwrite(&Width, 1, 1, fp);
	fwrite(&Height, 1, 1, fp);
	fwrite(TerrainWall, TERRAIN_SIZE * TERRAIN_SIZE, 1, fp);

	fclose(fp);
	return true;
}

void AddTerrainAttribute(int x, int y, BYTE att)
{
	int iIndex = (x + (y * TERRAIN_SIZE));

	TerrainWall[iIndex] |= att;
}


void SubTerrainAttribute(int x, int y, BYTE att)
{
	int iIndex = (x + (y * TERRAIN_SIZE));

	TerrainWall[iIndex] ^= (TerrainWall[iIndex] & att);
}

void AddTerrainAttributeRange(int x, int y, int dx, int dy, BYTE att, BYTE Add)
{
	for (int j = 0; j < dy; ++j)
	{
		for (int i = 0; i < dx; ++i)
		{
			if (Add)
			{
				AddTerrainAttribute(x + i, y + j, att);
			}
			else
			{
				SubTerrainAttribute(x + i, y + j, att);
			}
		}
	}
}

void SetTerrainWaterState(std::list<int>& terrainIndex, int state)
{
	if (state == 0)
	{
		for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; ++i)
		{
			if ((TerrainWall[i] & TW_WATER) == TW_WATER)
			{
				TerrainWall[i] = 0;
				terrainIndex.push_back(i);
			}
		}
	}
	else
	{
		for (std::list<int>::iterator iter = terrainIndex.begin(); iter != terrainIndex.end(); )
		{
			std::list<int>::iterator curiter = iter;
			++iter;
			int index = *curiter;
			TerrainWall[index] = TW_WATER;
		}
	}
}

int SaveCameraAngle(char* FileName, BYTE* pbySrc, int iSize)
{
	if (pbySrc != NULL && iSize != 0)
	{
		FILE* fp = fopen(FileName, "wb");

		if (fp != NULL)
		{
			fwrite(pbySrc, iSize, 1, fp);
			fclose(fp);
			return (1);
		}
	}

	return (0);
}

int Open_Camera_Angle_Position(char* WorldName)
{
	char FileName[64];

	sprintf(FileName, "Data\\%s\\Camera_Angle_Position.bmd", WorldName);


	FILE* fp = fopen(FileName, "rb");

	if (fp == NULL)
	{
		return (0);
	}

	fseek(fp, 0, SEEK_END);

	unsigned int FileSize = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	BYTE Enc[4];

	fread(&Enc, 4u, 1u, fp);

	std::vector<BYTE> FileBuffer;

	if (Enc[0] == 'M' && Enc[1] == 'A' && Enc[2] == 'P' && Enc[3] == 1)
	{
		FileSize -= 4;

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		fread(FileBuffer.data(), 1, FileSize, fp);

		fclose(fp);

		MuFileDecrypt2(FileBuffer);
	}
	else
	{
		BYTE* EncData = new BYTE[FileSize];

		fseek(fp, 0, SEEK_SET);

		fread(EncData, 1u, FileSize, fp);

		fclose(fp);

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		MapFileDecrypt(FileBuffer.data(), EncData, FileSize);

		SAFE_DELETE_ARRAY(EncData);
	}

	sprintf(FileName, "Data\\%s\\CAP.ini", WorldName);

	return SaveCameraAngle(FileName, FileBuffer.data(), FileBuffer.size());
}

int OpenTerrainMapping(char* FileName)	//
{
	InitTerrainMappingLayer();


	FILE* fp = fopen(FileName, "rb");

	if (fp == NULL)
	{
		return (-1);
	}

	fseek(fp, 0, SEEK_END);

	unsigned int FileSize = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	BYTE Enc[4];

	fread(&Enc, 4u, 1u, fp);

	std::vector<BYTE> FileBuffer;

	if (Enc[0] != 'M' || Enc[1] != 'A' || Enc[2] != 'P' || Enc[3] != 1)
	{
		BYTE* EncData = new BYTE[FileSize];

		fseek(fp, 0, SEEK_SET);

		fread(EncData, 1u, FileSize, fp);

		fclose(fp);

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		MapFileDecrypt(FileBuffer.data(), EncData, FileSize);

		SAFE_DELETE_ARRAY(EncData);
	}
	else
	{
		FileSize -= 4;

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		fread(FileBuffer.data(), 1, FileSize, fp);

		fclose(fp);

		MuFileDecrypt2(FileBuffer);
	}


	int DataPtr = 0;
	BYTE Version = FileBuffer[0];
	int iMapNumber = FileBuffer[1];

	DataPtr = 2;

	memcpy(TerrainMappingLayer1, &FileBuffer[DataPtr], 256 * 256);
	DataPtr += (TERRAIN_SIZE * TERRAIN_SIZE);
	memcpy(TerrainMappingLayer2, &FileBuffer[DataPtr], 256 * 256);
	DataPtr += (TERRAIN_SIZE * TERRAIN_SIZE);

	for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
	{
		TerrainMappingAlpha[i] = ((float)FileBuffer[DataPtr++]) / 255.f;
	}

	TerrainGrassEnable = true;

	if (gMapManager->InChaosCastle() == true)
	{
		TerrainGrassEnable = false;
	}
	if (gMapManager->InBattleCastle())
	{
		TerrainGrassEnable = false;
	}

	return (iMapNumber);
}

bool SaveTerrainMapping(char* FileName, int iMapNumber)	//
{
	FILE* fp = fopen(FileName, "wb");

	BYTE Version = 0;
	fwrite(&Version, 1, 1, fp);
	fwrite(&iMapNumber, 1, 1, fp);	//
	fwrite(TerrainMappingLayer1, TERRAIN_SIZE * TERRAIN_SIZE, 1, fp);
	fwrite(TerrainMappingLayer2, TERRAIN_SIZE * TERRAIN_SIZE, 1, fp);
	for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
	{
		unsigned char Alpha = (unsigned char)(TerrainMappingAlpha[i] * 255.f);
		fwrite(&Alpha, 1, 1, fp);
	}
	/*
#ifndef BATTLE_CASTLE
	for(i=0;i<MAX_GROUNDS;i++)
	{
		GROUND *o = &Grounds[i];
		if(o->Live)
		{
			fwrite(&o->Type ,2,1,fp);
			fwrite(&o->x    ,1,1,fp);
			fwrite(&o->y    ,1,1,fp);
			fwrite(&o->Angle,1,1,fp);
		}
	}
#endif// BATTLE_CASTLE
*/
	fclose(fp);

	{
		fp = fopen(FileName, "rb");
		if (fp == NULL)
		{
			return (false);
		}
		fseek(fp, 0, SEEK_END);
		int EncBytes = ftell(fp);	//
		fseek(fp, 0, SEEK_SET);
		unsigned char* EncData = new unsigned char[EncBytes];	//
		fread(EncData, 1, EncBytes, fp);//
		fclose(fp);

		int DataBytes = MapFileEncrypt(NULL, EncData, EncBytes);	//
		unsigned char* Data = new unsigned char[DataBytes];		//
		MapFileEncrypt(Data, EncData, EncBytes);	//
		delete[] EncData;		//

		fp = fopen(FileName, "wb");
		fwrite(Data, DataBytes, 1, fp);
		fclose(fp);
		delete[] Data;
	}
	return true;
}

/*
#ifndef BATTLE_CASTLE
	void CreateGround(int Type,int x,int y,float Angle)
	{
		for(int i=0;i<MAX_GROUNDS;i++)
		{
			GROUND *o = &Grounds[i];
			if(!o->Live)
			{
				o->Live  = true;
				o->Type  = Type;
				o->x     = x;
				o->y     = y;
				o->Angle = (unsigned char)(Angle/360.f*255.f);
				return;
			}
		}
	}

	void DeleteGround(int x,int y)
	{
		for(int i=0;i<MAX_GROUNDS;i++)
		{
			GROUND *o = &Grounds[i];
			if(o->Live)
			{
				if(o->x==x && o->y==y) o->Live = false;
			}
		}
	}

	void RenderGrounds()
	{
		for(int i=0;i<MAX_GROUNDS;i++)
		{
			GROUND *o = &Grounds[i];
			if(o->Live)
			{
				float Angle = (float)o->Angle/255.f*360.f;
				RenderTerrainBitmap(BITMAP_CURSOR+6,o->x,o->y,Angle);
			}
		}
	}
#endif// BATTLE_CASTLE
*/
void CreateTerrainNormal()
{
	for (int y = 0; y < TERRAIN_SIZE; y++)
	{
		for (int x = 0; x < TERRAIN_SIZE; x++)
		{
			int Index = TERRAIN_INDEX(x, y);
			vec3_t v1, v2, v3, v4;
			Vector((x)*TERRAIN_SCALE, (y)*TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y)], v4);
			Vector((x + 1) * TERRAIN_SCALE, (y)*TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + 1, y)], v1);
			Vector((x + 1) * TERRAIN_SCALE, (y + 1) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + 1, y + 1)], v2);
			Vector((x)*TERRAIN_SCALE, (y + 1) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y + 1)], v3);
			FaceNormalize(v1, v2, v3, TerrainNormal[Index]);
		}
	}
}

void CreateTerrainNormal_Part(int xi, int yi)
{
	if (xi > TERRAIN_SIZE - 4) xi = TERRAIN_SIZE - 4;
	else if (xi < 4)         xi = 4;

	if (yi > TERRAIN_SIZE - 4) yi = TERRAIN_SIZE - 4;
	else if (yi < 4)         yi = 4;

	for (int y = yi - 4; y < yi + 4; y++)
	{
		for (int x = xi - 4; x < xi + 4; x++)
		{
			int Index = TERRAIN_INDEX(x, y);
			vec3_t v1, v2, v3, v4;
			Vector((x)*TERRAIN_SCALE, (y)*TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y)], v4);
			Vector((x + 1) * TERRAIN_SCALE, (y)*TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + 1, y)], v1);
			Vector((x + 1) * TERRAIN_SCALE, (y + 1) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + 1, y + 1)], v2);
			Vector((x)*TERRAIN_SCALE, (y + 1) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y + 1)], v3);
			FaceNormalize(v1, v2, v3, TerrainNormal[Index]);
		}
	}
}

void CreateTerrainLight()
{
	vec3_t Light;
	if (gMapManager->InBattleCastle())
	{
		Vector(0.5f, -1.f, 1.f, Light);
	}
	else
	{
		Vector(0.5f, -0.5f, 0.5f, Light);
	}
	for (int y = 0; y < TERRAIN_SIZE; y++)
	{
		for (int x = 0; x < TERRAIN_SIZE; x++)
		{
			int Index = TERRAIN_INDEX(x, y);
			float Luminosity = DotProduct(TerrainNormal[Index], Light) + 0.5f;
			if (Luminosity < 0.f) Luminosity = 0.f;
			else if (Luminosity > 1.f) Luminosity = 1.f;
			for (int i = 0; i < 3; i++)
				BackTerrainLight[Index][i] = TerrainLight[Index][i] * Luminosity;
		}
	}
}

void CreateTerrainLight_Part(int xi, int yi)
{
	if (xi > TERRAIN_SIZE - 4) xi = TERRAIN_SIZE - 4;
	else if (xi < 4)         xi = 4;

	if (yi > TERRAIN_SIZE - 4) yi = TERRAIN_SIZE - 4;
	else if (yi < 4)         yi = 4;

	vec3_t Light;
	Vector(0.5f, -0.5f, 0.5f, Light);
	for (int y = yi - 4; y < yi + 4; y++)
	{
		for (int x = xi - 4; x < xi + 4; x++)
		{
			int Index = TERRAIN_INDEX(x, y);
			float Luminosity = DotProduct(TerrainNormal[Index], Light) + 0.5f;
			if (Luminosity < 0.f) Luminosity = 0.f;
			else if (Luminosity > 1.f) Luminosity = 1.f;
			for (int i = 0; i < 3; i++)
				BackTerrainLight[Index][i] = TerrainLight[Index][i] * Luminosity;
		}
	}
}

void pull_extension(std::string& fileName, const char* ext)
{
	size_t dotPosition = fileName.find_last_of('.');
	if (dotPosition != std::string::npos)
	{
		fileName.erase(dotPosition + 1);  // Elimina la extensi�n existente
		fileName += ext;  // Agrega la nueva extensi�n
	}
}

BOOL ExistOZJ(std::string filename)
{
	FILE* fp;
	filename = "Data\\" + filename;

	pull_extension(filename, "OZJ");
	if ((fp = fopen(filename.c_str(), "rb")) != NULL)
	{
		fclose(fp);
		return TRUE;
	}
	else
	{
		return !TRUE;
	}
}

bool OpenBMPBuffer(std::string filename, float* BufferFloat)
{
	FILE* fp;
	filename = "Data\\" + filename;

	pull_extension(filename, "OZB");

	if ((fp = fopen(filename.c_str(), "rb")) != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int iBytes = ftell(fp);
		fseek(fp, 4, SEEK_SET);

		BITMAPFILEHEADER header;
		BITMAPINFOHEADER bmiHeader;
		fread(&header, 1, sizeof(BITMAPFILEHEADER), fp);
		fread(&bmiHeader, 1, sizeof(BITMAPINFOHEADER), fp);

		BYTE* pbyData = new BYTE[iBytes];
		fread(pbyData, 1, iBytes, fp);
		fclose(fp);

		int Index = 0;
		for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; ++i)
		{
			if ((Index + 2) < iBytes)
			{
				BufferFloat[Index + 0] = (float)pbyData[Index + 2] / 255.f;
				BufferFloat[Index + 1] = (float)pbyData[Index + 1] / 255.f;
				BufferFloat[Index + 2] = (float)pbyData[Index + 0] / 255.f;
			}
			Index += 3;
		}

		delete[] pbyData;

		return true;
	}
	else
	{
		char Text[256];
		sprintf(Text, "%s - File not exist.", filename.c_str());
		MessageBox(gwinhandle->GethWnd(), Text, NULL, MB_OK);
		SendMessage(gwinhandle->GethWnd(), WM_DESTROY, 0, 0);
		return false;
	}
}

void OpenTerrainLight(char* FileName)
{
	if (ExistOZJ(FileName))
	{
		OpenJpegBuffer(FileName, &TerrainLight[0][0]);
	}
	else
	{
		OpenBMPBuffer(FileName, &TerrainLight[0][0]);
	}

	CreateTerrainNormal();
	CreateTerrainLight();
}

void SaveTerrainLight(char* FileName)
{
	unsigned char* Buffer = new unsigned char[TERRAIN_SIZE * TERRAIN_SIZE * 3];
	for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			float Light = TerrainLight[i][j] * 255.f;
			if (Light < 0.f)
				Light = 0.f;
			else if (Light > 255.f)
				Light = 255.f;
			Buffer[i * 3 + j] = (unsigned char)(Light);
		}
	}
	WriteJpeg(FileName, TERRAIN_SIZE, TERRAIN_SIZE, Buffer, 100);
	SAFE_DELETE_ARRAY(Buffer);
}

void CreateTerrain(char* FileName, bool bNew)
{
	ActiveTerrain = true;

	if (bNew)
	{
		OpenTerrainHeightNew(FileName);
	}
	else
	{
		OpenTerrainHeight(FileName);
	}

	CreateSun();
}

unsigned char BMPHeader[1080];

bool IsTerrainHeightExtMap(int iWorld)
{
	if (iWorld == WD_42CHANGEUP3RD_2ND
		|| gMapManager->IsPKField()
		|| iWorld == WD_66DOPPLEGANGER2
		|| IsUrukMountain(iWorld)
		|| IsNars(iWorld)
		|| IsFerea(iWorld)
		|| IsSwampOfDarkness(iWorld)
		|| IsKuberaMine()
		|| IsAtlansAbbyss()
		|| IsScorchedCanyon()
		|| IsIcarusRed()
		|| IsTempleOfArnil(iWorld)
		|| IsOldKethotum(iWorld)
		|| IsBlazeKethotum(iWorld)
		|| IsKanturuUnderGround(iWorld)
		)
	{
		return true;
	}

	if (iWorld > WD_81KARUTAN2)
	{
		CUSTOM_MAPE_INFO* world = gMapManager->FindMap(iWorld);

		if (world != NULL && world->TerrainExt)
		{
			return true;
		}
	}

	return false;
}

bool OpenTerrainHeight(char* filename)
{
	char FileName[256];

	char NewFileName[256];

	memset(NewFileName, 0, sizeof(NewFileName));
	for (int i = 0; i < (int)strlen(filename); i++)
	{
		NewFileName[i] = filename[i];
		NewFileName[i + 1] = NULL;
		if (filename[i] == '.')
			break;
	}
	NewFileName[255] = 0;

	strcpy(FileName, "Data\\");
	strcat(FileName, NewFileName);
	strcat(FileName, "OZB");

	FILE* fp = fopen(FileName, "rb");

	if (fp == NULL)
	{
		char Text[256];
		sprintf(Text, "%s file not found.", FileName);
		g_ErrorReport.Write(Text);
		g_ErrorReport.Write("\r\n");
		MessageBox(gwinhandle->GethWnd(), Text, NULL, MB_OK);
		SendMessage(gwinhandle->GethWnd(), WM_DESTROY, 0, 0);
		return false;
	}
	fseek(fp, 4, SEEK_SET);
	int Index = 1080;
	int Size = 256 * 256 + Index;
	unsigned char* Buffer = new unsigned char[Size];
	fread(Buffer, 1, Size, fp);
	fclose(fp);

	memcpy(BMPHeader, Buffer, Index);

	for (int i = 0; i < 256; i++)
	{
		unsigned char* src = &Buffer[Index + i * 256];
		float* dst = &BackTerrainHeight[i * 256];
		for (int j = 0; j < 256; j++)
		{
			if (gMapManager->currentMap == WD_55LOGINSCENE)
				*dst = (float)(*src) * 3.0f;
			else
				*dst = (float)(*src) * 1.5f;

			src++;
			dst++;
		}
	}
	SAFE_DELETE_ARRAY(Buffer);
	return true;
}

void SaveTerrainHeight(char* name)
{
	unsigned char* Buffer = new unsigned char[256 * 256];
	for (int i = 0; i < 256; i++)
	{
		float* src = &BackTerrainHeight[i * 256];
		unsigned char* dst = &Buffer[(255 - i) * 256];
		for (int j = 0; j < 256; j++)
		{
			float Height;

			if (gMapManager->currentMap == WD_55LOGINSCENE)
				Height = *src / 3.0f;
			else
				Height = *src / 1.5f;

			if (Height < 0.f)
				Height = 0.f;
			else if (Height > 255.f)
				Height = 255.f;

			*dst = (unsigned char)(Height);
			src++;
			dst++;
		}
	}

	FILE* fp = fopen(name, "wb");
	fwrite(BMPHeader, 1080, 1, fp);

	for (int i = 0; i < 256; i++)
		fwrite(Buffer + (255 - i) * 256, 256, 1, fp);

	SAFE_DELETE_ARRAY(Buffer);
	fclose(fp);
}

bool OpenTerrainHeightNew(const char* strFilename)
{
	char FileName[256];
	char NewFileName[256] = { '\0', };

	for (int i = 0; i < (int)strlen(strFilename); i++)
	{
		NewFileName[i] = strFilename[i];
		NewFileName[i + 1] = '\0';
		if (strFilename[i] == '.')
			break;
	}

	strcpy(FileName, "Data\\");
	strcat(FileName, NewFileName);
	strcat(FileName, "OZB");

	FILE* fp = fopen(FileName, "rb");
	fseek(fp, 0, SEEK_END);
	int iBytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	BYTE* pbyData = new BYTE[iBytes];
	fread(pbyData, 1, iBytes, fp);
	fclose(fp);

	DWORD dwCurPos = 0;
	dwCurPos += 4;

	BITMAPFILEHEADER header;
	memcpy(&header, &pbyData[dwCurPos], sizeof(BITMAPFILEHEADER));
	dwCurPos += sizeof(BITMAPFILEHEADER);

	BITMAPINFOHEADER bmiHeader;
	memcpy(&bmiHeader, &pbyData[dwCurPos], sizeof(BITMAPINFOHEADER));
	dwCurPos += sizeof(BITMAPINFOHEADER);

	for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; ++i)
	{
		BYTE* pbysrc = &pbyData[dwCurPos + i * 3];
		DWORD dwHeight = 0;
		BYTE* pbyHeight = (BYTE*)&dwHeight;

		pbyHeight[0] = pbysrc[2];
		pbyHeight[1] = pbysrc[1];
		pbyHeight[2] = pbysrc[0];

		BackTerrainHeight[i] = (float)dwHeight;
		BackTerrainHeight[i] += g_fMinHeight;
	}

	delete[] pbyData;

	return true;
}

extern int SceneFlag;

float RequestTerrainHeight(float xf, float yf)
{
	if ((gmProtect->SceneLogin == 1 && SceneFlag == LOG_IN_SCENE) || (gmProtect->SceneCharacter == 1 && SceneFlag == CHARACTER_SCENE))
	{
		return 0.f;
	}

	if (SceneFlag == SERVER_LIST_SCENE || SceneFlag == WEBZEN_SCENE || SceneFlag == LOADING_SCENE)
		return 0.f;

	if (xf < 0.f || yf < 0.f)
		return 0.f;

	xf = xf / TERRAIN_SCALE;
	yf = yf / TERRAIN_SCALE;

	unsigned int Index = TERRAIN_INDEX(xf, yf);

	if (Index >= TERRAIN_SIZE * TERRAIN_SIZE)
		return g_fSpecialHeight;

	if ((TerrainWall[Index] & TW_HEIGHT) == TW_HEIGHT)
	{
		return g_fSpecialHeight;
	}

	int xi = (int)xf;
	int yi = (int)yf;
	float xd = xf - (float)xi;
	float yd = yf - (float)yi;
	unsigned int Index1 = TERRAIN_INDEX_REPEAT(xi, yi);
	unsigned int Index2 = TERRAIN_INDEX_REPEAT(xi, yi + 1);
	unsigned int Index3 = TERRAIN_INDEX_REPEAT(xi + 1, yi);
	unsigned int Index4 = TERRAIN_INDEX_REPEAT(xi + 1, yi + 1);

	if (Index1 >= TERRAIN_SIZE * TERRAIN_SIZE || Index2 >= TERRAIN_SIZE * TERRAIN_SIZE ||
		Index3 >= TERRAIN_SIZE * TERRAIN_SIZE || Index4 >= TERRAIN_SIZE * TERRAIN_SIZE)
		return g_fSpecialHeight;

	float left = BackTerrainHeight[Index1] + (BackTerrainHeight[Index2] - BackTerrainHeight[Index1]) * yd;
	float right = BackTerrainHeight[Index3] + (BackTerrainHeight[Index4] - BackTerrainHeight[Index3]) * yd;

	return left + (right - left) * xd;
}

void RequestTerrainNormal(float xf, float yf, vec3_t Normal)
{
	xf = xf / TERRAIN_SCALE;
	yf = yf / TERRAIN_SCALE;
	int xi = (int)xf;
	int yi = (int)yf;
	VectorCopy(TerrainNormal[TERRAIN_INDEX_REPEAT(xi, yi)], Normal);
}

void AddTerrainHeight(float xf, float yf, float Height, int Range, float* Buffer)
{
	float rf = (float)Range;

	xf = xf / TERRAIN_SCALE;
	yf = yf / TERRAIN_SCALE;
	int   xi = (int)xf;
	int   yi = (int)yf;
	int   syi = yi - Range;
	int   eyi = yi + Range;
	float syf = (float)(syi);
	for (; syi <= eyi; syi++, syf += 1.f)
	{
		int   sxi = xi - Range;
		int   exi = xi + Range;
		float sxf = (float)(sxi);
		for (; sxi <= exi; sxi++, sxf += 1.f)
		{
			float xd = xf - sxf;
			float yd = yf - syf;
			float lf = (rf - sqrtf(xd * xd + yd * yd)) / rf;
			if (lf > 0.f)
			{
				Buffer[TERRAIN_INDEX_REPEAT(sxi, syi)] += Height * lf;
			}
		}
	}
}

void SetTerrainLight(float xf, float yf, vec3_t Light, int Range, vec3_t* Buffer)
{
	float rf = (float)Range;

	xf = (xf / TERRAIN_SCALE);
	yf = (yf / TERRAIN_SCALE);
	int   xi = (int)xf;
	int   yi = (int)yf;
	int   syi = yi - Range;
	int   eyi = yi + Range;
	float syf = (float)(syi);
	for (; syi <= eyi; syi++, syf += 1.f)
	{
		int   sxi = xi - Range;
		int   exi = xi + Range;
		float sxf = (float)(sxi);
		for (; sxi <= exi; sxi++, sxf += 1.f)
		{
			float xd = xf - sxf;
			float yd = yf - syf;
			float lf = (rf - sqrtf(xd * xd + yd * yd)) / rf;
			if (lf > 0.f)
			{
				float* b = &Buffer[TERRAIN_INDEX_REPEAT(sxi, syi)][0];
				for (int i = 0; i < 3; i++)
				{
					b[i] += Light[i] * lf;
				}
			}
		}
	}
}

void AddTerrainLight(float xf, float yf, vec3_t Light, int Range, vec3_t* Buffer)
{
	float rf = (float)Range;

	xf = (xf / TERRAIN_SCALE);
	yf = (yf / TERRAIN_SCALE);
	int   xi = (int)xf;
	int   yi = (int)yf;
	int   syi = yi - Range;
	int   eyi = yi + Range;
	float syf = (float)(syi);

	for (; syi <= eyi; syi++, syf += 1.f)
	{
		int   sxi = xi - Range;
		int   exi = xi + Range;
		float sxf = (float)(sxi);
		for (; sxi <= exi; sxi++, sxf += 1.f)
		{
			float xd = xf - sxf;
			float yd = yf - syf;
			float lf = (rf - sqrtf(xd * xd + yd * yd)) / rf;
			if (lf > 0.f)
			{
				float* b = &Buffer[TERRAIN_INDEX_REPEAT(sxi, syi)][0];
				for (int i = 0; i < 3; i++)
				{
					b[i] += Light[i] * lf;
					if (b[i] < 0.f)
						b[i] = 0.f;
				}
			}
		}
	}
}

void AddTerrainLightClip(float xf, float yf, vec3_t Light, int Range, vec3_t* Buffer)
{
	float rf = (float)Range;

	xf = (xf / TERRAIN_SCALE);
	yf = (yf / TERRAIN_SCALE);
	int   xi = (int)xf;
	int   yi = (int)yf;
	int   syi = yi - Range;
	int   eyi = yi + Range;
	float syf = (float)(syi);
	for (; syi <= eyi; syi++, syf += 1.f)
	{
		int   sxi = xi - Range;
		int   exi = xi + Range;
		float sxf = (float)(sxi);
		for (; sxi <= exi; sxi++, sxf += 1.f)
		{
			float xd = xf - sxf;
			float yd = yf - syf;
			float lf = (rf - sqrtf(xd * xd + yd * yd)) / rf;
			if (lf > 0.f)
			{
				float* b = &Buffer[TERRAIN_INDEX_REPEAT(sxi, syi)][0];
				for (int i = 0; i < 3; i++)
				{
					b[i] += Light[i] * lf;
					if (b[i] < 0.f) b[i] = 0.f;
					else if (b[i] > 1.f) b[i] = 1.f;
				}
			}
		}
	}
}

void RequestTerrainLight(float xf, float yf, vec3_t Light)
{
	if ((gmProtect->SceneLogin == 1 && SceneFlag == LOG_IN_SCENE) || (gmProtect->SceneCharacter == 1 && SceneFlag == CHARACTER_SCENE))
	{
		Vector(0.f, 0.f, 0.f, Light);
		return;
	}

	if (SceneFlag == SERVER_LIST_SCENE
		|| SceneFlag == WEBZEN_SCENE
		|| SceneFlag == LOADING_SCENE
		|| ActiveTerrain == false)
	{
		Vector(0.f, 0.f, 0.f, Light);
		return;
	}

	xf = xf / TERRAIN_SCALE;
	yf = yf / TERRAIN_SCALE;
	int xi = (int)xf;
	int yi = (int)yf;
	if (xi<0 || yi<0 || xi>TERRAIN_SIZE_MASK - 1 || yi>TERRAIN_SIZE_MASK - 1)
	{
		Vector(0.f, 0.f, 0.f, Light);
		return;
	}
	int Index1 = ((xi)+(yi)*TERRAIN_SIZE);
	int Index2 = ((xi + 1) + (yi)*TERRAIN_SIZE);
	int Index3 = ((xi + 1) + (yi + 1) * TERRAIN_SIZE);
	int Index4 = ((xi)+(yi + 1) * TERRAIN_SIZE);
	float xd = xf - (float)xi;
	float yd = yf - (float)yi;
	for (int i = 0; i < 3; i++)
	{
		float left = PrimaryTerrainLight[Index1][i] + (PrimaryTerrainLight[Index4][i] - PrimaryTerrainLight[Index1][i]) * yd;
		float right = PrimaryTerrainLight[Index2][i] + (PrimaryTerrainLight[Index3][i] - PrimaryTerrainLight[Index2][i]) * yd;
		Light[i] = (left + (right - left) * xd);
	}
}

void CreateLodBuffer()
{
	for (int y = 0; y < TERRAIN_SIZE; y += 4)
	{
		for (int x = 0; x < TERRAIN_SIZE; x += 4)
		{
			vec3_t NormalMin, NormalMax;
			NormalMin[0] = 1.f;
			NormalMin[1] = 1.f;
			NormalMin[2] = 1.f;
			NormalMax[0] = -1.f;
			NormalMax[1] = -1.f;
			NormalMax[2] = -1.f;
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					vec3_t v1, v2, v3, v4;
					Vector((x + j) * TERRAIN_SCALE, (y + i) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + j, y + i)], v1);
					Vector((x + j + 1) * TERRAIN_SCALE, (y + i) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + j + 1, y + i)], v2);
					Vector((x + j + 1) * TERRAIN_SCALE, (y + i + 1) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + j + 1, y + i + 1)], v3);
					Vector((x + j) * TERRAIN_SCALE, (y + i + 1) * TERRAIN_SCALE, BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + j, y + i + 1)], v4);

					int Index = TERRAIN_INDEX(x + j, y + i);
					FaceNormalize(v1, v2, v3, TerrainNormal[Index]);

					vec3_t* Normal = &TerrainNormal[Index];
					if (NormalMin[0] > (*Normal)[0]) NormalMin[0] = (*Normal)[0];
					if (NormalMax[0] < (*Normal)[0]) NormalMax[0] = (*Normal)[0];
					if (NormalMin[1] > (*Normal)[1]) NormalMin[1] = (*Normal)[1];
					if (NormalMax[1] < (*Normal)[1]) NormalMax[1] = (*Normal)[1];
					if (NormalMin[2] > (*Normal)[2]) NormalMin[2] = (*Normal)[2];
					if (NormalMax[2] < (*Normal)[2]) NormalMax[2] = (*Normal)[2];
				}
			}
			float Delta = maxf(maxf(absf(NormalMax[0] - NormalMin[0]), absf(NormalMax[1] - NormalMin[1])), absf(NormalMax[2] - NormalMin[2]));
			if (DetailLowEnable == true)
			{
				LodBuffer[y / 4 * 64 + x / 4] = 4;
			}
			else
			{
				if (Delta >= 1.f) LodBuffer[y / 4 * 64 + x / 4] = 1;
				else if (Delta >= 0.5f) LodBuffer[y / 4 * 64 + x / 4] = 2;
				else                   LodBuffer[y / 4 * 64 + x / 4] = 4;
				LodBuffer[y / 4 * 64 + x / 4] = 1;
			}
		}
	}
}

void InterpolationHeight(int lod, int x, int y, int xd, float* TerrainHeight)
{
	if (lod >= 4)
	{
		TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 2)] = (
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y)] +
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 4)]) * 0.5f;
	}
	if (lod >= 2)
	{
		TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 1)] = (
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y)] +
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 2)]) * 0.5f;
		TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 3)] = (
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 2)] +
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + xd, y + 4)]) * 0.5f;
	}
}

void InterpolationWidth(int lod, int x, int y, int yd, float* TerrainHeight)
{
	if (lod >= 4)
	{
		TerrainHeight[TERRAIN_INDEX_REPEAT(x + 2, y + yd)] = (
			TerrainHeight[TERRAIN_INDEX_REPEAT(x, y + yd)] +
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + 4, y + yd)]) * 0.5f;
	}
	if (lod >= 2)
	{
		TerrainHeight[TERRAIN_INDEX_REPEAT(x + 1, y + yd)] = (
			TerrainHeight[TERRAIN_INDEX_REPEAT(x, y + yd)] +
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + 2, y + yd)]) * 0.5f;
		TerrainHeight[TERRAIN_INDEX_REPEAT(x + 3, y + yd)] = (
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + 2, y + yd)] +
			TerrainHeight[TERRAIN_INDEX_REPEAT(x + 4, y + yd)]) * 0.5f;
	}
}

void InterpolationCross(int lod, int x, int y)
{
	BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y)] = (
		BackTerrainHeight[TERRAIN_INDEX_REPEAT(x - lod, y)] +
		BackTerrainHeight[TERRAIN_INDEX_REPEAT(x + lod, y)] +
		BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y - lod)] +
		BackTerrainHeight[TERRAIN_INDEX_REPEAT(x, y + lod)]) * 0.25f;
}

void PrefixTerrainHeightEdge(int x, int y, int lod, float* TerrainHeight)
{
	if (lod >= LodBuffer[((y) & 63) * 64 + ((x - 1) & 63)])
		InterpolationHeight(lod, x * 4, y * 4, 0, TerrainHeight);

	if (lod >= LodBuffer[((y) & 63) * 64 + ((x + 1) & 63)])
		InterpolationHeight(lod, x * 4, y * 4, 4, TerrainHeight);

	if (lod >= LodBuffer[((y - 1) & 63) * 64 + ((x) & 63)])
		InterpolationWidth(lod, x * 4, y * 4, 0, TerrainHeight);

	if (lod >= LodBuffer[((y + 1) & 63) * 64 + ((x) & 63)])
		InterpolationWidth(lod, x * 4, y * 4, 4, TerrainHeight);
}

void PrefixTerrainHeight()
{
	for (int y = 0; y < 64; y++)
	{
		for (int x = 0; x < 64; x++)
		{
			int lod = LodBuffer[((y) & 63) * 64 + ((x) & 63)];
			PrefixTerrainHeightEdge(x, y, lod, BackTerrainHeight);
			if (lod >= 2)
			{
				if (lod >= 4)
				{
					InterpolationHeight(lod, x * 4, y * 4, 2, BackTerrainHeight);
					InterpolationWidth(lod, x * 4, y * 4, 2, BackTerrainHeight);
				}
				if (lod >= 2)
				{
					InterpolationHeight(lod, x * 4, y * 4, 1, BackTerrainHeight);
					InterpolationWidth(lod, x * 4, y * 4, 1, BackTerrainHeight);
					InterpolationHeight(lod, x * 4, y * 4, 3, BackTerrainHeight);
					InterpolationWidth(lod, x * 4, y * 4, 3, BackTerrainHeight);
				}
			}
		}
	}
}

inline void Interpolation(int mx, int my)
{
	Index01 = (my * 2) * 512 + (mx * 2 + 1);
	Index12 = (my * 2 + 1) * 512 + (mx * 2 + 2);
	Index23 = (my * 2 + 2) * 512 + (mx * 2 + 1);
	Index30 = (my * 2 + 1) * 512 + (mx * 2);
	Index02 = (my * 2 + 1) * 512 + (mx * 2 + 1);

	for (int i = 0; i < 3; i++)
	{
		TerrainVertex01[i] = (TerrainVertex[0][i] + TerrainVertex[1][i]) * 0.5f;
		TerrainVertex12[i] = (TerrainVertex[1][i] + TerrainVertex[2][i]) * 0.5f;
		TerrainVertex23[i] = (TerrainVertex[2][i] + TerrainVertex[3][i]) * 0.5f;
		TerrainVertex30[i] = (TerrainVertex[3][i] + TerrainVertex[0][i]) * 0.5f;
		TerrainVertex02[i] = (TerrainVertex[0][i] + TerrainVertex[2][i]) * 0.5f;
	}

	for (int i = 0; i < 2; i++)
	{
		TerrainTextureCoord01[i] = (TerrainTextureCoord[0][i] + TerrainTextureCoord[1][i]) * 0.5f;
		TerrainTextureCoord12[i] = (TerrainTextureCoord[1][i] + TerrainTextureCoord[2][i]) * 0.5f;
		TerrainTextureCoord23[i] = (TerrainTextureCoord[2][i] + TerrainTextureCoord[3][i]) * 0.5f;
		TerrainTextureCoord30[i] = (TerrainTextureCoord[3][i] + TerrainTextureCoord[0][i]) * 0.5f;
		TerrainTextureCoord02[i] = (TerrainTextureCoord[0][i] + TerrainTextureCoord[2][i]) * 0.5f;
	}
}

inline void Vertex0()
{
	glTexCoord2f(TerrainTextureCoord[0][0], TerrainTextureCoord[0][1]);
	glColor3fv(PrimaryTerrainLight[TerrainIndex1]);
	glVertex3fv(TerrainVertex[0]);
}

inline void Vertex1()
{
	glTexCoord2f(TerrainTextureCoord[1][0], TerrainTextureCoord[1][1]);
	glColor3fv(PrimaryTerrainLight[TerrainIndex2]);
	glVertex3fv(TerrainVertex[1]);
}

inline void Vertex2()
{
	glTexCoord2f(TerrainTextureCoord[2][0], TerrainTextureCoord[2][1]);
	glColor3fv(PrimaryTerrainLight[TerrainIndex3]);
	glVertex3fv(TerrainVertex[2]);
}

inline void Vertex3()
{
	glTexCoord2f(TerrainTextureCoord[3][0], TerrainTextureCoord[3][1]);
	glColor3fv(PrimaryTerrainLight[TerrainIndex4]);
	glVertex3fv(TerrainVertex[3]);
}

inline void Vertex01()
{
	glTexCoord2f(TerrainTextureCoord01[0], TerrainTextureCoord01[1]);
	glColor3fv(PrimaryTerrainLight[Index01]);
	glVertex3fv(TerrainVertex01);
}

inline void Vertex12()
{
	glTexCoord2f(TerrainTextureCoord12[0], TerrainTextureCoord12[1]);
	glColor3fv(PrimaryTerrainLight[Index12]);
	glVertex3fv(TerrainVertex12);
}

inline void Vertex23()
{
	glTexCoord2f(TerrainTextureCoord23[0], TerrainTextureCoord23[1]);
	glColor3fv(PrimaryTerrainLight[Index23]);
	glVertex3fv(TerrainVertex23);
}

inline void Vertex30()
{
	glTexCoord2f(TerrainTextureCoord30[0], TerrainTextureCoord30[1]);
	glColor3fv(PrimaryTerrainLight[Index30]);
	glVertex3fv(TerrainVertex30);
}

inline void Vertex02()
{
	glTexCoord2f(TerrainTextureCoord02[0], TerrainTextureCoord02[1]);
	glColor3fv(PrimaryTerrainLight[Index02]);
	glVertex3fv(TerrainVertex02);
}

inline void VertexAlpha0()
{
	glTexCoord2f(TerrainTextureCoord[0][0], TerrainTextureCoord[0][1]);
	float* Light = &PrimaryTerrainLight[TerrainIndex1][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha[TerrainIndex1]);
	glVertex3fv(TerrainVertex[0]);
}

inline void VertexAlpha1()
{
	glTexCoord2f(TerrainTextureCoord[1][0], TerrainTextureCoord[1][1]);
	float* Light = &PrimaryTerrainLight[TerrainIndex2][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha[TerrainIndex2]);
	glVertex3fv(TerrainVertex[1]);
}

inline void VertexAlpha2()
{
	glTexCoord2f(TerrainTextureCoord[2][0], TerrainTextureCoord[2][1]);
	float* Light = &PrimaryTerrainLight[TerrainIndex3][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha[TerrainIndex3]);
	glVertex3fv(TerrainVertex[2]);
}

inline void VertexAlpha3()
{
	glTexCoord2f(TerrainTextureCoord[3][0], TerrainTextureCoord[3][1]);
	float* Light = &PrimaryTerrainLight[TerrainIndex4][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha[TerrainIndex4]);
	glVertex3fv(TerrainVertex[3]);
}

inline void VertexAlpha01()
{
	glTexCoord2f(TerrainTextureCoord01[0], TerrainTextureCoord01[1]);
	float* Light = &PrimaryTerrainLight[Index01][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha01);
	glVertex3fv(TerrainVertex01);
}

inline void VertexAlpha12()
{
	glTexCoord2f(TerrainTextureCoord12[0], TerrainTextureCoord12[1]);
	float* Light = &PrimaryTerrainLight[Index12][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha12);
	glVertex3fv(TerrainVertex12);
}

inline void VertexAlpha23()
{
	glTexCoord2f(TerrainTextureCoord23[0], TerrainTextureCoord23[1]);
	float* Light = &PrimaryTerrainLight[Index23][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha23);
	glVertex3fv(TerrainVertex23);
}

inline void VertexAlpha30()
{
	glTexCoord2f(TerrainTextureCoord30[0], TerrainTextureCoord30[1]);
	float* Light = &PrimaryTerrainLight[Index30][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha30);
	glVertex3fv(TerrainVertex30);
}

inline void VertexAlpha02()
{
	glTexCoord2f(TerrainTextureCoord02[0], TerrainTextureCoord02[1]);
	float* Light = &PrimaryTerrainLight[Index02][0];
	glColor4f(Light[0], Light[1], Light[2], TerrainMappingAlpha02);
	glVertex3fv(TerrainVertex02);
}

inline void VertexBlend0()
{
	glTexCoord2f(TerrainTextureCoord[0][0], TerrainTextureCoord[0][1]);
	float Light = TerrainMappingAlpha[TerrainIndex1];
	glColor3f(Light, Light, Light);
	glVertex3fv(TerrainVertex[0]);
}

inline void VertexBlend1()
{
	glTexCoord2f(TerrainTextureCoord[1][0], TerrainTextureCoord[1][1]);
	float Light = TerrainMappingAlpha[TerrainIndex2];
	glColor3f(Light, Light, Light);
	glVertex3fv(TerrainVertex[1]);
}

inline void VertexBlend2()
{
	glTexCoord2f(TerrainTextureCoord[2][0], TerrainTextureCoord[2][1]);
	float Light = TerrainMappingAlpha[TerrainIndex3];
	glColor3f(Light, Light, Light);
	glVertex3fv(TerrainVertex[2]);
}

inline void VertexBlend3()
{
	glTexCoord2f(TerrainTextureCoord[3][0], TerrainTextureCoord[3][1]);
	float Light = TerrainMappingAlpha[TerrainIndex4];
	glColor3f(Light, Light, Light);
	glVertex3fv(TerrainVertex[3]);
}

void Vertex__alpha0(float Alpha, bool Normal)
{
	glTexCoord2f(TerrainTextureCoord[0][0], TerrainTextureCoord[0][1]);
	glColor4f(PrimaryTerrainLight[TerrainIndex1][0], PrimaryTerrainLight[TerrainIndex1][1], PrimaryTerrainLight[TerrainIndex1][2], Alpha);
	glVertex3fv(TerrainVertex[0]);
	if (Normal) glNormal3fv(TerrainNormal[TerrainIndex1]);
}

void Vertex__alpha1(float Alpha, bool Normal)
{
	glTexCoord2f(TerrainTextureCoord[1][0], TerrainTextureCoord[1][1]);
	glColor4f(PrimaryTerrainLight[TerrainIndex2][0], PrimaryTerrainLight[TerrainIndex2][1], PrimaryTerrainLight[TerrainIndex2][2], Alpha);
	glVertex3fv(TerrainVertex[1]);
	if (Normal) glNormal3fv(TerrainNormal[TerrainIndex2]);
}

void Vertex__alpha2(float Alpha, bool Normal)
{
	glTexCoord2f(TerrainTextureCoord[2][0], TerrainTextureCoord[2][1]);
	glColor4f(PrimaryTerrainLight[TerrainIndex3][0], PrimaryTerrainLight[TerrainIndex3][1], PrimaryTerrainLight[TerrainIndex3][2], Alpha);
	glVertex3fv(TerrainVertex[2]);
	if (Normal) glNormal3fv(TerrainNormal[TerrainIndex3]);
}

void Vertex__alpha3(float Alpha, bool Normal)
{
	glTexCoord2f(TerrainTextureCoord[3][0], TerrainTextureCoord[3][1]);
	glColor4f(PrimaryTerrainLight[TerrainIndex4][0], PrimaryTerrainLight[TerrainIndex4][1], PrimaryTerrainLight[TerrainIndex4][2], Alpha);
	glVertex3fv(TerrainVertex[3]);

	if (Normal) glNormal3fv(TerrainNormal[TerrainIndex4]);
}

void RenderFace(int Texture, int mx, int my)
{
	if (g_pOption->GetRenderTerrain() == false)
		return;

	int pMapIndex = gMapManager->FindMapIndex();

	if (pMapIndex == WD_39KANTURU_3RD)
	{
		if (Texture == 3)
			EnableAlphaTest(TRUE);
		else if (Texture == 100)
			return;
		else
			DisableAlphaBlend();
	}
	else if (gMapManager->IsCursedTemple())
	{
		if (Texture == 100 || Texture == 101)
		{
			if (Texture == 100)
				EnableAlphaTest();
			else if (Texture == 101)
				EnableAlphaBlend();
		}
		else
		{
			DisableAlphaBlend();
		}
	}
	else
	{
		DisableAlphaBlend();
	}

	BindTexture(BITMAP_MAPTILE + Texture);

	// FIX PASSO 3.5: glBegin/glEnd removido
	DrawTerrainFan4(
		TerrainVertex[0], TerrainVertex[1], TerrainVertex[2], TerrainVertex[3],
		TerrainTextureCoord[0], TerrainTextureCoord[1], TerrainTextureCoord[2], TerrainTextureCoord[3],
		PrimaryTerrainLight[TerrainIndex1], PrimaryTerrainLight[TerrainIndex2],
		PrimaryTerrainLight[TerrainIndex3], PrimaryTerrainLight[TerrainIndex4]
	);
}

void RenderFace_After(int Texture, int mx, int my)
{
	if (g_pOption->GetRenderTerrain())
	{
		if (Texture == 100 || Texture == 101)
		{
			if (Texture == 100)
				EnableAlphaTest();
			else if (Texture == 101)
				EnableAlphaBlend();

			BindTexture(BITMAP_MAPTILE + Texture);

			DrawTerrainFan4(
				TerrainVertex[0], TerrainVertex[1], TerrainVertex[2], TerrainVertex[3],
				TerrainTextureCoord[0], TerrainTextureCoord[1], TerrainTextureCoord[2], TerrainTextureCoord[3],
				PrimaryTerrainLight[TerrainIndex1], PrimaryTerrainLight[TerrainIndex2],
				PrimaryTerrainLight[TerrainIndex3], PrimaryTerrainLight[TerrainIndex4]
			);
		}
	}
}

void RenderFaceAlpha(int Texture, int mx, int my)
{
	if (g_pOption->GetRenderTerrain())
	{
		EnableAlphaTest();
		BindTexture(BITMAP_MAPTILE + Texture);

		float c0[4] = {PrimaryTerrainLight[TerrainIndex1][0], PrimaryTerrainLight[TerrainIndex1][1], PrimaryTerrainLight[TerrainIndex1][2], TerrainMappingAlpha[TerrainIndex1]};
		float c1[4] = {PrimaryTerrainLight[TerrainIndex2][0], PrimaryTerrainLight[TerrainIndex2][1], PrimaryTerrainLight[TerrainIndex2][2], TerrainMappingAlpha[TerrainIndex2]};
		float c2[4] = {PrimaryTerrainLight[TerrainIndex3][0], PrimaryTerrainLight[TerrainIndex3][1], PrimaryTerrainLight[TerrainIndex3][2], TerrainMappingAlpha[TerrainIndex3]};
		float c3[4] = {PrimaryTerrainLight[TerrainIndex4][0], PrimaryTerrainLight[TerrainIndex4][1], PrimaryTerrainLight[TerrainIndex4][2], TerrainMappingAlpha[TerrainIndex4]};

		DrawTerrainFan4(
			TerrainVertex[0], TerrainVertex[1], TerrainVertex[2], TerrainVertex[3],
			TerrainTextureCoord[0], TerrainTextureCoord[1], TerrainTextureCoord[2], TerrainTextureCoord[3],
			c0, c1, c2, c3,
			true
		);
	}
}

void RenderFaceBlend(int Texture, int mx, int my)
{
	if (g_pOption->GetRenderTerrain())
	{
		EnableAlphaBlend();
		BindTexture(BITMAP_MAPTILE + Texture);

		float c0[4] = {TerrainMappingAlpha[TerrainIndex1], TerrainMappingAlpha[TerrainIndex1], TerrainMappingAlpha[TerrainIndex1], 1.0f};
		float c1[4] = {TerrainMappingAlpha[TerrainIndex2], TerrainMappingAlpha[TerrainIndex2], TerrainMappingAlpha[TerrainIndex2], 1.0f};
		float c2[4] = {TerrainMappingAlpha[TerrainIndex3], TerrainMappingAlpha[TerrainIndex3], TerrainMappingAlpha[TerrainIndex3], 1.0f};
		float c3[4] = {TerrainMappingAlpha[TerrainIndex4], TerrainMappingAlpha[TerrainIndex4], TerrainMappingAlpha[TerrainIndex4], 1.0f};

		DrawTerrainFan4(
			TerrainVertex[0], TerrainVertex[1], TerrainVertex[2], TerrainVertex[3],
			TerrainTextureCoord[0], TerrainTextureCoord[1], TerrainTextureCoord[2], TerrainTextureCoord[3],
			c0, c1, c2, c3
		);
	}
}

void FaceTexture(int Texture, float xf, float yf, bool Water, bool Scale)
{
	vec3_t Light, Pos;
	Vector(0.30f, 0.40f, 0.20f, Light);
	BITMAP_t* b = &Bitmaps[BITMAP_MAPTILE + Texture];
	float Width, Height;

	if (Scale)
	{
		Width = 16.f / b->Width;
		Height = 16.f / b->Height;
	}
	else
	{
		Width = 64.f / b->Width;
		Height = 64.f / b->Height;
	}

	float suf = xf * Width;
	float svf = yf * Height;
	if (!Water)
	{
		TEXCOORD(TerrainTextureCoord[0], suf, svf);
		TEXCOORD(TerrainTextureCoord[1], suf + Width, svf);
		TEXCOORD(TerrainTextureCoord[2], suf + Width, svf + Height);
		TEXCOORD(TerrainTextureCoord[3], suf, svf + Height);
	}
	else
	{
		float Water1 = 0.f;
		float Water2 = 0.f;
		float Water3 = 0.f;
		float Water4 = 0.f;

		if (gMapManager->currentMap == WD_34CRYWOLF_1ST && Texture == 5)
		{
			if (rand() % 50 == 0)
			{
				float sx = xf * TERRAIN_SCALE + (float)((rand() % 100 + 1) * 1.0f);
				float sy = yf * TERRAIN_SCALE + (float)((rand() % 100 + 1) * 1.0f);
				Vector(sx, sy, Hero->Object.Position[2] + 10.f, Pos);
				CreateParticleSync(BITMAP_SPOT_WATER, Pos, Hero->Object.Angle, Light, 0);
			}
		}

		if (gMapManager->currentMap == WD_30BATTLECASTLE && Texture == 5)
			suf -= WaterMove;
		else
			suf += WaterMove;

		if (Scale)
		{
			Water3 = TerrainGrassWind[TerrainIndex1] * 0.008f;
			Water4 = TerrainGrassWind[TerrainIndex2] * 0.008f;
		}
		else
		{
			Water3 = TerrainGrassWind[TerrainIndex1] * 0.002f;
			Water4 = TerrainGrassWind[TerrainIndex2] * 0.002f;
		}

		TEXCOORD(TerrainTextureCoord[0], suf + Water1, svf + Water3);
		TEXCOORD(TerrainTextureCoord[1], suf + Width + Water2, svf + Water4);
		TEXCOORD(TerrainTextureCoord[2], suf + Width + Water2, svf + Height + Water4);
		TEXCOORD(TerrainTextureCoord[3], suf + Water1, svf + Height + Water3);
	}
}

int WaterTextureNumber = 0;

// Forward declarations for terrain batch system
static void TerrainBatchInit();
static void TerrainBatchAddTile(int textureId, vec3_t* verts, vec2_t* texcoords, vec3_t* colors, float alpha);
static void TerrainBatchReset();
static void TerrainBatchFlush();
static bool g_terrainBatchEnabled = false;
static bool g_terrainBatchInited = false;
static bool g_terrainGrassMerged = false;   // [Tier1-B] setado em RenderTerrain() por frame

void RenderTerrainFace(float xf, float yf, int xi, int yi, float lodf)
{
	// FIX PASSO 3.5: glAlphaFunc removido no Core Profile — usar discard no fragment shader
	// glAlphaFunc(GL_GREATER, 0.0000000);

	// [Tier1-C] Hoist: RenderTerrainVisual ja se auto-guarda com
	// (InChaosCastle()==false || rand()%8); evita ~2 chamadas por face/frame fora do CC.
	if (gMapManager->InChaosCastle())
		RenderTerrainVisual(xi, yi);

	if (TerrainFlag != TERRAIN_MAP_GRASS)
	{
		int Texture;
		bool Alpha;
		bool Water = false;
		if (TerrainMappingAlpha[TerrainIndex1] >= 1.f && TerrainMappingAlpha[TerrainIndex2] >= 1.f && TerrainMappingAlpha[TerrainIndex3] >= 1.f && TerrainMappingAlpha[TerrainIndex4] >= 1.f)
		{
			Texture = TerrainMappingLayer2[TerrainIndex1];
			Alpha = false;
		}
		else
		{
			Texture = TerrainMappingLayer1[TerrainIndex1];
			Alpha = true;
			if (Texture == 5)
			{
				Water = true;
			}
			if (Texture == 11 && (gMapManager->IsPKField() || IsDoppelGanger2()))
				Water = true;
		}
		FaceTexture(Texture, xf, yf, Water, false);

		TerrainBatchInit();
		if (g_terrainBatchEnabled && TerrainFlag != TERRAIN_MAP_GRASS && !Alpha)
		{
			vec3_t colors[4];
			colors[0][0] = PrimaryTerrainLight[TerrainIndex1][0];
			colors[0][1] = PrimaryTerrainLight[TerrainIndex1][1];
			colors[0][2] = PrimaryTerrainLight[TerrainIndex1][2];
			colors[1][0] = PrimaryTerrainLight[TerrainIndex2][0];
			colors[1][1] = PrimaryTerrainLight[TerrainIndex2][1];
			colors[1][2] = PrimaryTerrainLight[TerrainIndex2][2];
			colors[2][0] = PrimaryTerrainLight[TerrainIndex3][0];
			colors[2][1] = PrimaryTerrainLight[TerrainIndex3][1];
			colors[2][2] = PrimaryTerrainLight[TerrainIndex3][2];
			colors[3][0] = PrimaryTerrainLight[TerrainIndex4][0];
			colors[3][1] = PrimaryTerrainLight[TerrainIndex4][1];
			colors[3][2] = PrimaryTerrainLight[TerrainIndex4][2];
			TerrainBatchAddTile(BITMAP_MAPTILE + Texture, TerrainVertex, TerrainTextureCoord, colors, 1.0f);
		}
		else
		{
			RenderFace(Texture, xi, yi);
		}

		if (TerrainMappingAlpha[TerrainIndex1] > 0.f
			|| TerrainMappingAlpha[TerrainIndex2] > 0.f
			|| TerrainMappingAlpha[TerrainIndex3] > 0.f
			|| TerrainMappingAlpha[TerrainIndex4] > 0.f)
		{
			if ((gMapManager->currentMap == WD_7ATLANSE || IsDoppelGanger3()) && TerrainMappingLayer2[TerrainIndex1] == 5)
			{
				Texture = BITMAP_WATER - BITMAP_MAPTILE + (int)WaterTextureNumber;
				FaceTexture(Texture, xf, yf, false, true);
				RenderFaceBlend(Texture, xi, yi);
			}
			else if (Alpha)
			{
				Texture = TerrainMappingLayer2[TerrainIndex1];
				if (Texture != 5)
					Water = false;
				if (Texture != 255)
				{
					FaceTexture(Texture, xf, yf, Water, false);
					RenderFaceAlpha(Texture, xi, yi);
				}
			}
		}
	}
	// [Tier1-B] Grama desenhada inline na passada NORMAL
	if (!EditFlag && TerrainFlag == TERRAIN_MAP_NORMAL && g_terrainGrassMerged
		&& TerrainGrassEnable && gMapManager->currentMap != WD_7ATLANSE && !IsDoppelGanger3())
	{
		if (TerrainMappingAlpha[TerrainIndex1] <= 0.f && TerrainMappingAlpha[TerrainIndex2] <= 0.f
			&& TerrainMappingAlpha[TerrainIndex3] <= 0.f && TerrainMappingAlpha[TerrainIndex4] <= 0.f
			&& CurrentLayer == 0 && (gMapManager->InBloodCastle() == false))
		{
			int Texture = BITMAP_MAPGRASS + TerrainMappingLayer1[TerrainIndex1];

			BITMAP_t* pBitmap = Bitmaps.FindTexture(Texture);
			if (pBitmap)
			{
				float Height = pBitmap->Height * 2.f;
				BindTexture(Texture);

				if (g_terrainGrassMerged)
					EnableAlphaTest();
				else if (gMapManager->IsPKField() || IsDoppelGanger2())
					EnableAlphaBlend();

				float Width = 64.f / 256.f;
				float su = xf * Width;
				su += TerrainGrassTexture[yi & TERRAIN_SIZE_MASK];
				TEXCOORD(TerrainTextureCoord[0], su, 0.f);
				TEXCOORD(TerrainTextureCoord[1], su + Width, 0.f);
				TEXCOORD(TerrainTextureCoord[2], su + Width, 1.f);
				TEXCOORD(TerrainTextureCoord[3], su, 1.f);
				VectorCopy(TerrainVertex[0], TerrainVertex[3]);
				VectorCopy(TerrainVertex[2], TerrainVertex[1]);
				TerrainVertex[0][2] += Height;
				TerrainVertex[1][2] += Height;
				TerrainVertex[0][0] += -50.f;
				TerrainVertex[1][0] += -50.f;

				if (IsKarutanMap())
				{
					TerrainVertex[0][1] += g_fTerrainGrassWind1[TerrainIndex1];
					TerrainVertex[1][1] += g_fTerrainGrassWind1[TerrainIndex2];
				}
				else
				{
					TerrainVertex[0][1] += TerrainGrassWind[TerrainIndex1];
					TerrainVertex[1][1] += TerrainGrassWind[TerrainIndex2];
				}

				vec4_t colors[4];
				int terrain_index[] = { TerrainIndex1 , TerrainIndex2, TerrainIndex3, TerrainIndex4, };
				for (int i = 0; i < 4; i++)
				{
					colors[i][3] = 1.0;
					colors[i][0] = PrimaryTerrainLight[terrain_index[i]][0];
					colors[i][1] = PrimaryTerrainLight[terrain_index[i]][1];
					colors[i][2] = PrimaryTerrainLight[terrain_index[i]][2];
				}

				// FIX PASSO 3.5: glEnableClientState/glVertexPointer removidos
				glEnableVertexAttribArray(0);  // position
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, TerrainVertex);
				glEnableVertexAttribArray(1);  // texCoord
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, TerrainTextureCoord);
				glEnableVertexAttribArray(2);  // color
				glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, colors);

				glDrawArrays(GL_QUADS, 0, 4);

				glDisableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
				glDisableVertexAttribArray(2);

				if (g_terrainGrassMerged || gMapManager->IsPKField() || IsDoppelGanger2())
					DisableAlphaBlend();
			}
		}
	}

	// FIX PASSO 3.5: glAlphaFunc removido no Core Profile
	// glAlphaFunc(GL_GREATER, 0.2500000);
}

void RenderTerrainFace_After(float xf, float yf, int xi, int yi, float lodf)
{
	if (TerrainFlag != TERRAIN_MAP_GRASS)
	{
		int Texture;
		int Water = 0;
		if (TerrainMappingAlpha[TerrainIndex1] >= 1.f && TerrainMappingAlpha[TerrainIndex2] >= 1.f && TerrainMappingAlpha[TerrainIndex3] >= 1.f && TerrainMappingAlpha[TerrainIndex4] >= 1.f)
		{
			Texture = TerrainMappingLayer2[TerrainIndex1];
		}
		else
		{
			Texture = TerrainMappingLayer1[TerrainIndex1];
			if (TerrainMappingLayer1[TerrainIndex1] == 5)
				Water = 1;
		}

		FaceTexture(Texture, xf, yf, Water, false);
		RenderFace_After(Texture, xi, yi);
	}
}

extern PATH* path;

extern int SelectWall;

bool RenderTerrainTile(float xf, float yf, int xi, int yi, float lodf, int lodi, bool Flag)
{
	TerrainIndex1 = TERRAIN_INDEX(xi, yi);

	if ((TerrainWall[TerrainIndex1] & TW_NOGROUND) == TW_NOGROUND && !Flag)
		return false;

	TerrainIndex2 = TERRAIN_INDEX(xi + lodi, yi);
	TerrainIndex3 = TERRAIN_INDEX(xi + lodi, yi + lodi);
	TerrainIndex4 = TERRAIN_INDEX(xi, yi + lodi);

	float sx = xf * TERRAIN_SCALE;
	float sy = yf * TERRAIN_SCALE;
	float esx = sx + TERRAIN_SCALE * lodf;   // [Tier1-A] extent LOD-aware (antes: sempre +TERRAIN_SCALE)
	float esy = sy + TERRAIN_SCALE * lodf;

	Vector(sx, sy, BackTerrainHeight[TerrainIndex1], TerrainVertex[0]);
	Vector(esx, sy, BackTerrainHeight[TerrainIndex2], TerrainVertex[1]);
	Vector(esx, esy, BackTerrainHeight[TerrainIndex3], TerrainVertex[2]);
	Vector(sx, esy, BackTerrainHeight[TerrainIndex4], TerrainVertex[3]);

	if ((TerrainWall[TerrainIndex1] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[0][2] = g_fSpecialHeight;
	if ((TerrainWall[TerrainIndex2] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[1][2] = g_fSpecialHeight;
	if ((TerrainWall[TerrainIndex3] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[2][2] = g_fSpecialHeight;
	if ((TerrainWall[TerrainIndex4] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[3][2] = g_fSpecialHeight;

	if (!Flag)
	{
		RenderTerrainFace(xf, yf, xi, yi, lodf);
#ifdef SHOW_PATH_INFO
#ifdef CSK_DEBUG_MAP_PATHFINDING
		if (g_bShowPath == true)
#endif // CSK_DEBUG_MAP_PATHFINDING
		{
			if (2 <= path->GetClosedStatus(TerrainIndex1))
			{
				EnableAlphaTest();
				DisableTexture();
				glBegin(GL_TRIANGLE_FAN);
				if (4 <= path->GetClosedStatus(TerrainIndex1))
				{
					glColor4f(0.3f, 0.3f, 1.0f, 0.5f);
				}
				else
				{
					glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
				}
				for (int i = 0; i < 4; i++)
				{
					glVertex3fv(TerrainVertex[i]);
				}
				glEnd();
				DisableAlphaBlend();
			}
		}
#endif // SHOW_PATH_INFO
	}
	else
	{
#ifdef _DEBUG
		if (EditFlag != EDIT_LIGHT)
		{
			DisableTexture();
			glColor3f(0.5f, 0.5f, 0.5f);
			glBegin(GL_LINE_STRIP);
			for (int i = 0; i < 4; i++)
			{
				glVertex3fv(TerrainVertex[i]);
			}
			glEnd();
			DisableAlphaBlend();
		}
#endif// _DEBUG

		vec3_t Normal;
		FaceNormalize(TerrainVertex[0], TerrainVertex[1], TerrainVertex[2], Normal);
		bool Success = CollisionDetectLineToFace(MousePosition, MouseTarget, 3, TerrainVertex[0], TerrainVertex[1], TerrainVertex[2], TerrainVertex[3], Normal);
		if (Success == false)
		{
			FaceNormalize(TerrainVertex[0], TerrainVertex[2], TerrainVertex[3], Normal);
			Success = CollisionDetectLineToFace(MousePosition, MouseTarget, 3, TerrainVertex[0], TerrainVertex[2], TerrainVertex[3], TerrainVertex[1], Normal);
		}
		if (Success == true)
		{
			SelectFlag = true;
			SelectXF = xf;
			SelectYF = yf;
		}
#ifdef CSK_DEBUG_MAP_ATTRIBUTE
		if (EditFlag == EDIT_WALL &&
			((SelectWall == 0 && (TerrainWall[TerrainIndex1] & TW_NOMOVE) == TW_NOMOVE)
				|| (SelectWall == 2 && (TerrainWall[TerrainIndex1] & TW_SAFEZONE) == TW_SAFEZONE)
				|| (SelectWall == 6 && (TerrainWall[TerrainIndex1] & TW_CAMERA_UP) == TW_CAMERA_UP)
				|| (SelectWall == 7 && (TerrainWall[TerrainIndex1] & TW_NOATTACKZONE) == TW_NOATTACKZONE)
				|| (SelectWall == 8 && (TerrainWall[TerrainIndex1] & TW_ATT1) == TW_ATT1)
				|| (SelectWall == 9 && (TerrainWall[TerrainIndex1] & TW_ATT2) == TW_ATT2)
				|| (SelectWall == 10 && (TerrainWall[TerrainIndex1] & TW_ATT3) == TW_ATT3)
				|| (SelectWall == 11 && (TerrainWall[TerrainIndex1] & TW_ATT4) == TW_ATT4)
				|| (SelectWall == 12 && (TerrainWall[TerrainIndex1] & TW_ATT5) == TW_ATT5)
				|| (SelectWall == 13 && (TerrainWall[TerrainIndex1] & TW_ATT6) == TW_ATT6)
				|| (SelectWall == 14 && (TerrainWall[TerrainIndex1] & TW_ATT7) == TW_ATT7)
				))
		{
			DisableDepthTest();
			EnableAlphaTest();
			DisableTexture();

			glBegin(GL_TRIANGLE_FAN);
			glColor4f(1.f, 0.5f, 0.5f, 0.3f);
			for (int i = 0; i < 4; i++)
			{
				glVertex3fv(TerrainVertex[i]);
			}
			glEnd();

			DisableAlphaBlend();
		}
#endif // CSK_DEBUG_MAP_ATTRIBUTE

		return Success;
	}
	return false;
}

void RenderTerrainTile_After(float xf, float yf, int xi, int yi, float lodf, int lodi, bool Flag)
{
	TerrainIndex1 = TERRAIN_INDEX(xi, yi);
	TerrainIndex2 = TERRAIN_INDEX(xi + lodi, yi);
	TerrainIndex3 = TERRAIN_INDEX(xi + lodi, yi + lodi);
	TerrainIndex4 = TERRAIN_INDEX(xi, yi + lodi);

	float sx = xf * TERRAIN_SCALE;
	float sy = yf * TERRAIN_SCALE;

	Vector(sx, sy, BackTerrainHeight[TerrainIndex1], TerrainVertex[0]);
	Vector(sx + TERRAIN_SCALE, sy, BackTerrainHeight[TerrainIndex2], TerrainVertex[1]);
	Vector(sx + TERRAIN_SCALE, sy + TERRAIN_SCALE, BackTerrainHeight[TerrainIndex3], TerrainVertex[2]);
	Vector(sx, sy + TERRAIN_SCALE, BackTerrainHeight[TerrainIndex4], TerrainVertex[3]);

	if ((TerrainWall[TerrainIndex1] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[0][2] = g_fSpecialHeight;
	if ((TerrainWall[TerrainIndex2] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[1][2] = g_fSpecialHeight;
	if ((TerrainWall[TerrainIndex3] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[2][2] = g_fSpecialHeight;
	if ((TerrainWall[TerrainIndex4] & TW_HEIGHT) == TW_HEIGHT)
		TerrainVertex[3][2] = g_fSpecialHeight;

	if (!Flag)
	{
		if ((TerrainWall[TerrainIndex1] & TW_NOGROUND) != TW_NOGROUND)
			RenderTerrainFace_After(xf, yf, xi, yi, lodf);
	}
}

void RenderTerrainBitmapTile(float xf, float yf, float lodf, int lodi, vec3_t c[4], bool LightEnable, float Alpha, float Height = 0.f)
{
	int xi = (int)xf;
	int yi = (int)yf;
	if (xi < 0 || yi < 0 || xi >= TERRAIN_SIZE_MASK || yi >= TERRAIN_SIZE_MASK)
		return;

	float TileScale = TERRAIN_SCALE * lodf;
	float sx = xf * TERRAIN_SCALE;
	float sy = yf * TERRAIN_SCALE;
	TerrainIndex1 = TERRAIN_INDEX(xi, yi);
	TerrainIndex2 = TERRAIN_INDEX(xi + lodi, yi);
	TerrainIndex3 = TERRAIN_INDEX(xi + lodi, yi + lodi);
	TerrainIndex4 = TERRAIN_INDEX(xi, yi + lodi);
	Vector(sx, sy, BackTerrainHeight[TerrainIndex1] + Height, TerrainVertex[0]);
	Vector(sx + TileScale, sy, BackTerrainHeight[TerrainIndex2] + Height, TerrainVertex[1]);
	Vector(sx + TileScale, sy + TileScale, BackTerrainHeight[TerrainIndex3] + Height, TerrainVertex[2]);
	Vector(sx, sy + TileScale, BackTerrainHeight[TerrainIndex4] + Height, TerrainVertex[3]);

	vec3_t Light[4];
	if (LightEnable)
	{
		VectorCopy(PrimaryTerrainLight[TerrainIndex1], Light[0]);
		VectorCopy(PrimaryTerrainLight[TerrainIndex2], Light[1]);
		VectorCopy(PrimaryTerrainLight[TerrainIndex3], Light[2]);
		VectorCopy(PrimaryTerrainLight[TerrainIndex4], Light[3]);
	}

	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i < 4; i++)
	{
		if (LightEnable)
		{
			if (Alpha == 1.f)
				glColor3fv(Light[i]);
			else
				glColor4f(Light[i][0], Light[i][1], Light[i][2], Alpha);
		}
		glTexCoord2f(c[i][0], c[i][1]);
		glVertex3fv(TerrainVertex[i]);
	}
	glEnd();
}

void RenderTerrainBitmap(int Texture, int mxi, int myi, float Rotation)
{
	glColor3f(1.f, 1.f, 1.f);

	vec3_t Angle;
	Vector(0.f, 0.f, Rotation, Angle);
	float Matrix[3][4];
	AngleMatrix(Angle, Matrix);

	BindTexture(Texture);
	BITMAP_t* b = &Bitmaps[Texture];
	float TexScaleU = 64.f / b->Width;
	float TexScaleV = 64.f / b->Height;
	for (float y = 0.f; y < b->Height / 64.f; y += 1.f)
	{
		for (float x = 0.f; x < b->Width / 64.f; x += 1.f)
		{
			vec3_t p1[4], p2[4];
			Vector((x)*TexScaleU, (y)*TexScaleV, 0.f, p1[0]);
			Vector((x + 1.f) * TexScaleU, (y)*TexScaleV, 0.f, p1[1]);
			Vector((x + 1.f) * TexScaleU, (y + 1.f) * TexScaleV, 0.f, p1[2]);
			Vector((x)*TexScaleU, (y + 1.f) * TexScaleV, 0.f, p1[3]);
			for (int i = 0; i < 4; i++)
			{
				p1[i][0] -= 0.5f;
				p1[i][1] -= 0.5f;
				VectorRotate(p1[i], Matrix, p2[i]);
				p2[i][0] += 0.5f;
				p2[i][1] += 0.5f;
			}
			RenderTerrainBitmapTile((float)mxi + x, (float)myi + y, 1.f, 1, p2, true, 1.f);
		}
	}
}

void RenderTerrainAlphaBitmap(int Texture, float xf, float yf, float SizeX, float SizeY, vec3_t Light, float Rotation, float Alpha, float Height)
{
	if (Alpha == 1.f)
		glColor3fv(Light);
	else
		glColor4f(Light[0], Light[1], Light[2], Alpha);

	vec3_t Angle;
	Vector(0.f, 0.f, Rotation, Angle);
	float Matrix[3][4];
	AngleMatrix(Angle, Matrix);

	BindTexture(Texture);
	float mxf = (xf / TERRAIN_SCALE);
	float myf = (yf / TERRAIN_SCALE);
	int   mxi = (int)(mxf);
	int   myi = (int)(myf);

	float Size;
	if (SizeX >= SizeY)
		Size = SizeX;
	else
		Size = SizeY;
	float TexU = (((float)mxi - mxf) + 0.5f * Size);
	float TexV = (((float)myi - myf) + 0.5f * Size);
	float TexScaleU = 1.f / Size;
	float TexScaleV = 1.f / Size;
	Size = (float)((int)Size + 1);
	float Aspect = SizeX / SizeY;
	for (float y = -Size; y <= Size; y += 1.f)
	{
		for (float x = -Size; x <= Size; x += 1.f)
		{
			vec3_t p1[4], p2[4];
			Vector((TexU + x) * TexScaleU, (TexV + y) * TexScaleV, 0.f, p1[0]);
			Vector((TexU + x + 1.f) * TexScaleU, (TexV + y) * TexScaleV, 0.f, p1[1]);
			Vector((TexU + x + 1.f) * TexScaleU, (TexV + y + 1.f) * TexScaleV, 0.f, p1[2]);
			Vector((TexU + x) * TexScaleU, (TexV + y + 1.f) * TexScaleV, 0.f, p1[3]);
			for (int i = 0; i < 4; i++)
			{
				p1[i][0] -= 0.5f;
				p1[i][1] -= 0.5f;
				VectorRotate(p1[i], Matrix, p2[i]);
				p2[i][0] *= Aspect;
				p2[i][0] += 0.5f;
				p2[i][1] += 0.5f;
				//if((p2[i][0]>=0.f && p2[i][0]<=1.f) || (p2[i][1]>=0.f && p2[i][1]<=1.f)) Clip = true;
			}
			RenderTerrainBitmapTile((float)mxi + x, (float)myi + y, 1.f, 1, p2, false, Alpha, Height);
		}
	}
}

vec3_t  FrustrumVertex[5];
vec3_t  FrustrumFaceNormal[5];
float   FrustrumFaceD[5];
int     FrustrumBoundMinX = 0;
int     FrustrumBoundMinY = 0;
int     FrustrumBoundMaxX = TERRAIN_SIZE_MASK;
int     FrustrumBoundMaxY = TERRAIN_SIZE_MASK;

float FrustrumX[4];
float FrustrumY[4];

// ============================================================================
// Debug de culling — escreve culling_debug.log ao lado do Main.exe
// Registra contagens de blocos/tiles/objetos testados vs visíveis, os limites
// do frustum 2D e possíveis "pops" (item culled mas perto da câmera).
// ============================================================================
static char s_cullLogPath[MAX_PATH] = { 0 };
static bool s_cullLogInit = false;
static DWORD s_cullLastDump = 0;

static int  s_cullBlocksTested = 0;
static int  s_cullBlocksVisible = 0;
static int  s_cullTilesTested = 0;
static int  s_cullTilesVisible = 0;
static int  s_cullObjectsTested = 0;
static int  s_cullObjectsVisible = 0;

// Suspeito de pop: item mais próximo da câmera que foi culled
static bool   s_cullNearCulled = false;
static float  s_cullNearDist = 1e9f;
static float  s_cullNearX = 0.f;
static float  s_cullNearY = 0.f;

static void CullingDebugResolvePath()
{
	if (s_cullLogInit)
		return;
	s_cullLogInit = true;

	GetModuleFileNameA(NULL, s_cullLogPath, MAX_PATH);
	char* slash = strrchr(s_cullLogPath, '\\');
	if (slash != NULL)
	{
		strcpy_s(slash + 1, MAX_PATH - (slash + 1 - s_cullLogPath), "culling_debug.log");
	}
	else
	{
		strcpy_s(s_cullLogPath, "culling_debug.log");
	}
}

void CullingDebugResetFrame()
{
	s_cullBlocksTested = 0;
	s_cullBlocksVisible = 0;
	s_cullTilesTested = 0;
	s_cullTilesVisible = 0;
	s_cullObjectsTested = 0;
	s_cullObjectsVisible = 0;
	s_cullNearCulled = false;
	s_cullNearDist = 1e9f;
	s_cullNearX = 0.f;
	s_cullNearY = 0.f;
}

static bool CullingDebugInsideQuad(float x, float y)
{
	int j = 3;
	for (int i = 0; i < 4; j = i, i++)
	{
		float d = (FrustrumX[i] - x) * (FrustrumY[j] - y) - (FrustrumX[j] - x) * (FrustrumY[i] - y);
		if (d <= 0.f)
			return false;
	}
	return true;
}

static void CullingDebugNear(float x, float y)
{
	// Só reporta pop real: item culled que ainda está DENTRO do quadrilátero
	// do frustum 2D (área que deveria ser visível na tela). Itens atrás da
	// câmera ficam fora do quadrilátero e são culled corretamente.
	if (!CullingDebugInsideQuad(x, y))
		return;

	float dx = x - CameraPosition[0] * 0.01f;
	float dy = y - CameraPosition[1] * 0.01f;
	float d = dx * dx + dy * dy;
	if (d < s_cullNearDist)
	{
		s_cullNearDist = d;
		s_cullNearX = x;
		s_cullNearY = y;
		s_cullNearCulled = true;
	}
}

void CullingDebugBlockTest(bool visible, float x, float y)
{
	s_cullBlocksTested++;
	if (visible)
	{
		s_cullBlocksVisible++;
	}
	else
	{
		CullingDebugNear(x, y);
	}
}

void CullingDebugTileTest(bool visible, float x, float y)
{
	s_cullTilesTested++;
	if (visible)
	{
		s_cullTilesVisible++;
	}
	else
	{
		CullingDebugNear(x, y);
	}
}

void CullingDebugObjectTest(bool visible)
{
	s_cullObjectsTested++;
	if (visible)
		s_cullObjectsVisible++;
}

void CullingDebugDump()
{
	CullingDebugResolvePath();

	DWORD now = GetTickCount();
	if (now - s_cullLastDump < 500)
		return;
	s_cullLastDump = now;

	FILE* f = NULL;
	if (fopen_s(&f, s_cullLogPath, "a") != 0 || f == NULL)
		return;

	fprintf(f,
		"[T=%lu] cam=(%.1f,%.1f) far=%.0f fov=%.1f bias=%.0f frRange=%.0f\n",
		now, CameraPosition[0] * 0.01f, CameraPosition[1] * 0.01f,
		CameraViewFar, CameraFOV, g_fRenderDistanceBias, GetTerrainFrustumRange());
	fprintf(f,
		"  bounds minX=%d minY=%d maxX=%d maxY=%d  frX=(%.1f,%.1f,%.1f,%.1f) frY=(%.1f,%.1f,%.1f,%.1f)\n",
		FrustrumBoundMinX, FrustrumBoundMinY, FrustrumBoundMaxX, FrustrumBoundMaxY,
		FrustrumX[0], FrustrumX[1], FrustrumX[2], FrustrumX[3],
		FrustrumY[0], FrustrumY[1], FrustrumY[2], FrustrumY[3]);
	fprintf(f,
		"  blocks %d/%d  tiles %d/%d  objects %d/%d\n",
		s_cullBlocksVisible, s_cullBlocksTested,
		s_cullTilesVisible, s_cullTilesTested,
		s_cullObjectsVisible, s_cullObjectsTested);
	if (s_cullNearCulled)
	{
		fprintf(f,
			"  POSSIBLE POP at (%.1f,%.1f) dist=%.1f tiles\n",
			s_cullNearX, s_cullNearY, sqrtf(s_cullNearDist));
	}
	fclose(f);

	CullingDebugResetFrame();
}

extern int GetScreenWidth();


void CreateFrustrum2D(vec3_t Position)
{
	float Width = 0.0f;
	float WidthFar = 0.0f;
	float WidthNear = 0.0f;
	float CameraViewFar = 0.0f;
	float CameraViewNear = 0.0f;
	float CameraViewTarget = 0.0f;

#if MAIN_UPDATE > 303
	if (gmProtect->ScreenType == 0 && (GMProtect->LookAndFeelType() != 3 && GMProtect->LookAndFeelType() != 4))
	{
		Width = (float)GetScreenWidth() / GetWindowsX;
	}
	else
	{
		if (WindowWidth > 800 || (GMProtect->LookAndFeelType() == 3 || GMProtect->LookAndFeelType() == 4))
			Width = 1.0;
		else
			Width = (float)GetScreenWidth() / GetWindowsX;
	}
#else
	Width = 1.0;
#endif // MAIN_UPDATE > 303

	CAMERA_INFO* CurrentCam = CameraFactorPtr->CurrentCam();

	int iWorld = gMapManager->currentMap;

	if (iWorld == WD_6STADIUM && (FindText(Hero->ID, "webzen") || FindText(Hero->ID, "webzen2")))
	{
		CameraViewFar = CurrentCam->camviewFar[0];
		CameraViewNear = CameraViewFar * 0.05f;
		CameraViewTarget = CameraViewFar * 0.47f;
		WidthFar = CurrentCam->WidthFar[0] * Width;
		WidthNear = CurrentCam->WidthNear[0] * Width;
	}
	else
	{
		if (gMapManager->InBattleCastle() && SceneFlag == MAIN_SCENE)
		{
			if (battleCastle::InBattleCastle2(Hero->Object.Position) && (Hero->Object.Position[0] < 17100.f || Hero->Object.Position[0]> 18300.f))
			{
				CameraViewFar = CurrentCam->camviewFar[1];// * 0.1f;
				CameraViewNear = CameraViewFar * 0.19f;//0.22
				CameraViewTarget = CameraViewFar * 0.47f;//0.47
				WidthFar = CurrentCam->WidthFar[1] * Width; // 1140.f
				WidthNear = CurrentCam->WidthNear[1] * Width; // 540.f
			}
			else
			{
				CameraViewFar = CurrentCam->camviewFar[2];// * 0.1f;
				CameraViewNear = CameraViewFar * 0.19f;//0.22
				CameraViewTarget = CameraViewFar * 0.47f;//0.47
				WidthFar = CurrentCam->WidthFar[2] * Width; // 1140.f
				WidthNear = CurrentCam->WidthNear[2] * Width; // 540.f
			}
		}
		else if (iWorld == WD_62SANTA_TOWN)
		{
			CameraViewFar = CurrentCam->camviewFar[3];
			CameraViewNear = CameraViewFar * 0.19f;
			CameraViewTarget = CameraViewFar * 0.47f;
			WidthFar = CurrentCam->WidthFar[3] * Width;
			WidthNear = CurrentCam->WidthNear[3] * Width;
		}
		else if (gMapManager->IsPKField() || IsDoppelGanger2())
		{
			CameraViewFar = CurrentCam->camviewFar[5];
			CameraViewNear = CameraViewFar * 0.0166666666666667;
			CameraViewTarget = CameraViewFar * 0.2515151515151515;
			WidthFar = Width * CurrentCam->WidthFar[5];
			WidthNear = Width * CurrentCam->WidthNear[5];
		}
		else
		{
			static int CameraLevel;

			if ((int)CameraDistanceTarget >= (int)CameraDistance)
				CameraLevel = g_shCameraLevel;

			if (CameraLevel != 0)
			{
				switch (CameraLevel)
				{
				case 1:
					Width += 0.1f;
					CameraViewFar = 2700.f;
					CameraViewNear = CameraViewFar * 0.19f;
					CameraViewTarget = CameraViewFar * 0.47f;
					WidthFar = 1200.f * Width;
					WidthNear = 540.f * Width;
					break;
				case 2:
					Width += 0.1f;
					CameraViewFar = 3000.f;
					CameraViewNear = CameraViewFar * 0.19f;
					CameraViewTarget = CameraViewFar * 0.47f;
					WidthFar = 1300.f * Width;
					WidthNear = 540.f * Width;
					break;
				case 3:
					Width += 0.1f;
					CameraViewFar = 3300.f;
					CameraViewNear = CameraViewFar * 0.19f;
					CameraViewTarget = CameraViewFar * 0.47f;
					WidthFar = 1500.f * Width;
					WidthNear = 580.f * Width;
					break;
				case 4:
				case 5:
					Width += 0.1f;
					CameraViewFar = 3400.f;
					CameraViewNear = CameraViewFar * 0.19f;
					CameraViewTarget = CameraViewFar * 0.47f;
					WidthFar = 1600.f * Width;
					WidthNear = 660.f * Width;
					break;
				}
			}
			else
			{
				if (SceneFlag == LOG_IN_SCENE)
				{
					if (iWorld == WD_55LOGINSCENE)
					{
						Width *= 17.0 * 6.0;
					}
				}
				else if (SceneFlag == CHARACTER_SCENE)
				{
					if (iWorld == WD_54CHARACTERSCENE || iWorld == WD_74NEW_CHARACTER_SCENE)
					{
						Width *= 10.0 * 0.575995f;
					}
					else
					{
						Width *= 9.1f * 0.404998f;
					}
				}
				else if (g_Direction.m_CKanturu.IsMayaScene())
				{
					Width *= 10.0f * 0.115f;
				}
				else
				{
#if MAIN_UPDATE > 303
					if (WindowWidth > 800 || (GMProtect->LookAndFeelType() == 3 || GMProtect->LookAndFeelType() == 4))
						Width = 1.0;
					else
						Width = (float)GetScreenWidth() / GetWindowsX;
#else
					Width = 1.0;
#endif // MAIN_UPDATE > 303
				}

				if (SceneFlag == LOG_IN_SCENE)
				{
					if (iWorld == WD_78NEW_CHARACTER_SCENE)
					{
						CameraViewFar = 44880.f;
						CameraViewNear = 20400.f;
						CameraViewTarget = 19584.f;
						WidthFar = 6300.f * Width;
						WidthNear = 690.f * Width;
					}
					else
					{
						if (gmProtect->SceneLogin != 0)
						{
							CameraViewFar = gCameraManager->CalcViewFar(2400.0);
							Width = gCameraManager->CalcWidthCam();

							CameraViewNear = CameraViewFar * 0.19f;
							CameraViewTarget = CameraViewFar * 0.47f;
							WidthFar = 1190.0 * Width;
							WidthNear = 540.0 * Width;
						}
						else
						{
							CameraViewFar = 530400.0;
							CameraViewNear = 20400.0;
							CameraViewTarget = 20400.0;
							WidthFar = 5000.f * Width;
							WidthNear = 300.f * Width;
						}
					}
				}
				else if (SceneFlag == CHARACTER_SCENE)
				{
					if (gmProtect->SceneCharacter != 0)
					{
						CameraViewFar = gCameraManager->CalcViewFar(2400.0);
						Width = gCameraManager->CalcWidthCam();
					}
					else
					{
						CameraViewFar = 7370.9639;
					}

					CameraViewNear = CameraViewFar * 0.19f;
					CameraViewTarget = CameraViewFar * 0.47f;
					WidthFar = 1190.0 * Width;
					WidthNear = 540.0 * Width;
				}
				else
				{
					if (iWorld == WD_39KANTURU_3RD)
						CameraViewFar = 2300.0;
					else
						CameraViewFar = CurrentCam->camviewFar[4];

					CameraViewNear = CameraViewFar * 0.19f;//0.22
					CameraViewTarget = CameraViewFar * 0.47f;//0.47
					WidthFar = CurrentCam->WidthFar[4] * Width; // 1140.f
					WidthNear = CurrentCam->WidthNear[4] * Width; // 540.f
				}
			}
		}
	}

	// Caps do frustum 2D do chao (quadrado preto nas bordas = WidthFar curto).
	// NAO reduzir WidthFar quando capar o Far (antes o *r encolhia a lateral esquerda).
	if (SceneFlag == MAIN_SCENE)
	{
		const float kMaxFar = 4500.f;
		const float kMaxWidthFar = 3600.f; // bem mais aberto (era 1300 → 2400 → 3600)

		// Boost em monitores largos (16:9 / 16:10)
		if (WindowWidth > 0 && WindowHeight > 0)
		{
			const float aspect = (float)WindowWidth / (float)WindowHeight;
			if (aspect > 1.45f)
			{
				float boost = aspect / (4.0f / 3.0f);
				if (boost > 1.65f) boost = 1.65f;
				WidthFar *= boost;
				WidthNear *= boost;
			}
		}

		// Extra abertura lateral (corrige "chao preto" / quadrado)
		WidthFar *= 1.20f;
		WidthNear *= 1.15f;

		if (CameraViewFar > kMaxFar && CameraViewFar > 0.f)
		{
			// So limita profundidade — nao encolhe largura do chao
			const float r = kMaxFar / CameraViewFar;
			CameraViewFar = kMaxFar;
			CameraViewNear *= r;
			CameraViewTarget *= r;
		}
		if (WidthFar > kMaxWidthFar && WidthFar > 0.f)
		{
			const float r = kMaxWidthFar / WidthFar;
			WidthFar = kMaxWidthFar;
			WidthNear *= r;
		}
		if (WidthNear > 1600.f)
			WidthNear = 1600.f;
	}

	vec3_t Angle, p[4];
	float Matrix[3][4];

	// Leve assimetria: lado esquerdo (-X) um pouco mais aberto (user: esq. ainda preto)
	const float WidthFarL = WidthFar * 1.12f;
	const float WidthFarR = WidthFar;
	const float WidthNearL = WidthNear * 1.10f;
	const float WidthNearR = WidthNear;

	Vector(-WidthFarL, CameraViewFar - CameraViewTarget, 0.f, p[0]);
	Vector(WidthFarR, CameraViewFar - CameraViewTarget, 0.f, p[1]);
	Vector(WidthNearR, CameraViewNear - CameraViewTarget, 0.f, p[2]);
	Vector(-WidthNearL, CameraViewNear - CameraViewTarget, 0.f, p[3]);

	if (iWorld == WD_6STADIUM && (FindText(Hero->ID, "webzen") || FindText(Hero->ID, "webzen2")))
	{
		Vector(0.f, 0.f, -CameraAngle[2], Angle);
	}
	else if (iWorld == WD_73NEW_LOGIN_SCENE || iWorld == WD_77NEW_LOGIN_SCENE)
	{
		VectorScale(CameraAngle, -1.0f, Angle);
		if (iWorld == WD_73NEW_LOGIN_SCENE)
		{
			CCameraMove::GetInstancePtr()->SetFrustumAngle(89.5f);
			vec3_t _Temp = { CCameraMove::GetInstancePtr()->GetFrustumAngle(), 0.0f, 0.0f };
			VectorAdd(Angle, _Temp, Angle);
		}
	}
	else if (iWorld == WD_55LOGINSCENE)
	{
		Vector(0.f, 0.f, CameraAngle[2], Angle);
	}
	else
	{
		if (SceneFlag == MAIN_SCENE)
		{
			Vector(0.f, 0.f, CurrentCam->camAngle, Angle);
		}
		else
		{
			Vector(0.f, 0.f, 45.0, Angle);
		}
	}

	AngleMatrix(Angle, Matrix);
	vec3_t Frustrum[4];
	for (int i = 0; i < 4; i++)
	{
		VectorRotate(p[i], Matrix, Frustrum[i]);
		VectorAdd(Frustrum[i], Position, Frustrum[i]);
		FrustrumX[i] = Frustrum[i][0] * 0.01f;
		FrustrumY[i] = Frustrum[i][1] * 0.01f;
	}
}

bool TestFrustrum2D(float x, float y, float Range)
{
	if (!((gmProtect->SceneLogin == 1 && SceneFlag == LOG_IN_SCENE) || (gmProtect->SceneCharacter == 1 && SceneFlag == CHARACTER_SCENE)))
	{
		if (SceneFlag == SERVER_LIST_SCENE || SceneFlag == WEBZEN_SCENE || SceneFlag == LOADING_SCENE)
			return true;

		// Aplicar bias uma vez antes do loop (anti-pop, port do 6.Main_UP52_Full)
		float adjustedRange = Range + g_fRenderDistanceBias;

		int j = 3;
		for (int i = 0; i < 4; j = i, i++)
		{
			float d = (FrustrumX[i] - x) * (FrustrumY[j] - y) - (FrustrumX[j] - x) * (FrustrumY[i] - y);

			if (d <= adjustedRange)
			{
				return false;
			}
		}
	}
	return true;
}

void CreateFrustrum(float Aspect, vec3_t position)
{
	float baseFOV = 35.f;
	float frustumMultiplier = 1.0f;

	if (CameraFOV < baseFOV)
	{
		float fovRatio = baseFOV / CameraFOV;
		frustumMultiplier = fovRatio * fovRatio;
		if (frustumMultiplier > 2.0f)
			frustumMultiplier = 2.0f;
	}
	else if (CameraFOV > baseFOV)
	{
		float fovRatio = CameraFOV / baseFOV;
		frustumMultiplier = fovRatio * fovRatio;
		if (frustumMultiplier > 2.0f)
			frustumMultiplier = 2.0f;
	}

	float farPlaneMultiplier = 1.4f;
	if (CameraFOV > baseFOV)
	{
		float fovRatio = CameraFOV / baseFOV;
		farPlaneMultiplier = 1.4f * fovRatio * fovRatio;
		if (farPlaneMultiplier > 2.0f)
			farPlaneMultiplier = 2.0f;
	}

	float Distance = (CameraViewFar * farPlaneMultiplier) * 0.9f * frustumMultiplier;
	float Width = tanf(CameraFOV * 0.5f * Q_PI / 180.f) * Distance * Aspect + 100.f;
	float Height = Width * 3.f / 4.f;

	if (CameraFOV > baseFOV)
	{
		float heightMultiplier = CameraFOV / baseFOV;
		Height *= heightMultiplier;
	}

	vec3_t Temp[5];
	Vector(0.f, 0.f, 0.f, Temp[0]);
	Vector(-Width, Height, -Distance, Temp[1]);
	Vector(Width, Height, -Distance, Temp[2]);
	Vector(Width, -Height, -Distance, Temp[3]);
	Vector(-Width, -Height, -Distance, Temp[4]);

	float FrustrumMinX = (float)TERRAIN_SIZE * TERRAIN_SCALE;
	float FrustrumMinY = (float)TERRAIN_SIZE * TERRAIN_SCALE;
	float FrustrumMaxX = 0.f;
	float FrustrumMaxY = 0.f;
	float Matrix[3][4];

	GetOpenGLMatrix(Matrix);

	for (int i = 0; i < 5; i++)
	{
		vec3_t t;
		VectorIRotate(Temp[i], Matrix, t);
		VectorAdd(t, CameraPosition, FrustrumVertex[i]);

		if (FrustrumMinX > FrustrumVertex[i][0])
			FrustrumMinX = FrustrumVertex[i][0];

		if (FrustrumMinY > FrustrumVertex[i][1])
			FrustrumMinY = FrustrumVertex[i][1];

		if (FrustrumMaxX < FrustrumVertex[i][0])
			FrustrumMaxX = FrustrumVertex[i][0];

		if (FrustrumMaxY < FrustrumVertex[i][1])
			FrustrumMaxY = FrustrumVertex[i][1];
	}

	int tileWidth = 4;

	FrustrumBoundMinX = (int)(FrustrumMinX / TERRAIN_SCALE) / tileWidth * tileWidth - tileWidth;
	FrustrumBoundMinY = (int)(FrustrumMinY / TERRAIN_SCALE) / tileWidth * tileWidth - tileWidth;
	FrustrumBoundMaxX = (int)(FrustrumMaxX / TERRAIN_SCALE) / tileWidth * tileWidth + tileWidth;
	FrustrumBoundMaxY = (int)(FrustrumMaxY / TERRAIN_SCALE) / tileWidth * tileWidth + tileWidth;

	if (g_fRenderDistanceBias < 0.f)
	{
		int steps = (int)floorf((-g_fRenderDistanceBias) / 40.f + 0.5f);
		int extra = (steps > 0 ? steps : 1) * tileWidth;
		FrustrumBoundMinX -= extra;
		FrustrumBoundMinY -= extra;
		FrustrumBoundMaxX += extra;
		FrustrumBoundMaxY += extra;
	}

	if (FrustrumBoundMinX < 0)
		FrustrumBoundMinX = 0;

	if (FrustrumBoundMinY < 0)
		FrustrumBoundMinY = 0;

	if (FrustrumBoundMaxX < 0)
		FrustrumBoundMaxX = 0;

	if (FrustrumBoundMaxY < 0)
		FrustrumBoundMaxY = 0;

	if (FrustrumBoundMinX > TERRAIN_SIZE_MASK - tileWidth)
		FrustrumBoundMinX = TERRAIN_SIZE_MASK - tileWidth;

	if (FrustrumBoundMinY > TERRAIN_SIZE_MASK - tileWidth)
		FrustrumBoundMinY = TERRAIN_SIZE_MASK - tileWidth;

	if (FrustrumBoundMaxX > TERRAIN_SIZE_MASK - tileWidth)
		FrustrumBoundMaxX = TERRAIN_SIZE_MASK - tileWidth;

	if (FrustrumBoundMaxY > TERRAIN_SIZE_MASK - tileWidth)
		FrustrumBoundMaxY = TERRAIN_SIZE_MASK - tileWidth;

	FaceNormalize(FrustrumVertex[0], FrustrumVertex[1], FrustrumVertex[2], FrustrumFaceNormal[0]);
	FaceNormalize(FrustrumVertex[0], FrustrumVertex[2], FrustrumVertex[3], FrustrumFaceNormal[1]);
	FaceNormalize(FrustrumVertex[0], FrustrumVertex[3], FrustrumVertex[4], FrustrumFaceNormal[2]);
	FaceNormalize(FrustrumVertex[0], FrustrumVertex[4], FrustrumVertex[1], FrustrumFaceNormal[3]);
	FaceNormalize(FrustrumVertex[3], FrustrumVertex[2], FrustrumVertex[1], FrustrumFaceNormal[4]);

	FrustrumFaceD[0] = -DotProduct(FrustrumVertex[0], FrustrumFaceNormal[0]);
	FrustrumFaceD[1] = -DotProduct(FrustrumVertex[0], FrustrumFaceNormal[1]);
	FrustrumFaceD[2] = -DotProduct(FrustrumVertex[0], FrustrumFaceNormal[2]);
	FrustrumFaceD[3] = -DotProduct(FrustrumVertex[0], FrustrumFaceNormal[3]);
	FrustrumFaceD[4] = -DotProduct(FrustrumVertex[1], FrustrumFaceNormal[4]);

	CreateFrustrum2D(position);
}

bool TestFrustrum(vec3_t Position, float Range)
{
	for (int i = 0; i < 5; i++)
	{
		float Value;
		Value = FrustrumFaceD[i] + DotProduct(Position, FrustrumFaceNormal[i]);
		if (Value < -Range)
			return false;
	}
	return true;
}

#ifdef DYNAMIC_FRUSTRUM

void CFrustrum::Make(vec3_t vEye, float fFov, float fAspect, float fDist)
{
	float Width = tanf(fFov * 0.5f * 3.141592f / 180.f) * fDist * fAspect + 100.f;
	float Height = Width * 3.f / 4.f;
	vec3_t Temp[5];
	vec3_t FrustrumVertex[6];
	Vector(0.f, 0.f, 0.f, Temp[0]);
	Vector(-Width, Height, -fDist, Temp[1]);
	Vector(Width, Height, -fDist, Temp[2]);
	Vector(Width, -Height, -fDist, Temp[3]);
	Vector(-Width, -Height, -fDist, Temp[4]);

	float Matrix[3][4];
	GetOpenGLMatrix(Matrix);
	for (int i = 0; i < 6; i++)
	{
		vec3_t t;
		VectorIRotate(Temp[i], Matrix, t);
		VectorAdd(t, vEye, FrustrumVertex[i]);
	}

	FaceNormalize(FrustrumVertex[0], FrustrumVertex[1], FrustrumVertex[2], m_FrustrumNorm[0]);
	FaceNormalize(FrustrumVertex[0], FrustrumVertex[2], FrustrumVertex[3], m_FrustrumNorm[1]);
	FaceNormalize(FrustrumVertex[0], FrustrumVertex[3], FrustrumVertex[4], m_FrustrumNorm[2]);
	FaceNormalize(FrustrumVertex[0], FrustrumVertex[4], FrustrumVertex[1], m_FrustrumNorm[3]);
	FaceNormalize(FrustrumVertex[3], FrustrumVertex[2], FrustrumVertex[1], m_FrustrumNorm[4]);
	m_FrustrumD[0] = -DotProduct(FrustrumVertex[0], m_FrustrumNorm[0]);
	m_FrustrumD[1] = -DotProduct(FrustrumVertex[0], m_FrustrumNorm[1]);
	m_FrustrumD[2] = -DotProduct(FrustrumVertex[0], m_FrustrumNorm[2]);
	m_FrustrumD[3] = -DotProduct(FrustrumVertex[0], m_FrustrumNorm[3]);
	m_FrustrumD[4] = -DotProduct(FrustrumVertex[1], m_FrustrumNorm[4]);
}

void CFrustrum::Create(vec3_t vEye, float fFov, float fAspect, float fDist)
{
	Make(vEye, fFov, fAspect, fDist);

	SetEye(vEye);
	SetFOV(fFov);
	SetAspect(fAspect);
	SetDist(fDist);
}

bool CFrustrum::Test(vec3_t vPos, float fRange)
{
	for (int i = 0; i < 6; ++i)
	{
		float fValue;
		fValue = m_FrustrumD[i] + DotProduct(vPos, m_FrustrumNorm[i]);
		if (fValue < -fRange) return false;
	}
	return true;
}

void CFrustrum::Reset()
{
	Make(m_vEye, m_fFov, m_fAspect, m_fDist);
}

void ResetAllFrustrum()
{
	FrustrumMap_t::iterator iter = g_FrustrumMap.begin();
	for (; iter != g_FrustrumMap.end(); ++iter)
	{
		CFrustrum* pData = iter->second;
		if (!pData) continue;
		pData->SetEye(CameraPosition);
		pData->Reset();
	}
}

CFrustrum* FindFrustrum(unsigned int iID)
{
	FrustrumMap_t::iterator iter = g_FrustrumMap.find(iID);
	if (iter != g_FrustrumMap.end()) return (CFrustrum*)iter->second;
	return NULL;
}

void DeleteAllFrustrum()
{
	FrustrumMap_t::iterator iter = g_FrustrumMap.begin();
	for (; iter != g_FrustrumMap.end(); ++iter)
	{
		CFrustrum* pData = iter->second;
		SAFE_DELETE(pData);
	}
	g_FrustrumMap.clear();
}

#endif// DYNAMIC_FRUSTRUM


/*bool TestFrustrum(vec3_t Position,float Range)
{
	int j = 3;
	for(int i=0;i<4;j=i,i++)
	{
		float d = (Frustrum[i][0]-Position[0]) * (Frustrum[j][1]-Position[1]) -
				  (Frustrum[j][0]-Position[0]) * (Frustrum[i][1]-Position[1]);
		if(d < 0.f) return false;
	}
	return true;
}*/

extern int RainCurrent;
extern int EnableEvent;

	void InitTerrainLight()
	{
		static int s_prevMinX = INT_MIN;
		static int s_prevMinY = INT_MIN;
		static int s_prevMaxX = INT_MIN;
		static int s_prevMaxY = INT_MIN;

		bool bFrustumUnchanged =
			s_prevMinX == FrustrumBoundMinX &&
			s_prevMinY == FrustrumBoundMinY &&
			s_prevMaxX == FrustrumBoundMaxX &&
			s_prevMaxY == FrustrumBoundMaxY;

		int xi, yi;

		if (!bFrustumUnchanged)
		{
			yi = FrustrumBoundMinY;
			for (; yi <= FrustrumBoundMaxY + 3; yi += 1)
			{
				xi = FrustrumBoundMinX;
				for (; xi <= FrustrumBoundMaxX + 3; xi += 1)
				{
					int Index = TERRAIN_INDEX_REPEAT(xi, yi);
					VectorCopy(BackTerrainLight[Index], PrimaryTerrainLight[Index]);
				}
			}
			s_prevMinX = FrustrumBoundMinX;
			s_prevMinY = FrustrumBoundMinY;
			s_prevMaxX = FrustrumBoundMaxX;
			s_prevMaxY = FrustrumBoundMaxY;
		}
	float WindScale;
	float WindSpeed;

	if (EnableEvent == 0)
	{
		WindScale = 10.f;
		WindSpeed = (int)WorldTime % (360000 * 2) * (0.002f);
	}
	else
	{
		WindScale = 10.f;
		WindSpeed = (int)WorldTime % 36000 * (0.01f);
	}

	float WindScale1 = 0.f;
	float WindSpeed1 = 0.f;
	if (IsKarutanMap())
	{
		WindScale1 = 15.f;
		WindSpeed1 = (int)WorldTime % 36000 * (0.008f);
	}
	yi = FrustrumBoundMinY;

	for (; yi <= min(FrustrumBoundMaxY + 3, TERRAIN_SIZE_MASK); yi += 1)
	{
		xi = FrustrumBoundMinX;
		float xf = (float)xi;
		for (; xi <= min(FrustrumBoundMaxX + 3, TERRAIN_SIZE_MASK); xi += 1, xf += 1.f)
		{
			int Index = TERRAIN_INDEX(xi, yi);
			if (gMapManager->currentMap == WD_8TARKAN)
			{
				TerrainGrassWind[Index] = sinf(WindSpeed + xf * 50.f) * WindScale;
			}
			else if (IsKarutanMap())
			{
				TerrainGrassWind[Index] = sinf(WindSpeed + xf * 50.f) * WindScale;
				g_fTerrainGrassWind1[Index] = sinf(WindSpeed1 + xf * 50.f) * WindScale1;
			}

			else if (gMapManager->currentMap == WD_57ICECITY || gMapManager->currentMap == WD_58ICECITY_BOSS)
			{
				WindScale = 60.f;
				TerrainGrassWind[Index] = sinf(WindSpeed + xf * 50.f) * WindScale;
			}
			else
			{
				TerrainGrassWind[Index] = sinf(WindSpeed + xf * 5.f) * WindScale;
			}
		}
	}
}

void InitTerrainShadow()
{
	/*int xi,yi;
	yi = FrustrumBoundMinY*2;
	for(;yi<=FrustrumBoundMaxY*2;yi+=2)
	{
		xi = FrustrumBoundMinX*2;
		for(;xi<=FrustrumBoundMaxX*2;xi+=2)
		{
			int Index1 = (yi  )*512+(xi  );
			int Index2 = (yi  )*512+(xi+1);
			int Index3 = (yi+1)*512+(xi+1);
			int Index4 = (yi+1)*512+(xi  );
			if(TerrainShadow[Index1] >= 1.f)
			{
				Vector(0.f,0.f,0.f,PrimaryTerrainLight[Index1]);
				Vector(0.f,0.f,0.f,PrimaryTerrainLight[Index2]);
				Vector(0.f,0.f,0.f,PrimaryTerrainLight[Index3]);
				Vector(0.f,0.f,0.f,PrimaryTerrainLight[Index4]);
			}
		}
	}*/
}

void Ray(int sx1, int sy1, int sx2, int sy2)
{
	/*int ShadowIndex = (sy1*TERRAIN_SIZE*2+sx1);
	if(TerrainShadow[ShadowIndex]==1.f) return;

	int nx1,ny1,d1,d2,len1,len2;
	int px1 = sx2-sx1;
	int py1 = sy2-sy1;
	if(px1 < 0  ) {px1 = -px1;nx1 =-1             ;} else nx1 = 1;
	if(py1 < 0  ) {py1 = -py1;ny1 =-TERRAIN_SIZE*2;} else ny1 = TERRAIN_SIZE*2;
	if(px1 > py1) {len1 = px1;len2 = py1;d1 = ny1;d2 = nx1;}
	else          {len1 = py1;len2 = px1;d1 = nx1;d2 = ny1;}

	int error = 0,count = 0;
	float Shadow = 0.f;
	do{
		TerrainShadow[ShadowIndex] = Shadow;
		int x = ShadowIndex%(TERRAIN_SIZE*2)/2;
		int y = ShadowIndex/(TERRAIN_SIZE*2)/2;
		if(TerrainWall[TERRAIN_INDEX(x,y)] >= 5)
			Shadow = 1.f;
		error += len2;
		if(error > len1/2)
		{
			ShadowIndex += d1;
			error -= len1;
		}
		ShadowIndex += d2;
	} while(++count <= len1);*/
}

void InitTerrainRay(int HeroX, int HeroY)
{
	/*TerrainShadow[HeroY*512+HeroX] = 0.f;

	int xi,yi;
	yi = FrustrumBoundMinY*2;
	for(;yi<=FrustrumBoundMaxY*2;yi++)
	{
		xi = FrustrumBoundMinX*2;
		for(;xi<=FrustrumBoundMaxX*2;xi++)
		{
			TerrainShadow[(yi*TERRAIN_SIZE*2+xi)] = -1.f;
		}
	}
	yi = FrustrumBoundMinY*2;
	for(;yi<=FrustrumBoundMaxY*2;yi++)
	{
		xi = FrustrumBoundMinX*2;
		for(;xi<=FrustrumBoundMaxX*2;xi++)
		{
			Ray(HeroX,HeroY,xi,yi);
		}
	}*/
}

// ============================================================================
// TERRAIN BATCH SYSTEM — acumula tiles por textura e desenha com glDrawArrays
// Toggle: config.ini [Performance2] TerrainBatch=1 (1=on, 0=off)
// ============================================================================
struct TerrainBatchVertex
{
	float x, y, z;       // position
	float u, v;          // texcoord
	float r, g, b;       // color
	float a;             // alpha
};

static const int MAX_BATCH_TEXTURES = 64;
static const int MAX_BATCH_VERTS = 16384;

static TerrainBatchVertex g_batchVerts[MAX_BATCH_TEXTURES][MAX_BATCH_VERTS];
static int g_batchCount[MAX_BATCH_TEXTURES];
static int g_batchTextureMap[MAX_BATCH_TEXTURES];
static int g_batchGroupCount = 0;

void TerrainBatchInit()
{
	if (g_terrainBatchInited) return;
	g_terrainBatchInited = true;
	g_terrainBatchEnabled = (GetPrivateProfileIntA("Performance2", "TerrainBatch", 1, ".\\config.ini") != 0);
}

int TerrainBatchFindOrCreateGroup(int textureId)
{
	for (int i = 0; i < g_batchGroupCount; i++)
	{
		if (g_batchTextureMap[i] == textureId)
			return i;
	}
	if (g_batchGroupCount < MAX_BATCH_TEXTURES)
	{
		int idx = g_batchGroupCount++;
		g_batchTextureMap[idx] = textureId;
		g_batchCount[idx] = 0;
		return idx;
	}
	return -1;
}

void TerrainBatchAddTile(int textureId, vec3_t* verts, vec2_t* texcoords, vec3_t* colors, float alpha)
{
	int gi = TerrainBatchFindOrCreateGroup(textureId);
	if (gi < 0) return;
	if (g_batchCount[gi] + 6 > MAX_BATCH_VERTS) return;

	TerrainBatchVertex* buf = &g_batchVerts[gi][g_batchCount[gi]];

	buf[0].x = verts[0][0]; buf[0].y = verts[0][1]; buf[0].z = verts[0][2];
	buf[0].u = texcoords[0][0]; buf[0].v = texcoords[0][1];
	buf[0].r = colors[0][0]; buf[0].g = colors[0][1]; buf[0].b = colors[0][2]; buf[0].a = alpha;

	buf[1].x = verts[1][0]; buf[1].y = verts[1][1]; buf[1].z = verts[1][2];
	buf[1].u = texcoords[1][0]; buf[1].v = texcoords[1][1];
	buf[1].r = colors[1][0]; buf[1].g = colors[1][1]; buf[1].b = colors[1][2]; buf[1].a = alpha;

	buf[2].x = verts[2][0]; buf[2].y = verts[2][1]; buf[2].z = verts[2][2];
	buf[2].u = texcoords[2][0]; buf[2].v = texcoords[2][1];
	buf[2].r = colors[2][0]; buf[2].g = colors[2][1]; buf[2].b = colors[2][2]; buf[2].a = alpha;

	buf[3].x = verts[0][0]; buf[3].y = verts[0][1]; buf[3].z = verts[0][2];
	buf[3].u = texcoords[0][0]; buf[3].v = texcoords[0][1];
	buf[3].r = colors[0][0]; buf[3].g = colors[0][1]; buf[3].b = colors[0][2]; buf[3].a = alpha;

	buf[4].x = verts[2][0]; buf[4].y = verts[2][1]; buf[4].z = verts[2][2];
	buf[4].u = texcoords[2][0]; buf[4].v = texcoords[2][1];
	buf[4].r = colors[2][0]; buf[4].g = colors[2][1]; buf[4].b = colors[2][2]; buf[4].a = alpha;

	buf[5].x = verts[3][0]; buf[5].y = verts[3][1]; buf[5].z = verts[3][2];
	buf[5].u = texcoords[3][0]; buf[5].v = texcoords[3][1];
	buf[5].r = colors[3][0]; buf[5].g = colors[3][1]; buf[5].b = colors[3][2]; buf[5].a = alpha;

	g_batchCount[gi] += 6;
}

void TerrainBatchReset()
{
	g_batchGroupCount = 0;
	for (int i = 0; i < MAX_BATCH_TEXTURES; i++)
		g_batchCount[i] = 0;
}

void TerrainBatchFlush()
{
	for (int i = 0; i < g_batchGroupCount; i++)
	{
		if (g_batchCount[i] <= 0) continue;

		BindTexture(g_batchTextureMap[i]);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainBatchVertex), &g_batchVerts[i][0].x);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainBatchVertex), &g_batchVerts[i][0].u);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainBatchVertex), &g_batchVerts[i][0].r);

		glDrawArrays(GL_TRIANGLES, 0, g_batchCount[i]);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	TerrainBatchReset();
}

// ============================================================================

// [Tier1-A] LOD por aneis de distancia da camera (centro do bloco 4x4, em tiles):
//   <=10 tiles: lodi=1 (16 tiles) | <=24: lodi=2 (4 tiles) | resto: lodi=4 (1 tile)
// Indices sempre multiplos de 4 -> alinhados a qualquer lodi. Desativavel via
// config.ini [Performance2] TerrainLOD=0.
static bool  s_terrainLODEnabled = true;
static int   s_terrainLODInited = 0;
static const float TERRAIN_LOD_NEAR2 = 10.f * 10.f;   // dist^2 em tiles
static const float TERRAIN_LOD_FAR2 = 24.f * 24.f;

void RenderTerrainBlock(float xf, float yf, int xi, int yi, bool EditFlag)
{
	if (!s_terrainLODInited)
	{
		char buf[8] = { 0 };
		GetPrivateProfileStringA("Performance2", "TerrainLOD", "1", buf, sizeof(buf), ".\\config.ini");
		s_terrainLODEnabled = (atoi(buf) != 0);
		s_terrainLODInited = 1;
	}
	int lodi = 1;
	if (s_terrainLODEnabled && !EditFlag)
	{
		float dx = CameraPosition[0] - (xf + 2.f) * TERRAIN_SCALE;   // CameraPosition em world units
		float dy = CameraPosition[1] - (yf + 2.f) * TERRAIN_SCALE;
		float dist2Tiles = (dx * dx + dy * dy) / (TERRAIN_SCALE * TERRAIN_SCALE);
		if (dist2Tiles > TERRAIN_LOD_FAR2)       lodi = 4;
		else if (dist2Tiles > TERRAIN_LOD_NEAR2) lodi = 2;
	}
	float lodf = (float)lodi;
	for (int i = 0; i < 4; i += lodi)
	{
		float temp = xf;
		for (int j = 0; j < 4; j += lodi)
		{
			bool visible = TestFrustrum2D(xf + 0.5f, yf + 0.5f, -8.f) || CameraTopViewEnable;
			CullingDebugTileTest(visible, xf + 0.5f, yf + 0.5f);
			if (visible)
			{
				RenderTerrainTile(xf, yf, xi + j, yi + i, lodf, lodi, EditFlag);
			}
			xf += lodf;
		}
		xf = temp;
		yf += lodf;
	}
}

void RenderTerrainFrustrum(bool EditFlag)
{
	int     xi;
	int     yi = FrustrumBoundMinY;
	float   xf;
	float   yf = (float)yi;

	TerrainBatchInit();
	TerrainBatchReset();

	for (; yi <= FrustrumBoundMaxY; yi += 4, yf += 4.f)
	{
		xi = FrustrumBoundMinX;
		xf = (float)xi;
		for (; xi <= FrustrumBoundMaxX; xi += 4, xf += 4.f)
		{
			bool visible = TestFrustrum2D(xf + 2.f, yf + 2.f, GetTerrainFrustumRange()) || CameraTopViewEnable;
			CullingDebugBlockTest(visible, xf + 2.f, yf + 2.f);
			if (visible)
			{
				if (gMapManager->currentMap == WD_73NEW_LOGIN_SCENE || gMapManager->currentMap == WD_77NEW_LOGIN_SCENE)
				{
					float fDistance_x = CameraPosition[0] - xf / 0.01f;
					float fDistance_y = CameraPosition[1] - yf / 0.01f;
					float fDistance = sqrtf(fDistance_x * fDistance_x + fDistance_y * fDistance_y);

					if (fDistance > 5200.f)
						continue;
				}
				RenderTerrainBlock(xf, yf, xi, yi, EditFlag);
			}
		}
	}

	if (g_terrainBatchEnabled)
		TerrainBatchFlush();

	CullingDebugDump();
}

void RenderTerrainBlock_After(float xf, float yf, int xi, int yi, bool EditFlag)
{
	int lodi = 1;
	float lodf = (float)lodi;
	for (int i = 0; i < 4; i += lodi)
	{
		float temp = xf;
		for (int j = 0; j < 4; j += lodi)
		{
			if (TestFrustrum2D(xf + 0.5f, yf + 0.5f, -8.f) || CameraTopViewEnable)
			{
				RenderTerrainTile_After(xf, yf, xi + j, yi + i, lodf, lodi, EditFlag);
			}
			xf += lodf;
		}
		xf = temp;
		yf += lodf;
	}
}

void RenderTerrainFrustrum_After(bool EditFlag)
{
	int   xi, yi;
	float xf, yf;
	yi = FrustrumBoundMinY;
	yf = (float)yi;
	for (; yi <= FrustrumBoundMaxY; yi += 4, yf += 4.f)
	{
		xi = FrustrumBoundMinX;
		xf = (float)xi;
		for (; xi <= FrustrumBoundMaxX; xi += 4, xf += 4.f)
		{
			if (TestFrustrum2D(xf + 2.f, yf + 2.f, -80.f) || CameraTopViewEnable)
			{
				RenderTerrainBlock_After(xf, yf, xi, yi, EditFlag);
			}
		}
	}
}

extern int SelectMapping;
extern void RenderCharactersClient();

void RenderTerrain(bool EditFlag)
{
	if (!EditFlag)
	{
		if (gMapManager->currentMap == WD_8TARKAN)
		{
			WaterMove = (float)((int)(WorldTime) % 40000) * 0.000025f;
		}
		else if (gMapManager->currentMap == WD_42CHANGEUP3RD_2ND)
		{
			int iWorldTime = (int)WorldTime;
			int iRemainder = iWorldTime % 50000;
			WaterMove = (float)iRemainder * 0.00002f;
		}
		else
		{
			WaterMove = (float)((int)(WorldTime) % 20000) * 0.00005f;
		}
	}

	if (!EditFlag)
	{
		DisableAlphaBlend();
	}
	else
	{
		SelectFlag = false;
		InitCollisionDetectLineToFace();
	}

	// [Tier1-B] grama renderizada inline durante a passada NORMAL.
	g_terrainGrassMerged = (!EditFlag && TerrainGrassEnable
		&& gMapManager->currentMap != WD_7ATLANSE && !IsDoppelGanger3());

	TerrainFlag = TERRAIN_MAP_NORMAL;
	RenderTerrainFrustrum(EditFlag);
	//  
	if (EditFlag && SelectFlag)
	{
		RenderTerrainTile(SelectXF, SelectYF, (int)SelectXF, (int)SelectYF, 1.f, 1, EditFlag);
	}
	if (!EditFlag)
	{
		if (TerrainGrassEnable && gMapManager->currentMap != WD_7ATLANSE && !IsDoppelGanger3())
			glAlphaFunc(GL_GREATER, 0.25f);   // reforco pos-terrain (mantido do fluxo antigo)
		DisableDepthTest();
		EnableCullFace();
		RenderPointers();
		EnableDepthTest();
	}
}

void RenderTerrain_After(bool EditFlag)
{
	if (gMapManager->currentMap != WD_39KANTURU_3RD)
		return;

	TerrainFlag = TERRAIN_MAP_NORMAL;
	RenderTerrainFrustrum_After(EditFlag);
}

OBJECT Sun;

void CreateSun()
{
	//Sun.Type = BITMAP_LIGHT;
	Sun.Scale = 8.f;
	Sun.AnimationFrame = 1.f;
}

void RenderSun()
{
	EnableAlphaBlend();
	vec3_t Angle;
	float Matrix[3][4];
	Angle[0] = 0.f;
	Angle[1] = 0.f;
	Angle[2] = CameraAngle[2];
	AngleIMatrix(Angle, Matrix);
	vec3_t p, Position;
	Vector(-900.f, CameraViewFar * 0.9f, 0.f, p);
	VectorRotate(p, Matrix, Position);
	VectorAdd(CameraPosition, Position, Sun.Position);
	Sun.Position[2] = 550.f;
	Sun.Visible = TestDepthBuffer(Sun.Position);
	BeginSprite();
	//RenderSprite(&Sun);
	EndSprite();
	DisableAlphaBlend();
}

void RenderSky()
{
	vec3_t Angle;
	float Matrix[3][4];
	Angle[0] = 0.f;
	Angle[1] = 0.f;
	Angle[2] = CameraAngle[2];
	AngleIMatrix(Angle, Matrix);
	float Aspect = (float)(WindowWidth) / (float)(WindowWidth);
	float Width = 1780.f * Aspect;

	BeginSprite();
	vec3_t p, Position;
	float Num = 20.f;
	vec3_t LightTable[21];

	for (int i = 0; i <= Num; i++)
	{
		Vector(((float)i - Num * 0.5f) * (Width / Num), CameraViewFar * 0.99f, 0.f, p);
		VectorRotate(p, Matrix, Position);
		VectorAdd(CameraPosition, Position, Position);
		RequestTerrainLight(Position[0], Position[1], LightTable[i]);
	}

	for (int i = 1; i <= (int)Num; i++)
	{
		if (LightTable[i][0] <= 0.f)
		{
			Vector(0.f, 0.f, 0.f, LightTable[i - 1]);
		}
	}

	for (int i = (int)Num - 1; i >= 0; i--)
	{
		if (LightTable[i][0] <= 0.f)
		{
			Vector(0.f, 0.f, 0.f, LightTable[i + 1]);
		}
	}
	for (float x = 0.f; x < Num; x += 1.f)
	{
		float UV[4][2];
		TEXCOORD(UV[0], (x) * (1.f / Num), 1.f);
		TEXCOORD(UV[1], (x + 1.f) * (1.f / Num), 1.f);
		TEXCOORD(UV[2], (x + 1.f) * (1.f / Num), 0.f);
		TEXCOORD(UV[3], (x) * (1.f / Num), 0.f);

		vec3_t Light[4];
		VectorCopy(LightTable[(int)x], Light[0]);
		VectorCopy(LightTable[(int)x + 1], Light[1]);
		//VectorCopy(LightTable[(int)x+1],Light[2]);
		//VectorCopy(LightTable[(int)x  ],Light[3]);
		Vector(1.f, 1.f, 1.f, Light[2]);
		Vector(1.f, 1.f, 1.f, Light[3]);

		Vector((x - Num * 0.5f + 0.5f) * (Width / Num), CameraViewFar * 0.9f, 0.f, p);
		VectorRotate(p, Matrix, Position);
		VectorAdd(CameraPosition, Position, Position);
		Position[2] = 400.f;
		//RenderSpriteUV(BITMAP_SKY,Position,Width/Num,Height,UV,Light);
	}
	EndSprite();
}


