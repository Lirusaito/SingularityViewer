/** 
 * @file llglslshader.cpp
 * @brief GLSL helper functions and state.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llglslshader.h"

#include "llshadermgr.h"
#include "llfile.h"
#include "llrender.h"
#include "llcontrol.h"
#include "llvertexbuffer.h"

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

// Lots of STL stuff in here, using namespace std to keep things more readable
using std::vector;
using std::pair;
using std::make_pair;
using std::string;

GLhandleARB LLGLSLShader::sCurBoundShader = 0;
LLGLSLShader* LLGLSLShader::sCurBoundShaderPtr = NULL;
S32 LLGLSLShader::sIndexedTextureChannels = 0;
bool LLGLSLShader::sNoFixedFunction = false;

//UI shader -- declared here so llui_libtest will link properly
//Singu note: Not using llui_libtest... and LLViewerShaderMgr is a part of newview. So, 
// these are declared in newview/llviewershadermanager.cpp just like every other shader.
//LLGLSLShader	gUIProgram(LLViewerShaderMgr::SHADER_INTERFACE);
//LLGLSLShader	gSolidColorProgram(LLViewerShaderMgr::SHADER_INTERFACE);

BOOL shouldChange(const LLVector4& v1, const LLVector4& v2)
{
	return v1 != v2;
}

LLShaderFeatures::LLShaderFeatures()
	: atmosphericHelpers(false)
	, calculatesLighting(false)
	, calculatesAtmospherics(false)
	, hasLighting(false)
	, isAlphaLighting(false)
	, isShiny(false)
	, isFullbright(false)
	, isSpecular(false)
	, hasWaterFog(false)
	, hasTransport(false)
	, hasSkinning(false)
	, hasObjectSkinning(false)
	, hasAtmospherics(false)
	, hasGamma(false)
	, mIndexedTextureChannels(0)
	, disableTextureIndex(false)
	, hasAlphaMask(false)
{
}

//===============================
// LLGLSL Shader implementation
//===============================
LLGLSLShader::LLGLSLShader(S32 shader_class)
	: mProgramObject(0),
	  mShaderClass(shader_class),
	  mAttributeMask(0),
	  mTotalUniformSize(0),
	  mActiveTextureChannels(0),
	  mShaderLevel(0),
	  mShaderGroup(SG_DEFAULT),
	  mUniformsDirty(FALSE)
{
	LLShaderMgr::getGlobalShaderList().push_back(this);
}

LLGLSLShader::~LLGLSLShader()
{
	
}
void LLGLSLShader::unload()
{
	stop_glerror();
	mAttribute.clear();
	mTexture.clear();
	mUniform.clear();
	mShaderFiles.clear();
	mDefines.clear();

	if (mProgramObject)
	{
		//Don't do this! Attached objects are already flagged for deletion.
		//They will be deleted when no programs have them attached. (deleting a program auto-detaches them!)
		/*GLhandleARB obj[1024];
		GLsizei count;

		glGetAttachedObjectsARB(mProgramObject, 1024, &count, obj);
		for (GLsizei i = 0; i < count; i++) 
		{
			glDeleteObjectARB(obj[i]);
		}*/
		if(mProgramObject)
			glDeleteObjectARB(mProgramObject);
		mProgramObject = 0;
	}
	
	//hack to make apple not complain
	glGetError();
	
	stop_glerror();
}

