#version 430 core

// VBO-ból érkező változók
layout (location = 0 ) in vec3 inputObjectSpacePosition;

// a pipeline-ban tovább adandó értékek
out vec3 worldPosition;

// shader külső paraméterei - most a három transzformációs mátrixot külön-külön vesszük át
uniform mat4 world;
uniform mat4 viewProj;

void main()
{
	gl_Position = (viewProj * world * vec4( inputObjectSpacePosition, 1 )).xyww;	// [x,y,w,w] => homogén osztás után [x/w, y/w, 1]

	worldPosition = inputObjectSpacePosition;
}
