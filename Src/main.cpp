#include "Precompiled.h"
#include "../../../Source/Modules/Terrain/TerrainMisc.h"
#include <shellapi.h>

//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int w, h;
	E_GetMonitorResolution(w, h);
	w *= 0.95f;
	h *= 0.95f;
	
	if (!Engine_Init(NULL, "SimView2", "1.0", w, h, "", "sandbox_simview2/"))//, eInvisible/*|eFullScreen*/))
		return 0;

	//E_SetWindowPos(550, 550);
	//E_ShowWindow(true);
	

	gConsole.ExecuteFile("content_simview2/engine.cfg");

	while (Engine_DoFrame()) {}

	Engine_Deinit();

	return 0;
}









// OLD TEST CODE BELOW


//--------------------------------------------------------------------------------------
// Copyright 2014 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
// Created by: filip.strugar@intel.com
//--------------------------------------------------------------------------------------
//
// - function "GenerateGaussShaderKernelWeightsAndOffsets" will generate shader 
//      constants for high performance Gaussian blur filter implementation (separable, 
//      using hardware linear filter when sampling to get two samples at a time)
//
// - function "GenerateGaussFunctionCode" will generate GLSL code using the above
//      constants; for HLSL replace vec2/vec3 with float2/float3, etc, "should work"
//
// - Acceptable kernel sizes are of 4*n-1 (3, 7, 11, 15, ...)
//
// - Gauss distribution Sigma value is generated "ad hoc": feel free to modify the code
//      for your purpose. Performance of the algorithm is only dependent on kernel size.
//


#if 0
#include <assert.h>
#include <vector>
#include <string>
#include <stdarg.h>

inline std::string stringFormatA(const char * fmt, ...)
{
	int nSize = 0;
	char buff[4096];
	va_list args;
	va_start(args, fmt);
	nSize = vsnprintf(buff, sizeof(buff) - 1, fmt, args); // C4996
	return std::string(buff);
}

inline std::vector<double> GenerateSeparableGaussKernel(double sigma, int kernelSize)
{
	if ((kernelSize % 2) != 1)
	{
		assert(false); // kernel size must be odd number
		return std::vector<double>();
	}

	int halfKernelSize = kernelSize / 2;

	std::vector<double> kernel;
	kernel.resize(kernelSize);

	const double cPI = 3.14159265358979323846;
	double mean = halfKernelSize;
	double sum = 0.0;
	for (int x = 0; x < kernelSize; ++x)
	{
		kernel[x] = (float)sqrt(exp(-0.5 * (pow((x - mean) / sigma, 2.0) + pow((mean) / sigma, 2.0)))
			/ (2 * cPI * sigma * sigma));
		sum += kernel[x];
	}
	for (int x = 0; x < kernelSize; ++x)
		kernel[x] /= (float)sum;

	return kernel;
}

inline std::vector<float> GetAppropriateSeparableGauss(int kernelSize)
{
	if ((kernelSize % 2) != 1)
	{
		assert(false); // kernel size must be odd number
		return std::vector<float>();
	}

	// Search for sigma to cover the whole kernel size with sensible values (might not be ideal for all cases quality-wise but is good enough for performance testing)
	const double epsilon = 2e-2f / kernelSize;
	double searchStep = 1.0;
	double sigma = 1.0;
	while (true)
	{

		std::vector<double> kernelAttempt = GenerateSeparableGaussKernel(sigma, kernelSize);
		if (kernelAttempt[0] > epsilon)
		{
			if (searchStep > 0.02)
			{
				sigma -= searchStep;
				searchStep *= 0.1;
				sigma += searchStep;
				continue;
			}
			std::vector<float> retVal;
			for (int i = 0; i < kernelSize; i++)
				retVal.push_back((float)kernelAttempt[i]);
			return retVal;
		}

		sigma += searchStep;

		if (sigma > 1000.0)
		{
			assert(false); // not tested, preventing infinite loop
		}
	}

	return std::vector<float>();
}

