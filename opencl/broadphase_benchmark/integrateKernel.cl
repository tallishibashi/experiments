MSTRINGIFY(

float4 quatMult(float4 q1, float4 q2)
{
	float4 q;
	q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	q.y = q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z;
	q.z = q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x;
	q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z; 
	return q;
}

float4 quatNorm(float4 q)
{
	float len = native_sqrt(dot(q, q));
	if(len > 0.f)
	{
		q *= 1.f / len;
	}
	else
	{
		q.x = q.y = q.z = 0.f;
		q.w = 1.f;
	}
	return q;
}


typedef struct
{
	float4 m_pos;
	float4 m_quat;
	float4 m_linVel;
	float4 m_angVel;

	unsigned int m_collidableIdx;
	float m_invMass;
	float m_restituitionCoeff;
	float m_frictionCoeff;
} Body;


__kernel void 
  integrateTransformsKernel( const int startOffset, const int numNodes, __global float4 *g_vertexBuffer,
		   __global float4 *linVel,
		   __global float4 *pAngVel,
		   __global float* pBodyTimes)
{
	int nodeID = get_global_id(0);
	
	
	
	float BT_GPU_ANGULAR_MOTION_THRESHOLD = (0.25f * 3.14159254f);
	float mAmplitude = 66.f;
	float timeStep = 0.0166666f;
	
	if( nodeID < numNodes )
	{
	
		//g_vertexBuffer[nodeID + startOffset/4+numNodes] += pAngVel[nodeID];
		if (1)
		{
			float4 axis;
			//add some hardcoded angular damping
			pAngVel[nodeID].x *= 0.99f;
			pAngVel[nodeID].y *= 0.99f;
			pAngVel[nodeID].z *= 0.99f;
			
			float4 angvel = pAngVel[nodeID];
			float fAngle = native_sqrt(dot(angvel, angvel));
			//limit the angular motion
			if(fAngle*timeStep > BT_GPU_ANGULAR_MOTION_THRESHOLD)
			{
				fAngle = BT_GPU_ANGULAR_MOTION_THRESHOLD / timeStep;
			}
			if(fAngle < 0.001f)
			{
				// use Taylor's expansions of sync function
				axis = angvel * (0.5f*timeStep-(timeStep*timeStep*timeStep)*0.020833333333f * fAngle * fAngle);
			}
			else
			{
				// sync(fAngle) = sin(c*fAngle)/t
				axis = angvel * ( native_sin(0.5f * fAngle * timeStep) / fAngle);
			}
			float4 dorn = axis;
			dorn.w = native_cos(fAngle * timeStep * 0.5f);
			float4 orn0 = g_vertexBuffer[nodeID + startOffset/4+numNodes];
			float4 predictedOrn = quatMult(dorn, orn0);
			predictedOrn = quatNorm(predictedOrn);
			g_vertexBuffer[nodeID + startOffset/4+numNodes]=predictedOrn;
		}

	//linear velocity		
		g_vertexBuffer[nodeID + startOffset/4] +=  linVel[nodeID] * timeStep;
		
	}
}



__kernel void 
  integrateTransformsKernel2( const int startOffset, const int numNodes, __global Body* bodies,
		   __global float4 *linVel,
		   __global float4 *pAngVel,
		   __global float* pBodyTimes)
{
	int nodeID = get_global_id(0);
	
	
	
	float BT_GPU_ANGULAR_MOTION_THRESHOLD = (0.25f * 3.14159254f);
	float mAmplitude = 66.f;
	float timeStep = 0.0166666f;
	
	if( nodeID < numNodes )
	{
	
		//g_vertexBuffer[nodeID + startOffset/4+numNodes] += pAngVel[nodeID];
		if (1)
		{
			float4 axis;
			//add some hardcoded angular damping
			pAngVel[nodeID].x *= 0.99f;
			pAngVel[nodeID].y *= 0.99f;
			pAngVel[nodeID].z *= 0.99f;
			
			float4 angvel = pAngVel[nodeID];
			float fAngle = native_sqrt(dot(angvel, angvel));
			//limit the angular motion
			if(fAngle*timeStep > BT_GPU_ANGULAR_MOTION_THRESHOLD)
			{
				fAngle = BT_GPU_ANGULAR_MOTION_THRESHOLD / timeStep;
			}
			if(fAngle < 0.001f)
			{
				// use Taylor's expansions of sync function
				axis = angvel * (0.5f*timeStep-(timeStep*timeStep*timeStep)*0.020833333333f * fAngle * fAngle);
			}
			else
			{
				// sync(fAngle) = sin(c*fAngle)/t
				axis = angvel * ( native_sin(0.5f * fAngle * timeStep) / fAngle);
			}
			float4 dorn = axis;
			dorn.w = native_cos(fAngle * timeStep * 0.5f);
			float4 orn0 = bodies[nodeID].m_quat;

			float4 predictedOrn = quatMult(dorn, orn0);
			predictedOrn = quatNorm(predictedOrn);
			bodies[nodeID].m_quat=predictedOrn;
		}

	//linear velocity		
		bodies[nodeID].m_pos +=  linVel[nodeID] * timeStep;
		
	}
}



__kernel void 
  sineWaveKernel( const int startOffset, const int numNodes, __global float4 *g_vertexBuffer,
		   __global float4 *linVel,
		   __global float4 *pAngVel,
		   __global float* pBodyTimes)
{
	int nodeID = get_global_id(0);
	float timeStepPos = 0.000166666;
	
	float BT_GPU_ANGULAR_MOTION_THRESHOLD = (0.25f * 3.14159254f);
	float mAmplitude = 166.f;
	
	
	if( nodeID < numNodes )
	{
		pBodyTimes[nodeID] += timeStepPos;
		float4 position = g_vertexBuffer[nodeID + startOffset/4];
		position.x = native_cos(pBodyTimes[nodeID]*2.17f)*mAmplitude + native_sin(pBodyTimes[nodeID])*mAmplitude*0.5f;
		position.y = native_cos(pBodyTimes[nodeID]*1.38f)*mAmplitude + native_sin(pBodyTimes[nodeID]*mAmplitude);
		position.z = native_cos(pBodyTimes[nodeID]*2.17f)*mAmplitude + native_sin(pBodyTimes[nodeID]*0.777f)*mAmplitude;

		//position.x = (int)position.x;
		//position.y = (int)position.y;
		//position.z = (int)position.z;


		g_vertexBuffer[nodeID + startOffset/4] = position;
	}
}



);