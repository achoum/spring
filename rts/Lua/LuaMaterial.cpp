#include "StdAfx.h"
// LuaMaterial.cpp: implementation of the CLuaMaterial class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaMaterial.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaHashString.h"
#include "LuaHandle.h"
#include "LuaOpenGL.h"

#include "Game/Camera.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/Unit.h"
#include "System/LogOutput.h"


LuaMatHandler LuaMatHandler::handler;
LuaMatHandler& luaMatHandler = LuaMatHandler::handler;


#define PRINTF logOutput.Print

#define STRING_CASE(ptr, x) case x: ptr = #x; break;


/******************************************************************************/
/******************************************************************************/
//
//  LuaUnitUniforms
//

void LuaUnitUniforms::Execute(CUnit* unit) const
{
	if (!haveUniforms) {
		return;
	}
	if (speedLoc != 0) {
		glUniform3f(speedLoc, unit->speed.x, unit->speed.y, unit->speed.z);
	}
	if (healthLoc != 0) {
		glUniform1f(healthLoc, unit->health / unit->maxHealth);
	}
	if (unitIDLoc != 0) {
		glUniform1i(unitIDLoc, unit->id);
	}
	if (teamIDLoc != 0) {
		glUniform1i(teamIDLoc, unit->id);
	}
	if (customLoc != 0) {
		if (customCount > 0) {
			glUniform1fv(customLoc, customCount, customData);
		}
	}
}


void LuaUnitUniforms::SetCustomCount(int count)
{
	customCount = count;
	delete[] customData;
	if (count > 0) {
		customData = SAFE_NEW float[count];
		memset(customData, 0, customCount * sizeof(GLfloat));
	} else {
		customData = NULL;
	}
}


LuaUnitUniforms& LuaUnitUniforms::operator=(const LuaUnitUniforms& u)
{
	delete[] customData;
	customData = NULL;
	
	haveUniforms = u.haveUniforms;
	speedLoc     = u.speedLoc;	
	healthLoc    = u.healthLoc;	
	unitIDLoc    = u.unitIDLoc;	
	teamIDLoc    = u.teamIDLoc;	
	customLoc    = u.customLoc;
	customCount  = u.customCount;

	if (customCount > 0) {
		customData = SAFE_NEW GLfloat[customCount];
		memcpy(customData, u.customData, customCount * sizeof(GLfloat));
	}

	return *this;
}