inline std::string GenerateGaussShaderKernelWeightsAndOffsets(int kernelSize, bool forPreprocessorDefine = false, bool workaroundForNoCLikeArrayInitialization = false)
{
	// Gauss filter kernel & offset creation
	std::vector<float> inputKernel = GetAppropriateSeparableGauss(kernelSize);

	assert((kernelSize % 2) == 1);
	assert((((kernelSize / 2) + 1) % 2) == 0);

	std::vector<float> oneSideInputs;
	for (int i = (kernelSize / 2); i >= 0; i--)
	{
		if (i == (kernelSize / 2))
			oneSideInputs.push_back((float)inputKernel[i] * 0.5f);
		else
			oneSideInputs.push_back((float)inputKernel[i]);
	}

	assert((oneSideInputs.size() % 2) == 0);
	int numSamples = oneSideInputs.size() / 2;

	std::vector<float> weights;

	for (int i = 0; i < numSamples; i++)
	{
		float sum = oneSideInputs[i * 2 + 0] + oneSideInputs[i * 2 + 1];
		weights.push_back(sum);
	}

	std::vector<float> offsets;

	for (int i = 0; i < numSamples; i++)
	{
		offsets.push_back(i*2.0f + oneSideInputs[i * 2 + 1] / weights[i]);
	}

	std::string indent = "    ";

	std::string shaderCode = (forPreprocessorDefine) ? ("") : ("");
	std::string eol = (forPreprocessorDefine) ? ("\\\n") : ("\n");
	//if (!forPreprocessorDefine) shaderCode += indent + "//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;" + eol;
	if (!forPreprocessorDefine) shaderCode += indent + stringFormatA("// Kernel width %d x %d", kernelSize, kernelSize) + eol;
	//if (!forPreprocessorDefine) shaderCode += indent + "//" + eol;
	shaderCode += indent + stringFormatA("const int stepCount = %d;", numSamples) + eol;

	if (!workaroundForNoCLikeArrayInitialization)
	{
		if (!forPreprocessorDefine) shaderCode += indent + "//" + eol;
		shaderCode += indent + "const float gWeights[stepCount] ={" + eol;
		for (int i = 0; i < numSamples; i++)
			shaderCode += indent + stringFormatA("   %.5f", weights[i]) + ((i != (numSamples - 1)) ? (",") : ("")) + eol;
		shaderCode += indent + "};" + eol;
		shaderCode += indent + "const float gOffsets[stepCount] ={" + eol;
		for (int i = 0; i < numSamples; i++)
			shaderCode += indent + stringFormatA("   %.5f", offsets[i]) + ((i != (numSamples - 1)) ? (",") : ("")) + eol;
		shaderCode += indent + "};" + eol;
	}
	else
	{
		if (!forPreprocessorDefine) shaderCode += indent + "//" + eol;
		shaderCode += indent + "float gWeights[stepCount];" + eol;
		for (int i = 0; i < numSamples; i++)
			shaderCode += indent + stringFormatA(" gWeights[%d] = %.5f;", i, weights[i]) + eol;
		shaderCode += indent + eol;
		shaderCode += indent + "float gOffsets[stepCount];" + eol;
		for (int i = 0; i < numSamples; i++)
			shaderCode += indent + stringFormatA(" gOffsets[%d] = %.5f;", i, offsets[i]) + eol;
		shaderCode += indent + eol;
	}

	if (!forPreprocessorDefine) shaderCode += indent + "//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////;" + eol;

	return shaderCode;
}

