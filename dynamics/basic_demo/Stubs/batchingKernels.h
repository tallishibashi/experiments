//this file is autogenerated using stringify.bat (premake --stringify) in the build folder of this project
static const char* batchingKernelsCL= \
"/*\n"
"Copyright (c) 2012 Advanced Micro Devices, Inc.  \n"
"\n"
"This software is provided 'as-is', without any express or implied warranty.\n"
"In no event will the authors be held liable for any damages arising from the use of this software.\n"
"Permission is granted to anyone to use this software for any purpose, \n"
"including commercial applications, and to alter it and redistribute it freely, \n"
"subject to the following restrictions:\n"
"\n"
"1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.\n"
"2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.\n"
"3. This notice may not be removed or altered from any source distribution.\n"
"*/\n"
"//Originally written by Takahiro Harada\n"
"\n"
"\n"
"#pragma OPENCL EXTENSION cl_amd_printf : enable\n"
"#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable\n"
"#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable\n"
"#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable\n"
"#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable\n"
"\n"
"#ifdef cl_ext_atomic_counters_32\n"
"#pragma OPENCL EXTENSION cl_ext_atomic_counters_32 : enable\n"
"#else\n"
"#define counter32_t volatile __global int*\n"
"#endif\n"
"\n"
"\n"
"typedef unsigned int u32;\n"
"typedef unsigned short u16;\n"
"typedef unsigned char u8;\n"
"\n"
"#define GET_GROUP_IDX get_group_id(0)\n"
"#define GET_LOCAL_IDX get_local_id(0)\n"
"#define GET_GLOBAL_IDX get_global_id(0)\n"
"#define GET_GROUP_SIZE get_local_size(0)\n"
"#define GET_NUM_GROUPS get_num_groups(0)\n"
"#define GROUP_LDS_BARRIER barrier(CLK_LOCAL_MEM_FENCE)\n"
"#define GROUP_MEM_FENCE mem_fence(CLK_LOCAL_MEM_FENCE)\n"
"#define AtomInc(x) atom_inc(&(x))\n"
"#define AtomInc1(x, out) out = atom_inc(&(x))\n"
"#define AppendInc(x, out) out = atomic_inc(x)\n"
"#define AtomAdd(x, value) atom_add(&(x), value)\n"
"#define AtomCmpxhg(x, cmp, value) atom_cmpxchg( &(x), cmp, value )\n"
"#define AtomXhg(x, value) atom_xchg ( &(x), value )\n"
"\n"
"\n"
"#define SELECT_UINT4( b, a, condition ) select( b,a,condition )\n"
"\n"
"#define make_float4 (float4)\n"
"#define make_float2 (float2)\n"
"#define make_uint4 (uint4)\n"
"#define make_int4 (int4)\n"
"#define make_uint2 (uint2)\n"
"#define make_int2 (int2)\n"
"\n"
"\n"
"#define max2 max\n"
"#define min2 min\n"
"\n"
"\n"
"#define WG_SIZE 64\n"
"\n"
"\n"
"\n"
"typedef struct \n"
"{\n"
"	float4 m_worldPos[4];\n"
"	float4 m_worldNormal;\n"
"	u32 m_coeffs;\n"
"	int m_batchIdx;\n"
"\n"
"	int m_bodyA;//sign bit set for fixed objects\n"
"	int m_bodyB;\n"
"}Contact4;\n"
"\n"
"typedef struct \n"
"{\n"
"	int m_n;\n"
"	int m_start;\n"
"	int m_staticIdx;\n"
"	int m_paddings[1];\n"
"} ConstBuffer;\n"
"\n"
"typedef struct \n"
"{\n"
"	int m_a;\n"
"	int m_b;\n"
"	u32 m_idx;\n"
"}Elem;\n"
"\n"
"#define STACK_SIZE (WG_SIZE*10)\n"
"//#define STACK_SIZE (WG_SIZE)\n"
"#define RING_SIZE 1024\n"
"#define RING_SIZE_MASK (RING_SIZE-1)\n"
"#define CHECK_SIZE (WG_SIZE)\n"
"\n"
"\n"
"#define GET_RING_CAPACITY (RING_SIZE - ldsRingEnd)\n"
"#define RING_END ldsTmp\n"
"\n"
"u32 readBuf(__local u32* buff, int idx)\n"
"{\n"
"	idx = idx % (32*CHECK_SIZE);\n"
"	int bitIdx = idx%32;\n"
"	int bufIdx = idx/32;\n"
"	return buff[bufIdx] & (1<<bitIdx);\n"
"}\n"
"\n"
"void writeBuf(__local u32* buff, int idx)\n"
"{\n"
"	idx = idx % (32*CHECK_SIZE);\n"
"	int bitIdx = idx%32;\n"
"	int bufIdx = idx/32;\n"
"//	buff[bufIdx] |= (1<<bitIdx);\n"
"	atom_or( &buff[bufIdx], (1<<bitIdx) );\n"
"}\n"
"\n"
"u32 tryWrite(__local u32* buff, int idx)\n"
"{\n"
"	idx = idx % (32*CHECK_SIZE);\n"
"	int bitIdx = idx%32;\n"
"	int bufIdx = idx/32;\n"
"	u32 ans = (u32)atom_or( &buff[bufIdx], (1<<bitIdx) );\n"
"	return ((ans >> bitIdx)&1) == 0;\n"
"}\n"
"\n"
"//	batching on the GPU\n"
"__kernel void CreateBatches( __global const Contact4* gConstraints, __global Contact4* gConstraintsOut,\n"
"		__global const u32* gN, __global const u32* gStart, \n"
"		int m_staticIdx )\n"
"{\n"
"	__local u32 ldsStackIdx[STACK_SIZE];\n"
"	__local u32 ldsStackEnd;\n"
"	__local Elem ldsRingElem[RING_SIZE];\n"
"	__local u32 ldsRingEnd;\n"
"	__local u32 ldsTmp;\n"
"	__local u32 ldsCheckBuffer[CHECK_SIZE];\n"
"	__local u32 ldsFixedBuffer[CHECK_SIZE];\n"
"	__local u32 ldsGEnd;\n"
"	__local u32 ldsDstEnd;\n"
"\n"
"	int wgIdx = GET_GROUP_IDX;\n"
"	int lIdx = GET_LOCAL_IDX;\n"
"	\n"
"	const int m_n = gN[wgIdx];\n"
"	const int m_start = gStart[wgIdx];\n"
"		\n"
"	if( lIdx == 0 )\n"
"	{\n"
"		ldsRingEnd = 0;\n"
"		ldsGEnd = 0;\n"
"		ldsStackEnd = 0;\n"
"		ldsDstEnd = m_start;\n"
"	}\n"
"	\n"
"//	while(1)\n"
"//was 250\n"
"	for(int ie=0; ie<50; ie++)\n"
"	{\n"
"		ldsFixedBuffer[lIdx] = 0;\n"
"\n"
"		for(int giter=0; giter<4; giter++)\n"
"		{\n"
"			int ringCap = GET_RING_CAPACITY;\n"
"		\n"
"			//	1. fill ring\n"
"			if( ldsGEnd < m_n )\n"
"			{\n"
"				while( ringCap > WG_SIZE )\n"
"				{\n"
"					if( ldsGEnd >= m_n ) break;\n"
"					if( lIdx < ringCap - WG_SIZE )\n"
"					{\n"
"						int srcIdx;\n"
"						AtomInc1( ldsGEnd, srcIdx );\n"
"						if( srcIdx < m_n )\n"
"						{\n"
"							int dstIdx;\n"
"							AtomInc1( ldsRingEnd, dstIdx );\n"
"							\n"
"							int a = gConstraints[m_start+srcIdx].m_bodyA;\n"
"							int b = gConstraints[m_start+srcIdx].m_bodyB;\n"
"							ldsRingElem[dstIdx].m_a = (a>b)? b:a;\n"
"							ldsRingElem[dstIdx].m_b = (a>b)? a:b;\n"
"							ldsRingElem[dstIdx].m_idx = srcIdx;\n"
"						}\n"
"					}\n"
"					ringCap = GET_RING_CAPACITY;\n"
"				}\n"
"			}\n"
"\n"
"			GROUP_LDS_BARRIER;\n"
"	\n"
"			//	2. fill stack\n"
"			__local Elem* dst = ldsRingElem;\n"
"			if( lIdx == 0 ) RING_END = 0;\n"
"\n"
"			int srcIdx=lIdx;\n"
"			int end = ldsRingEnd;\n"
"\n"
"			{\n"
"				for(int ii=0; ii<end; ii+=WG_SIZE, srcIdx+=WG_SIZE)\n"
"				{\n"
"					Elem e;\n"
"					if(srcIdx<end) e = ldsRingElem[srcIdx];\n"
"					bool done = (srcIdx<end)?false:true;\n"
"\n"
"					for(int i=lIdx; i<CHECK_SIZE; i+=WG_SIZE) ldsCheckBuffer[lIdx] = 0;\n"
"					\n"
"					if( !done )\n"
"					{\n"
"						int aUsed = readBuf( ldsFixedBuffer, abs(e.m_a));\n"
"						int bUsed = readBuf( ldsFixedBuffer, abs(e.m_b));\n"
"\n"
"						if( aUsed==0 && bUsed==0 )\n"
"						{\n"
"							int aAvailable;\n"
"							int bAvailable;\n"
"							int ea = abs(e.m_a);\n"
"							int eb = abs(e.m_b);\n"
"\n"
"							aAvailable = tryWrite( ldsCheckBuffer, ea );\n"
"							bAvailable = tryWrite( ldsCheckBuffer, eb );\n"
"\n"
"							aAvailable = (e.m_a<0)? 1: aAvailable;\n"
"							bAvailable = (e.m_b<0)? 1: bAvailable;\n"
"							\n"
"							aAvailable = (e.m_a==m_staticIdx)? 1: aAvailable;\n"
"							bAvailable = (e.m_b==m_staticIdx)? 1: bAvailable;\n"
"\n"
"							bool success = (aAvailable && bAvailable);\n"
"							if(success)\n"
"							{\n"
"								writeBuf( ldsFixedBuffer, ea );\n"
"								writeBuf( ldsFixedBuffer, eb );\n"
"							}\n"
"							done = success;\n"
"						}\n"
"					}\n"
"\n"
"					//	put it aside\n"
"					if(srcIdx<end)\n"
"					{\n"
"						if( done )\n"
"						{\n"
"							int dstIdx; AtomInc1( ldsStackEnd, dstIdx );\n"
"							if( dstIdx < STACK_SIZE )\n"
"								ldsStackIdx[dstIdx] = e.m_idx;\n"
"							else{\n"
"								done = false;\n"
"								AtomAdd( ldsStackEnd, -1 );\n"
"							}\n"
"						}\n"
"						if( !done )\n"
"						{\n"
"							int dstIdx; AtomInc1( RING_END, dstIdx );\n"
"							dst[dstIdx] = e;\n"
"						}\n"
"					}\n"
"\n"
"					//	if filled, flush\n"
"					if( ldsStackEnd == STACK_SIZE )\n"
"					{\n"
"						for(int i=lIdx; i<STACK_SIZE; i+=WG_SIZE)\n"
"						{\n"
"							int idx = m_start + ldsStackIdx[i];\n"
"							int dstIdx; AtomInc1( ldsDstEnd, dstIdx );\n"
"							gConstraintsOut[ dstIdx ] = gConstraints[ idx ];\n"
"							gConstraintsOut[ dstIdx ].m_batchIdx = ie;\n"
"						}\n"
"						if( lIdx == 0 ) ldsStackEnd = 0;\n"
"\n"
"						//for(int i=lIdx; i<CHECK_SIZE; i+=WG_SIZE) \n"
"						ldsFixedBuffer[lIdx] = 0;\n"
"					}\n"
"				}\n"
"			}\n"
"\n"
"			if( lIdx == 0 ) ldsRingEnd = RING_END;\n"
"		}\n"
"\n"
"		GROUP_LDS_BARRIER;\n"
"\n"
"		for(int i=lIdx; i<ldsStackEnd; i+=WG_SIZE)\n"
"		{\n"
"			int idx = m_start + ldsStackIdx[i];\n"
"			int dstIdx; AtomInc1( ldsDstEnd, dstIdx );\n"
"			gConstraintsOut[ dstIdx ] = gConstraints[ idx ];\n"
"			gConstraintsOut[ dstIdx ].m_batchIdx = ie;\n"
"		}\n"
"\n"
"		//	in case it couldn't consume any pair. Flush them\n"
"		//	todo. Serial batch worth while?\n"
"		if( ldsStackEnd == 0 )\n"
"		{\n"
"			for(int i=lIdx; i<ldsRingEnd; i+=WG_SIZE)\n"
"			{\n"
"				int idx = m_start + ldsRingElem[i].m_idx;\n"
"				int dstIdx; AtomInc1( ldsDstEnd, dstIdx );\n"
"				gConstraintsOut[ dstIdx ] = gConstraints[ idx ];\n"
"				gConstraintsOut[ dstIdx ].m_batchIdx = 100+i;\n"
"			}\n"
"			GROUP_LDS_BARRIER;\n"
"			if( lIdx == 0 ) ldsRingEnd = 0;\n"
"		}\n"
"\n"
"		if( lIdx == 0 ) ldsStackEnd = 0;\n"
"\n"
"		GROUP_LDS_BARRIER;\n"
"\n"
"		//	termination\n"
"		if( ldsGEnd == m_n && ldsRingEnd == 0 )\n"
"			break;\n"
"	}\n"
"\n"
"\n"
"}\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
;
