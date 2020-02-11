#version 150 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform sampler2D secondTex;
uniform sampler2D secondBumpTex;
uniform sampler2DShadow shadowTex;

uniform vec3 cameraPos; //keeps cameras world space positon
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
uniform float mixer;
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
  vec3 tangent;
  vec3 binormal;
  vec4 shadowProj;
} IN;

out vec4 fragColour;

void main(void){

  vec4 diffuse1=texture(diffuseTex,IN.texCoord);
  vec4 diffuse2=texture(secondTex,IN.texCoord);
  vec4 totalDiffuse=mix(diffuse1, diffuse2, mixer);

  mat3 TBN = mat3(IN.tangent,IN.binormal,IN.normal);
  vec4 bmSample = texture(bumpTex,IN.texCoord);
  vec4 bmSample2 = texture(secondBumpTex,IN.texCoord);
  vec4 mixBumpMap = mix(bmSample,bmSample2, mixer);

  vec3 normal = normalize(TBN*mixBumpMap.rgb*2.0-1.0);
  vec3 incident=normalize(lightPos-IN.worldPos);
  float lambert = max(0.0,dot(incident,normal));//do max because if the cosine is less than zero, we want cos(theta)=0
  float dist = length(lightPos-IN.worldPos);
  float atten = 1.0-clamp(dist/lightRadius,0.0,1.0);

  vec3 viewDir = normalize(cameraPos-IN.worldPos);
  vec3 halfDir = normalize(incident + viewDir);
  float rFactor = max(0.0,dot(halfDir,normal));
  float specFactor = pow(rFactor,50.0); //here, 50 is how shiny the surface is

  float shadow = 1.0;
  if(IN.shadowProj.w>0.0){
    shadow = textureProj(shadowTex,IN.shadowProj);
}
    lambert *= shadow;

  vec3 colour=(totalDiffuse.rgb*lightColour.rgb);
  colour += (lightColour.rgb*specFactor)*0.33;

  vec3 output = colour*lambert*atten;

  vec3 newDirection = SpotlightDirection;
  newDirection.z =spotLightModZ;
  newDirection.x =spotLightModX;
  
  vec3 spotlightIncident = normalize(SpotlightPos - IN.worldPos);
  vec3 spotlightDir = normalize(-newDirection);
  float fragAngle = dot(spotlightIncident,spotlightDir);



  if(acos(fragAngle) < SpotlightCutoff){
    lambert = max(0.0,dot(spotlightIncident,normal));//do max because if the cosine is less than zero, we want cos(theta)=0
    dist = length(SpotlightPos-IN.worldPos);
    atten = 1.0-clamp(dist/SpotlightRadius,0.0,1.0);

    viewDir = normalize(cameraPos-IN.worldPos);
    halfDir = normalize(spotlightIncident + viewDir);
    rFactor = max(0.0,dot(halfDir,normal));
    specFactor = pow(rFactor,50.0); //here, 50 is how shiny the surface is

    lambert*=shadow;

    colour=(totalDiffuse.rgb*SpotlightColour.rgb);
    colour += (SpotlightColour.rgb*specFactor)*0.33;
    output+=colour*lambert*atten;
}


  fragColour = vec4(output,totalDiffuse.a);
  fragColour.rgb+=(totalDiffuse.rgb*lightColour.rgb)*0.1;
//  fragColour.rgb+=(totalDiffuse.rgb*SpotlightColour.rgb)*0.1;
  //fragColour.rgb=IN.tangent.rgb;

}