inline std::string GenerateGaussFunctionCode(int kernelSize, bool workaroundForNoCLikeArrayInitialization = false)
{
	std::string shaderCode;

	std::string eol = "\n";

	shaderCode += eol;
	//shaderCode += "// automatically generated by GenerateGaussFunctionCode in GaussianBlur.h                                                                                            " + eol;
	//shaderCode += "vec3 GaussianBlur( sampler2D tex0, vec2 centreUV, vec2 halfPixelOffset, vec2 pixelOffset )                                                                           " + eol;
	shaderCode += "vec3 GaussianBlur( sampler2D tex0, vec2 centreUV, vec2 pixelOffset )" + eol;
	shaderCode += "{" + eol;
	shaderCode += "    vec3 colOut = vec3( 0, 0, 0 );" + eol;
	shaderCode += eol;
	shaderCode += GenerateGaussShaderKernelWeightsAndOffsets(kernelSize, false, workaroundForNoCLikeArrayInitialization);
	shaderCode += eol;
	shaderCode += "    for( int i = 0; i < stepCount; i++ )" + eol;
	shaderCode += "    {" + eol;
	shaderCode += "        vec2 texCoordOffset = gOffsets[i] * pixelOffset;" + eol;
	//shaderCode += "        vec3 col = texture( tex0, centreUV + texCoordOffset ).xyz + texture( tex0, centreUV - texCoordOffset ).xyz;                                                " + eol;
	shaderCode += "        vec3 col = texture2D( tex0, centreUV + texCoordOffset ).xyz + texture2D( tex0, centreUV - texCoordOffset ).xyz;" + eol;
	shaderCode += "        colOut += gWeights[i] * col;" + eol;
	shaderCode += "    }" + eol;
	shaderCode += eol;
	shaderCode += "    return colOut;" + eol;
	shaderCode += "}" + eol;
	shaderCode += eol;

	return shaderCode;
}
#endif

//-----------------------------------------------------------------------------
/*static inline float ASurfaceArea(const Caabb& a)
{
	//return B3_TWO * (Width() * Depth() + Width() * Height() + Depth() * Height());
	CVec3 d = a.Diameter();
	return 2.0f * (d.x*d.y + d.x*d.z + d.y*d.z);
}
//-----------------------------------------------------------------------------
static inline float AVolume(const Caabb& a)
{
	CVec3 d = a.Diameter();
	return d.x*d.y*d.z;
}
void TestRelation()
{
	Caabb a;
	a.m_Min = CVec3(0, 0, 0);
	a.m_Max = CVec3(1, 1, 1);

	for (int i = 1; i <= 10; i++)
	{
		a.m_Max.x = i;
		CVec3 d = a.Diameter();
		LOG("<%.0f, %.0f, %.0f> > A: %.1f, V: %.1f, R: %.3f\n", d.x, d.y, d.z, ASurfaceArea(a), AVolume(a), ASurfaceArea(a) / AVolume(a));
	}
	for (int i = 2; i <= 10; i++)
	{
		a.m_Max.y = i;
		CVec3 d = a.Diameter();
		LOG("<%.0f, %.0f, %.0f> > A: %.1f, V: %.1f, R: %.3f\n", d.x, d.y, d.z, ASurfaceArea(a), AVolume(a), ASurfaceArea(a) / AVolume(a));
	}
	for (int i = 2; i <= 10; i++)
	{
		a.m_Max.y = i;
		a.m_Max.z = i;
		CVec3 d = a.Diameter();
		LOG("<%.0f, %.0f, %.0f> > A: %.1f, V: %.1f, R: %.3f\n", d.x, d.y, d.z, ASurfaceArea(a), AVolume(a), ASurfaceArea(a) / AVolume(a));
	}
}

void TestDynTree()
{
	DynAabbTree tree;

	const int N = 10000;
	Caabb* aabbs = new Caabb[N];
	
	for (int i = 0; i < N; i++)
	{
		float s = rangeRandf(0.1f, 30.0f);
		
		CVec3 s3;
		s3.x = s * rangeRandf(0.5f, 1.5f);
		s3.y = s * rangeRandf(0.5f, 1.5f);
		s3.z = s * rangeRandf(0.5f, 1.5f);

		aabbs[i].m_Min.x = rangeRandf(-1000.0f, 1000.0f);
		aabbs[i].m_Min.y = rangeRandf(-1000.0f, 1000.0f);
		aabbs[i].m_Min.z = rangeRandf(-1000.0f, 1000.0f);
		aabbs[i].m_Max = aabbs[i].m_Min + s3;

		int abba = 10;
	}

	CStopWatch sw;
	for (int i = 0; i < N; i++)
	{
		tree.InsertNode(aabbs[i], NULL, false);
	}
	float t1 = sw.StopGetMs();

	LOG("Time %d: Total: %.3f, per instance: %.4f\n", N, t1, t1 / N);
	LOG("Time %d: Total: %.3f, per instance: %.4f\n", N, t1, t1 / N);

	delete[] aabbs;
}*/