BOOL LLGLSLShader::createShader(std::vector<LLStaticHashedString> * attributes,
								std::vector<LLStaticHashedString> * uniforms,
								U32 varying_count,
								const char** varyings)
{
	//reloading, reset matrix hash values
	for (U32 i = 0; i < LLRender::NUM_MATRIX_MODES; ++i)
	{
		mMatHash[i] = 0xFFFFFFFF;
	}
	mLightHash = 0xFFFFFFFF;

	llassert_always(!mShaderFiles.empty());
	BOOL success = TRUE;

	if(mProgramObject)	//purge the old program
		glDeleteObjectARB(mProgramObject);
	// Create program
	mProgramObject = glCreateProgramObjectARB();
	
#if LL_DARWIN
    // work-around missing mix(vec3,vec3,bvec3)
    mDefines["OLD_SELECT"] = "1";
#endif
	
	//compile new source
	vector< pair<string,GLenum> >::iterator fileIter = mShaderFiles.begin();
	for ( ; fileIter != mShaderFiles.end(); fileIter++ )
	{
		GLhandleARB shaderhandle = LLShaderMgr::instance()->loadShaderFile((*fileIter).first, mShaderLevel, (*fileIter).second, &mDefines, mFeatures.mIndexedTextureChannels);
		LL_DEBUGS("ShaderLoading") << "SHADER FILE: " << (*fileIter).first << " mShaderLevel=" << mShaderLevel << LL_ENDL;
		if (shaderhandle > 0)
		{
			attachObject(shaderhandle);
		}
		else
		{
			success = FALSE;
		}
	}

	// Attach existing objects
	if (!LLShaderMgr::instance()->attachShaderFeatures(this))
	{
		if(mProgramObject)
			glDeleteObjectARB(mProgramObject);
		mProgramObject = 0;
		return FALSE;
	}

	static const LLCachedControl<bool> no_texture_indexing("ShyotlUseLegacyTextureBatching",false);
 	if ((gGLManager.mGLSLVersionMajor < 2 && gGLManager.mGLSLVersionMinor < 3) || no_texture_indexing)
 	{ //attachShaderFeatures may have set the number of indexed texture channels, so set to 1 again
		mFeatures.mIndexedTextureChannels = llmin(mFeatures.mIndexedTextureChannels, 1);
	}

#ifdef GL_INTERLEAVED_ATTRIBS
	if (varying_count > 0 && varyings)
	{
		glTransformFeedbackVaryings(mProgramObject, varying_count, varyings, GL_INTERLEAVED_ATTRIBS);
	}
#endif

	// Map attributes and uniforms
	if (success)
	{
		success = mapAttributes(attributes);
	}
	if (success)
	{
		success = mapUniforms(uniforms);
	}
	if( !success )
	{
		if(mProgramObject)
			glDeleteObjectARB(mProgramObject);
		mProgramObject = 0;

		LL_WARNS("ShaderLoading") << "Failed to link shader: " << mName << LL_ENDL;

		// Try again using a lower shader level;
		if (mShaderLevel > 0)
		{
			LL_WARNS("ShaderLoading") << "Failed to link using shader level " << mShaderLevel << " trying again using shader level " << (mShaderLevel - 1) << LL_ENDL;
			mShaderLevel--;
			return createShader(attributes,uniforms);
		}
	}
	else if (mFeatures.mIndexedTextureChannels > 0)
	{ //override texture channels for indexed texture rendering
		bind();
		S32 channel_count = mFeatures.mIndexedTextureChannels;

		for (S32 i = 0; i < channel_count; i++)
		{
			LLStaticHashedString uniName(llformat("tex%d", i));
			uniform1i(uniName, i);
		}

		S32 cur_tex = channel_count; //adjust any texture channels that might have been overwritten
		for (U32 i = 0; i < mTexture.size(); i++)
		{
			if (mTexture[i] > -1 && mTexture[i] < channel_count)
			{
				llassert(cur_tex < gGLManager.mNumTextureImageUnits);
				uniform1i(i, cur_tex);
				mTexture[i] = cur_tex++;
			}
		}
		unbind();
	}

	return success;
}

BOOL LLGLSLShader::attachObject(std::string object)
{
	std::multimap<std::string, LLShaderMgr::CachedObjectInfo>::iterator it = LLShaderMgr::instance()->mShaderObjects.begin();
	for(; it!=LLShaderMgr::instance()->mShaderObjects.end(); it++)
	{
		if((*it).first == object)
		{
			glAttachObjectARB(mProgramObject, (*it).second.mHandle);
			stop_glerror();
			return TRUE;
		}
	}
	{
		LL_WARNS("ShaderLoading") << "Attempting to attach shader object that hasn't been compiled: " << object << LL_ENDL;
		return FALSE;
	}
}

void LLGLSLShader::attachObject(GLhandleARB object)
{
	if (object != 0)
	{
		std::multimap<std::string, LLShaderMgr::CachedObjectInfo>::iterator it = LLShaderMgr::instance()->mShaderObjects.begin();
		for(; it!=LLShaderMgr::instance()->mShaderObjects.end(); it++)
		{
			if((*it).second.mHandle == object)
			{
				LL_INFOS("ShaderLoading") << "Attached: " << (*it).first << LL_ENDL;
				break;
			}
		}
		if(it == LLShaderMgr::instance()->mShaderObjects.end())
		{
			LL_WARNS("ShaderLoading") << "Attached unknown shader!" << LL_ENDL;
		}
		
		stop_glerror();
		glAttachObjectARB(mProgramObject, object);
		stop_glerror();
	}
	else
	{
		LL_WARNS("ShaderLoading") << "Attempting to attach non existing shader object. " << LL_ENDL;
	}
}

