#version 150 core

uniform sampler2D diffuseTex;
uniform samplerCube cubeTex;

uniform vec4 lightColour;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform float lightRadius;
uniform float waterTransparency;

uniform float spotLightModX;
uniform float spotLightModZ;

uniform vec4 SpotlightColour;
uniform vec3 SpotlightPos;
uniform float SpotlightRadius;
uniform vec3 SpotlightDirection;
uniform float SpotlightCutoff;

in Vertex{
  vec4 colour;
  vec2 texCoord;
  vec3 normal;
  vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void){
  vec4 diffuse = texture(diffuseTex, IN.texCoord)*IN.colour;
  vec3 incident=normalize(IN.worldPos-cameraPos);

  float dist = length(lightPos - IN.worldPos);
  float atten = 1.0-clamp((dist/lightRadius),0.2,1.0);
  vec4 reflection = texture(cubeTex,reflect(incident, normalize(IN.normal)));

  fragColour = (lightColour*diffuse*atten)*(diffuse+reflection);

  vec3 newDirection = SpotlightDirection;

  newDirection.z =spotLightModZ;
  newDirection.x =spotLightModX;

  vec3 spotlightIncident = normalize(SpotlightPos - IN.worldPos);
  vec3 spotlightDir = normalize(-newDirection);
  float fragAngle = dot(spotlightIncident,spotlightDir);

  if(acos(fragAngle) < SpotlightCutoff){
    dist = length(SpotlightPos-IN.worldPos);
    atten = 1.0-clamp(dist/SpotlightRadius,0.2,1.0);

    fragColour+=(SpotlightColour*diffuse*atten)*(diffuse+reflection);
}

  fragColour= vec4(fragColour.rgb,fragColour.a *= waterTransparency);
}
