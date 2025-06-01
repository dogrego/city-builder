#version 430

// VBO-ból érkező változók
layout( location = 0 ) in vec3 inputObjectSpacePosition;
layout( location = 1 ) in vec3 inputObjectSpaceNormal;
layout( location = 2 ) in vec2 inputTextureCoords;

// a pipeline-ban tovább adandó értékek
out vec3 worldPosition;
out vec3 worldNormal;
out vec2 textureCoords;

// shader külső paraméterei - most a három transzformációs mátrixot külön-külön vesszük át
uniform mat4 world;
uniform mat4 worldInvTransp;
uniform mat4 viewProj;

void main()
{
	gl_Position = viewProj * world * vec4( inputObjectSpacePosition, 1 );
	worldPosition  = (world   * vec4(inputObjectSpacePosition,  1)).xyz;
	worldNormal = (worldInvTransp * vec4(inputObjectSpaceNormal, 0)).xyz;

	textureCoords = inputTextureCoords;
}