void LLGLSLShader::attachObjects(GLhandleARB* objects, S32 count)
{
	for (S32 i = 0; i < count; i++)
	{
		attachObject(objects[i]);
	}
}

BOOL LLGLSLShader::mapAttributes(const std::vector<LLStaticHashedString> * attributes)
{
	//before linking, make sure reserved attributes always have consistent locations
	for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
	{
		const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
		glBindAttribLocationARB(mProgramObject, i, (const GLcharARB *) name);
	}
	
	//link the program
	BOOL res = link();

	mAttribute.clear();
	U32 numAttributes = (attributes == NULL) ? 0 : attributes->size();
	mAttribute.resize(LLShaderMgr::instance()->mReservedAttribs.size() + numAttributes, -1);
	
	if (res)
	{ //read back channel locations

		mAttributeMask = 0;

		//read back reserved channels first
		for (U32 i = 0; i < (S32) LLShaderMgr::instance()->mReservedAttribs.size(); i++)
		{
			const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
			S32 index = glGetAttribLocationARB(mProgramObject, (const GLcharARB *)name);
			if (index != -1)
			{
				mAttribute[i] = index;
				mAttributeMask |= 1 << i;
				LL_DEBUGS("ShaderLoading") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
			}
		}
		if (attributes != NULL)
		{
			for (U32 i = 0; i < numAttributes; i++)
			{
				const char* name = (*attributes)[i].String().c_str();
				S32 index = glGetAttribLocationARB(mProgramObject, name);
				if (index != -1)
				{
					mAttribute[LLShaderMgr::instance()->mReservedAttribs.size() + i] = index;
					LL_DEBUGS("ShaderLoading") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
				}
			}
		}

		return TRUE;
	}
	
	return FALSE;
}

void LLGLSLShader::mapUniform(GLint index, const vector<LLStaticHashedString> * uniforms)
{
	if (index == -1)
	{
		return;
	}

	GLenum type;
	GLsizei length;
	GLint size = -1;
	char name[1024];		/* Flawfinder: ignore */
	name[0] = 0;

	glGetActiveUniformARB(mProgramObject, index, 1024, &length, &size, &type, (GLcharARB *)name);
#if !LL_DARWIN
	if (size > 0)
	{
		switch(type)
		{
			case GL_FLOAT_VEC2: size *= 2; break;
			case GL_FLOAT_VEC3: size *= 3; break;
			case GL_FLOAT_VEC4: size *= 4; break;
			case GL_DOUBLE: size *= 2; break;
			case GL_DOUBLE_VEC2: size *= 2; break;
			case GL_DOUBLE_VEC3: size *= 6; break;
			case GL_DOUBLE_VEC4: size *= 8; break;
			case GL_INT_VEC2: size *= 2; break;
			case GL_INT_VEC3: size *= 3; break;
			case GL_INT_VEC4: size *= 4; break;
			case GL_UNSIGNED_INT_VEC2: size *= 2; break;
			case GL_UNSIGNED_INT_VEC3: size *= 3; break;
			case GL_UNSIGNED_INT_VEC4: size *= 4; break;
			case GL_BOOL_VEC2: size *= 2; break;
			case GL_BOOL_VEC3: size *= 3; break;
			case GL_BOOL_VEC4: size *= 4; break;
			case GL_FLOAT_MAT2: size *= 4; break;
			case GL_FLOAT_MAT3: size *= 9; break;
			case GL_FLOAT_MAT4: size *= 16; break;
			case GL_FLOAT_MAT2x3: size *= 6; break;
			case GL_FLOAT_MAT2x4: size *= 8; break;
			case GL_FLOAT_MAT3x2: size *= 6; break;
			case GL_FLOAT_MAT3x4: size *= 12; break;
			case GL_FLOAT_MAT4x2: size *= 8; break;
			case GL_FLOAT_MAT4x3: size *= 12; break;
			case GL_DOUBLE_MAT2: size *= 8; break;
			case GL_DOUBLE_MAT3: size *= 18; break;
			case GL_DOUBLE_MAT4: size *= 32; break;
			case GL_DOUBLE_MAT2x3: size *= 12; break;
			case GL_DOUBLE_MAT2x4: size *= 16; break;
			case GL_DOUBLE_MAT3x2: size *= 12; break;
			case GL_DOUBLE_MAT3x4: size *= 24; break;
			case GL_DOUBLE_MAT4x2: size *= 16; break;
			case GL_DOUBLE_MAT4x3: size *= 24; break;
		}
		mTotalUniformSize += size;
	}
#endif

	S32 location = glGetUniformLocationARB(mProgramObject, name);
	if (location != -1)
	{
		//chop off "[0]" so we can always access the first element
		//of an array by the array name
		char* is_array = strstr(name, "[0]");
		if (is_array)
		{
			is_array[0] = 0;
		}

		LLStaticHashedString hashedName(name);
		mUniformMap[hashedName] = location;

		LL_DEBUGS("ShaderLoading") << "Uniform " << name << " is at location " << location << LL_ENDL;
	
		//find the index of this uniform
		for (S32 i = 0; i < (S32) LLShaderMgr::instance()->mReservedUniforms.size(); i++)
		{
			if ( (mUniform[i] == -1)
				&& (LLShaderMgr::instance()->mReservedUniforms[i] == name))
			{
				//found it
				mUniform[i] = location;
				mTexture[i] = mapUniformTextureChannel(location, type);
				return;
			}
		}

		if (uniforms != NULL)
		{
			for (U32 i = 0; i < uniforms->size(); i++)
			{
				if ( (mUniform[i+LLShaderMgr::instance()->mReservedUniforms.size()] == -1)
					&& ((*uniforms)[i].String() == name))
				{
					//found it
					mUniform[i+LLShaderMgr::instance()->mReservedUniforms.size()] = location;
					mTexture[i+LLShaderMgr::instance()->mReservedUniforms.size()] = mapUniformTextureChannel(location, type);
					return;
				}
			}
		}
	}
}

