// StdAfx.h — Precompiled Header
// FIX: MSVC 2022+ (v143) não exporta mais funções C de <cmath> para o
// namespace global quando compilando como C++. Incluímos <math.h> (header C)
// explicitamente para manter compatibilidade com código legado do MU.

#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

// ============================================================
// FIX CRÍTICO DE COMPILAÇÃO
// math.h (C) deve vir ANTES de qualquer include C++ que use
// fabsf, cosf, sinf, sqrtf, powf, etc.
// ============================================================
#include <math.h>

#include <windows.h>

// Headers C++ padrão
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>        // FIX: steady_clock.h e w_ObjectInfo.h usam std::chrono
#include <unordered_map> // FIX: steady_clock.h usa std::unordered_map

// OpenGL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================
// FIX: Forward declarations para evitar dependência circular.
// ZzzObject.h usa BMD na linha 14, mas ZzzBMD.h depende de ZzzObject.h.
// ============================================================
class BMD;

// ============================================================
// FIX: Ordem correta de includes do projeto.
// Cada header depende dos anteriores. Não altere a ordem.
// ============================================================

// 1. Defines primeiro — nenhuma dependência (MAX_CLASS, ITEM_AXE, etc.)
#include "_define.h"        // FIX: deve vir ANTES de _enum.h

// 2. Enums — depende de _define.h (MAX_CLASS, ITEM_AXE, MODEL_MONSTER_END)
#include "_enum.h"

// 3. Types — depende de _enum.h (eBuffState para BuffStateMap)
#include "_types.h"

// 4. Structs — depende de _types.h (vec3_t)
#include "_struct.h"

// 5. Object — depende de _struct.h (OBJECT, PART_t)
//    BMD já foi forward-declarado acima, então ZzzObject.h pode usá-lo
#include "ZzzObject.h"

// 6. BMD — depende de ZzzObject.h (OBJECT)
#include "ZzzBMD.h"

// 7. Character — depende de _struct.h (Script_Skill) e ZzzObject.h (OBJECT)
#include "ZzzCharacter.h"

// Mantenha seus includes adicionais do projeto abaixo:
// #include "BaseCls.h"
// #include "muConsoleDebug.h"