LuaUnitUniforms::LuaUnitUniforms(const LuaUnitUniforms& u)
{
	customData = NULL;
	*this = u;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaUnitMaterial
//

bool LuaUnitMaterial::SetLODCount(unsigned int count)
{
	lodCount = count;
	lastLOD = lodCount - 1;
	lodMats.resize(count);
	return true;
}


bool LuaUnitMaterial::SetLastLOD(unsigned int lod)
{
	lastLOD = min(lod, lodCount - 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatShader
//

void LuaMatShader::Finalize()
{
	if (type != LUASHADER_GL) {
		openglID = 0;
	}
}


int LuaMatShader::Compare(const LuaMatShader& a, const LuaMatShader& b)
{
	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;
	}
	if (a.type == LUASHADER_GL) {
		if (a.openglID != b.openglID) {
			return (a.openglID < b.openglID) ? -1 : +1;
		}
	}
	return 0; // LUASHADER_NONE and LUASHADER_SHADOWMAP ignore openglID
}


void LuaMatShader::Execute(const LuaMatShader& prev) const
{
	if (type != prev.type) {
		if (prev.type == LUASHADER_GL) {
			glUseProgram(0);
		}
		else if (prev.type == LUASHADER_3DO) {
			if (luaMatHandler.reset3doShader) { luaMatHandler.reset3doShader(); }
		}
		else if (prev.type == LUASHADER_S3O) {
			if (luaMatHandler.resetS3oShader) { luaMatHandler.resetS3oShader(); }
		}

		if (type == LUASHADER_GL) {
			glUseProgram(openglID);
		}
		else if (type == LUASHADER_3DO) {
			if (luaMatHandler.setup3doShader) { luaMatHandler.setup3doShader(); }
		}
		else if (type == LUASHADER_S3O) {
			if (luaMatHandler.setupS3oShader) { luaMatHandler.setupS3oShader(); }
		}
	}
	else if (type == LUASHADER_GL) {
		if (openglID != prev.openglID) {
			glUseProgram(openglID);
		}
	}
}


void LuaMatShader::Print(const string& indent) const
{
	const char* typeName = "Unknown";
	switch (type) {
		STRING_CASE(typeName, LUASHADER_NONE);
		STRING_CASE(typeName, LUASHADER_GL);
		STRING_CASE(typeName, LUASHADER_3DO);
		STRING_CASE(typeName, LUASHADER_S3O);
	}
	PRINTF("%s%s %i\n", indent.c_str(), typeName, openglID);
}

/******************************************************************************/
/******************************************************************************/
//
//  LuaMatTexture
//

void LuaMatTexture::Finalize()
{
	int maxTex = 0;
	for (int t = 0; t < maxTexUnits; t++) {
		if ((type == LUATEX_GL) && (openglID == 0)) {
			type = LUATEX_NONE;
		}
		if (type == LUATEX_NONE) {
			enable = false;
		}
		if (type != LUATEX_GL) {
			openglID = 0;
		}
	}
}


int LuaMatTexture::Compare(const LuaMatTexture& a, const LuaMatTexture& b)
{
	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;
	}
	if (a.type == LUATEX_GL) {
		if (a.openglID != b.openglID) {
			return (a.openglID < b.openglID) ? -1 : +1;
		}
	}
	if (a.enable != b.enable) {
		return a.enable ? -1 : +1;
	}
	return 0; // LUATEX_NONE and LUATEX_SHADOWMAP ignore openglID
}


void LuaMatTexture::Execute(const LuaMatTexture& prev) const
{
	// blunt force -- poor form
	if (prev.enable) {
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	}

	if (type == LUATEX_GL) {
		glBindTexture(GL_TEXTURE_2D, openglID);
		if (enable) {
			glEnable(GL_TEXTURE_2D);
		}
	}
	else if (type == LUATEX_SHADOWMAP) {
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		if (enable) {
			glEnable(GL_TEXTURE_2D);
		}
	}
	else if (type == LUATEX_REFLECTION) {
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, unitDrawer->boxtex);
		if (enable) {
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		}
	}
	else if (type == LUATEX_SPECULAR) {
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, unitDrawer->specularTex);
		if (enable) {
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		}
	}  
}