void LLGLSLShader::addPermutation(std::string name, std::string value)
{
	mDefines[name] = value;
}

void LLGLSLShader::removePermutation(std::string name)
{
	mDefines[name].erase();
}

GLint LLGLSLShader::mapUniformTextureChannel(GLint location, GLenum type)
{
	if (type >= GL_SAMPLER_1D_ARB && type <= GL_SAMPLER_2D_RECT_SHADOW_ARB /*||
		type == GL_SAMPLER_2D_MULTISAMPLE*/)
	{	//this here is a texture
		glUniform1iARB(location, mActiveTextureChannels);
		LL_DEBUGS("ShaderLoading") << "Assigned to texture channel " << mActiveTextureChannels << LL_ENDL;
		return mActiveTextureChannels++;
	}
	return -1;
}

BOOL LLGLSLShader::mapUniforms(const vector<LLStaticHashedString> * uniforms)
{
	BOOL res = TRUE;
	
	mTotalUniformSize = 0;
	mActiveTextureChannels = 0;
	mUniform.clear();
	mUniformMap.clear();
	mTexture.clear();
	mValueVec4.clear();
	mValueMat3.clear();
	mValueMat4.clear();
	//initialize arrays
	U32 numUniforms = (uniforms == NULL) ? 0 : uniforms->size();
	mUniform.resize(numUniforms + LLShaderMgr::instance()->mReservedUniforms.size(), -1);
	mTexture.resize(numUniforms + LLShaderMgr::instance()->mReservedUniforms.size(), -1);
	
	bind();

	//get the number of active uniforms
	GLint activeCount;
	glGetObjectParameterivARB(mProgramObject, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &activeCount);

	for (S32 i = 0; i < activeCount; i++)
	{
		mapUniform(i, uniforms);
	}

	unbind();

	LL_DEBUGS("ShaderLoading") << "Total Uniform Size: " << mTotalUniformSize << LL_ENDL;
	return res;
}

BOOL LLGLSLShader::link(BOOL suppress_errors)
{
	return LLShaderMgr::instance()->linkProgramObject(mProgramObject, suppress_errors);
}

