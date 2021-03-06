	
	hasCL = findOpenCL_Intel()
	
	if (hasCL) then

		project "OpenCL_gpu_narrowphase_test_Intel"

		initOpenCL_Intel()
	
		language "C++"
				
		kind "ConsoleApp"
		targetdir "../../../bin"


		initOpenGL()
		initGlut()
		initGlew()

			includedirs {
		"../../../rendering/BulletMath",
		"../../primitives",
		"../../../bullet2",
		"../../../dynamics/testbed"
		}

	links { "testbed",
			"BulletDynamics",
			"BulletCollision",
			"LinearMath",
		}		
		files {
			"../main.cpp",
			"../BasicDemo.cpp",
			"../BasicDemo.h",
			"../btGpuDispatcher.h",
			"../btGpuDispatcher.cpp",
			"../btParallelAxisSweep3.cpp",
			"../btParallelAxisSweep3.h",
			"../../gpu_rigidbody_pipeline2/ConvexHullContact.cpp",
			"../../gpu_rigidbody_pipeline2/ConvexHullContact.h",
			"../../basic_initialize/btOpenCLUtils.cpp",
			"../../basic_initialize/btOpenCLUtils.h",
			"../../basic_initialize/btOpenCLInclude.h"
		}
		
	end