void LuaMatTexture::Print(const string& indent) const
{
	const char* typeName = "Unknown";
	switch (type) {
		STRING_CASE(typeName, LUATEX_NONE);
		STRING_CASE(typeName, LUATEX_GL);
		STRING_CASE(typeName, LUATEX_SHADOWMAP);
		STRING_CASE(typeName, LUATEX_REFLECTION);
		STRING_CASE(typeName, LUATEX_SPECULAR);
	}
	PRINTF("%s%s %i %s\n", indent.c_str(),
	       typeName, openglID, enable ? "true" : "false");
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMaterial
//

const LuaMaterial LuaMaterial::defMat;


void LuaMaterial::Finalize()
{
	shader.Finalize();

	texCount = 0;
	for (int t = 0; t < LuaMatTexture::maxTexUnits; t++) {
		LuaMatTexture& tex = textures[t];
		tex.Finalize();
		if (tex.type != LuaMatTexture::LUATEX_NONE) {
			texCount = (t + 1);
		}
	}
}


void LuaMaterial::Execute(const LuaMaterial& prev) const
{
	if (prev.postList != 0) {
		glCallList(prev.postList);
	}
	if (preList != 0) {
		glCallList(preList);
	}

	shader.Execute(prev.shader);

	if (cameraLoc) {
		// FIXME: this is happening too much, just use floats?
		GLfloat array[16];
		for (int i = 0; i < 16; i++) {
			array[i] = (GLfloat)camera->modelview[i];
		}
		glUniformMatrix4fv(cameraLoc, 1, GL_FALSE, array);
	}

	const int maxTex = max(texCount, prev.texCount);
	for (int t = 0; t < maxTex; t++) {
		glActiveTexture(GL_TEXTURE0 + t);
		textures[t].Execute(prev.textures[t]);
	}
	glActiveTexture(GL_TEXTURE0);

	if (useCamera != prev.useCamera) {
		if (useCamera) {
			glPopMatrix();
		} else {
			glPushMatrix();
			glLoadIdentity();
		}
	}
}


int LuaMaterial::Compare(const LuaMaterial& a, const LuaMaterial& b)
{
	// NOTE: the order of the comparisons is important
	int cmp;

	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;  // should not happen
	}
	
	if (a.order != b.order) {
		return (a.order < b.order) ? -1 : +1;
	}

	cmp = LuaMatShader::Compare(a.shader, b.shader);
	if (cmp != 0) {
		return cmp;
	}

	const int maxTex = min(a.texCount, b.texCount);
	for (int t = 0; t < maxTex; t++) {
		cmp = LuaMatTexture::Compare(a.textures[t], b.textures[t]);
		if (cmp != 0) {
			return cmp;
		}
	}
	if (a.texCount != b.texCount) {
		return (a.texCount < b.texCount) ? -1 : + 1;
	}

	if (a.preList != b.preList) {
		return (a.preList < b.preList) ? -1 : +1;
	}

	if (a.postList != b.postList) {
		return (a.postList < b.postList) ? -1 : +1;
	}

	if (a.useCamera != b.useCamera) {
		return a.useCamera ? -1 : +1;
	}

	if (a.cameraLoc != b.cameraLoc) {
		return (a.cameraLoc < a.cameraLoc) ? -1 : +1;
	}

	return 0;
}


static const char* GetMatTypeName(LuaMatType type)
{
	const char* typeName = "Unknown";
	switch (type) {
		STRING_CASE(typeName, LUAMAT_ALPHA);
		STRING_CASE(typeName, LUAMAT_OPAQUE);
		STRING_CASE(typeName, LUAMAT_SHADOW);
	}
	return typeName;
}


