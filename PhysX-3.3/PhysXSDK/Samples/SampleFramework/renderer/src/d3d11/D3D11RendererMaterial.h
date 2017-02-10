/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#ifndef D3D11_RENDERER_MATERIAL_H
#define D3D11_RENDERER_MATERIAL_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <RendererMaterial.h>

#include "D3D11Renderer.h"
#include "D3D11RendererTraits.h"
#include "D3Dcompiler.h"

namespace SampleRenderer
{

class D3D11Renderer;

class D3D11RendererMaterial : public RendererMaterial
{
	friend class D3D11Renderer;
	friend class D3D11RendererMesh;
	friend class D3D11RendererVariableManager;

	typedef RendererMaterial::Variable Variable;

public:
	D3D11RendererMaterial(D3D11Renderer& renderer, const RendererMaterialDesc& desc);
	virtual ~D3D11RendererMaterial(void);
	virtual void setModelMatrix(const float* matrix);

	bool tessellationEnabled() const;
	bool tessellationInitialized() const;
	bool tessellationSupported() const;

	bool geometryEnabled() const;
	bool geometryInitialized() const;

private:
	virtual const Renderer& getRenderer() const { return m_renderer; }
	virtual void bind(RendererMaterial::Pass pass, RendererMaterialInstance* materialInstance, bool instanced) const;
	virtual void bindMeshState(bool instanced) const;
	virtual void unbind(void) const;
	virtual void bindVariable(Pass pass, const Variable& variable, const void* data) const;

private:
	typedef RendererMaterial::Variable D3D11BaseVariable;
	class D3D11Variable : public D3D11BaseVariable
	{
	public:
		D3D11Variable(const char* name, RendererMaterial::VariableType type, PxU32 offset)
			: Variable(name, type, offset) { }
		virtual void bind(RendererMaterial::Pass pass, const void* data) = 0;
	};

private:
	D3D11RendererMaterial& operator=(const D3D11RendererMaterial&) { return *this;}

	ID3D11VertexShader*   getVS(bool bInstanced) const { return bInstanced ? m_instancedVertexShader : m_vertexShader; }
	ID3D11PixelShader*    getPS(RendererMaterial::Pass pass) const { return m_fragmentPrograms[pass]; }
	ID3D11GeometryShader* getGS() const { return geometryEnabled()     ? m_geometryShader : NULL; }
	ID3D11HullShader*     getHS() const { return tessellationEnabled() ? m_hullShader     : NULL; }
	ID3D11DomainShader*   getDS() const { return tessellationEnabled() ? m_domainShader   : NULL; }

	ID3DBlob* getVSBlob(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, PxU64 inputDescHash, bool bInstanced) const;
	ID3DBlob* getVSBlob(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, PxU64 inputDescHash, bool bInstanced) const;
	
	const char* getPath(const D3DType type) const { return m_shaderPaths[type].c_str(); }

	void cacheVS(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, PxU64 inputDescHash, bool bInstanced, ID3D11VertexShader** ppShader = NULL, ID3DBlob** ppBlob = NULL) const;
	std::string getShaderNameFromInputLayout(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs,const std::string& shaderName) const;

	void setVariables(RendererMaterial::Pass pass) const;
	void setBlending(RendererMaterial::Pass pass) const;
	void setShaders(bool bInstanced, RendererMaterial::Pass pass) const;

	void loadBlending(const RendererMaterialDesc& desc);
	void loadShaders(const RendererMaterialDesc& desc);

private:
	D3D11Renderer&               m_renderer;

	ID3D11BlendState*            m_blendState;

	mutable ID3D11VertexShader*  m_vertexShader;
	mutable ID3D11VertexShader*  m_instancedVertexShader;
	ID3D11GeometryShader*        m_geometryShader;
	ID3D11HullShader*            m_hullShader;
	ID3D11DomainShader*          m_domainShader;
	ID3D11PixelShader*           m_fragmentPrograms[NUM_PASSES];

	std::string                  m_shaderNames[D3DTypes::NUM_SHADER_TYPES];
	std::string                  m_shaderPaths[D3DTypes::NUM_SHADER_TYPES];
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