void LLGLSLShader::bind()
{
	gGL.flush();
	if (gGLManager.mHasShaderObjects)
	{
		LLVertexBuffer::unbind();
		glUseProgramObjectARB(mProgramObject);
		sCurBoundShader = mProgramObject;
		sCurBoundShaderPtr = this;
		if (mUniformsDirty)
		{
			LLShaderMgr::instance()->updateShaderUniforms(this);
			mUniformsDirty = FALSE;
		}
	}
}

void LLGLSLShader::unbind()
{
	gGL.flush();
	if (gGLManager.mHasShaderObjects)
	{
		stop_glerror();
		if (gGLManager.mIsNVIDIA)
		{
			for (U32 i = 0; i < mAttribute.size(); ++i)
			{
				vertexAttrib4f(i, 0,0,0,1);
				stop_glerror();
			}
		}
		LLVertexBuffer::unbind();
		glUseProgramObjectARB(0);
		sCurBoundShader = 0;
		sCurBoundShaderPtr = NULL;
		stop_glerror();
	}
}

void LLGLSLShader::bindNoShader(void)
{
	LLVertexBuffer::unbind();
	if (gGLManager.mHasShaderObjects)
	{
		glUseProgramObjectARB(0);
		sCurBoundShader = 0;
		sCurBoundShaderPtr = NULL;
	}
}

S32 LLGLSLShader::bindTexture(const std::string &uniform, LLTexture *texture, LLTexUnit::eTextureType mode)
{
	S32 channel = 0;
	channel = getUniformLocation(uniform);
	
	return bindTexture(channel, texture, mode);
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLTexture *texture, LLTexUnit::eTextureType mode)
{
	if (uniform < 0 || uniform >= (S32)mTexture.size())
	{
		UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
		return -1;
	}
	
	uniform = mTexture[uniform];
	
	if (uniform > -1)
	{
		gGL.getTexUnit(uniform)->bind(texture, mode);
	}
	
	return uniform;
}

S32 LLGLSLShader::unbindTexture(const std::string &uniform, LLTexUnit::eTextureType mode)
{
	S32 channel = 0;
	channel = getUniformLocation(uniform);
	
	return unbindTexture(channel);
}

S32 LLGLSLShader::unbindTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
	if (uniform < 0 || uniform >= (S32)mTexture.size())
	{
		UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
		return -1;
	}
	
	uniform = mTexture[uniform];
	
	if (uniform > -1)
	{
		gGL.getTexUnit(uniform)->unbind(mode);
	}
	
	return uniform;
}

S32 LLGLSLShader::enableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
	if (uniform < 0 || uniform >= (S32)mTexture.size())
	{
		UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
		return -1;
	}
	S32 index = mTexture[uniform];
	if (index != -1)
	{
		gGL.getTexUnit(index)->activate();
		gGL.getTexUnit(index)->enable(mode);
	}
	return index;
}

S32 LLGLSLShader::disableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
	if (uniform < 0 || uniform >= (S32)mTexture.size())
	{
		UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
		return -1;
	}
	S32 index = mTexture[uniform];
	if (index != -1 && gGL.getTexUnit(index)->getCurrType() != LLTexUnit::TT_NONE)
	{
		if (gDebugGL && gGL.getTexUnit(index)->getCurrType() != mode)
		{
			if (gDebugSession)
			{
				gFailLog << "Texture channel " << index << " texture type corrupted." << std::endl;
				ll_fail("LLGLSLShader::disableTexture failed");
			}
			else
			{
				LL_ERRS() << "Texture channel " << index << " texture type corrupted." << LL_ENDL;
			}
		}
		gGL.getTexUnit(index)->disable();
	}
	return index;
}

GLint LLGLSLShader::getUniformLocation(U32 index)
{
	GLint ret = -1;
	if (mProgramObject > 0)
	{
		llassert(index < mUniform.size());
		return mUniform[index];
	}

	return ret;
}

GLint LLGLSLShader::getAttribLocation(U32 attrib)
{
	if (attrib < mAttribute.size())
	{
		return mAttribute[attrib];
	}
	else
	{
		return -1;
	}
}

void LLGLSLShader::vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	if (mAttribute[index] > 0)
	{
		glVertexAttrib4fARB(mAttribute[index], x, y, z, w);
	}
}

void LLGLSLShader::setMinimumAlpha(F32 minimum)
{
	gGL.flush();
	uniform1f(LLShaderMgr::MINIMUM_ALPHA, minimum);
}