void LuaMaterial::Print(const string& indent) const
{
	PRINTF("%s%s\n", indent.c_str(), GetMatTypeName(type));
	PRINTF("%sorder = %i\n", indent.c_str(), order);
	shader.Print(indent);
	PRINTF("%stexCount = %i\n", indent.c_str(), texCount);
	for (int t = 0; t < texCount; t++) {
		char buf[32];
		SNPRINTF(buf, sizeof(buf), "%s  tex[%i] ", indent.c_str(), t);
		textures[t].Print(buf);
	}
	PRINTF("%spreList  = %i\n",  indent.c_str(), preList);
	PRINTF("%spostList = %i\n",  indent.c_str(), postList);
	PRINTF("%suseCamera = %s\n", indent.c_str(), useCamera ? "true" : "false");
	PRINTF("%scameraLoc = %i\n", indent.c_str(), cameraLoc);
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatRef
//

LuaMatRef::LuaMatRef(LuaMatBin* _bin)
{
	bin = _bin;
	if (bin != NULL) {
		bin->Ref();
	}
}


LuaMatRef::~LuaMatRef()
{
	if (bin != NULL) {
		bin->UnRef();
	}
}


void LuaMatRef::Reset()
{
	if (bin != NULL) {
		bin->UnRef();
	}
	bin = NULL;
}


LuaMatRef& LuaMatRef::operator=(const LuaMatRef& mr)
{
	if (bin != mr.bin) {
		if (bin != NULL) { bin->UnRef(); }
		bin = mr.bin;
		if (bin != NULL) { bin->Ref(); }
	}
	return *this;
}


LuaMatRef::LuaMatRef(const LuaMatRef& mr)
{
	bin = mr.bin;
	if (bin != NULL) {
		bin->Ref();
	}
}


void LuaMatRef::AddUnit(CUnit* unit)
{
  if (bin != NULL) {
    bin->AddUnit(unit);
  }
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatBin
//

void LuaMatBin::Ref()
{
	refCount++;
}


void LuaMatBin::UnRef()
{
	refCount--;
	if (refCount <= 0) {
		luaMatHandler.FreeBin(this);
	}
}


void LuaMatBin::Print(const string& indent) const
{
	PRINTF("%sunitCount = %i\n", indent.c_str(), (int)units.size());
	PRINTF("%spointer = %p\n", indent.c_str(), this);
	LuaMaterial::Print(indent + "  ");
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatHandler
//

LuaMatHandler::LuaMatHandler()
{
	setup3doShader = NULL;
	reset3doShader = NULL;
	setupS3oShader = NULL;
	resetS3oShader = NULL;
}


LuaMatHandler::~LuaMatHandler()
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		LuaMatBinSet& binSet = binTypes[LuaMatType(m)];
		LuaMatBinSet::iterator it;
		for (it = binSet.begin(); it != binSet.end(); ++it) {
			delete *it;
		}
	}
}


LuaMatRef LuaMatHandler::GetRef(const LuaMaterial& mat)
{
	if ((mat.type < 0) || (mat.type >= LUAMAT_TYPE_COUNT)) {
		logOutput.Print("ERROR: LuaMatHandler::GetRef() untyped material\n");
		return LuaMatRef();
	}
	LuaMatBinSet& binSet = binTypes[mat.type];

	LuaMatBin* fakeBin = (LuaMatBin*) &mat;
	LuaMatBinSet::iterator it = binSet.find(fakeBin);
	if (it != binSet.end()) {
		return LuaMatRef(*it);
	}

	LuaMatBin* bin = SAFE_NEW LuaMatBin(mat);
	binSet.insert(bin);
	return LuaMatRef(bin);
}


void LuaMatHandler::ClearBins(LuaMatType type)
{
	if ((type < 0) || (type >= LUAMAT_TYPE_COUNT)) {
		return;
	}
	LuaMatBinSet& binSet = binTypes[type];
	LuaMatBinSet::iterator it;
	for (it = binSet.begin(); it != binSet.end(); ++it) {
		LuaMatBin* bin = *it;
		bin->Clear();
	}	
}


void LuaMatHandler::ClearBins()
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		ClearBins(LuaMatType(m));
	}
}


void LuaMatHandler::FreeBin(LuaMatBin* bin)
{
	LuaMatBinSet& binSet = binTypes[bin->type];
	LuaMatBinSet::iterator it = binSet.find(bin);
	if (it != binSet.end()) {
		if (*it != bin) {
			logOutput.Print("ERROR: LuaMatHandler::FreeBin pointer mismatch\n");
		}
		delete bin;
		binSet.erase(it);
	}
}


void LuaMatHandler::PrintBins(const string& indent, LuaMatType type) const
{
	if ((type < 0) || (type >= LUAMAT_TYPE_COUNT)) {
		return;
	}
	const LuaMatBinSet& binSet = binTypes[type];
	LuaMatBinSet::const_iterator it;
	int num = 0;
	PRINTF("%sBINCOUNT = %i\n", indent.c_str(), binSet.size());
	for (it = binSet.begin(); it != binSet.end(); ++it) {
		LuaMatBin* bin = *it;
		PRINTF("%sBIN %i:\n", indent.c_str(), num);
		bin->Print(indent + "    ");
		num++;
	}
}


void LuaMatHandler::PrintAllBins(const string& indent) const
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		string newIndent = indent + GetMatTypeName(LuaMatType(m));
		newIndent += "  ";
		PrintBins(newIndent, LuaMatType(m));
	}
}


/******************************************************************************/
/******************************************************************************/