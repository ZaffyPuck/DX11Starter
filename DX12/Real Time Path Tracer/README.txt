Adam Zaffram
Assignment 8 - Final Assignment
IGME 542 - Game Graphics Programming II
Section 1

Description
I chose to make an addition/augmentation to my raytracing project. I added shadow rays - shadows are projected on the ground from a directional light source. I added a sigmoid falloff to planes that do not see light (the back of a sphere).

Noted issues
The program will not run in release mode. I wonder if it has to do with the amount of space I am reserving for the payload here on line 345 of RaytracingHelper.cpp:
shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(DirectX::XMFLOAT3) + sizeof(unsigned int) * 2 + sizeof(bool)*8;
However, I could not get it to work after adjusting a few variables. The output did not have much of an explanation. The program has an access violation at line 444 which tends to be from a lack of resources reserved:
raytracingPipelineStateObject->QueryInterface(IID_PPV_ARGS(&raytracingPipelineProperties));
I am unsure why this would be an issue going from debug to release mode.


Resources 
https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial/extra/dxr_tutorial_extra_another_ray_type