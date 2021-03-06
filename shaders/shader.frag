#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D metallicMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D roughnessMap;
layout(set = 1, binding = 4) uniform sampler2D aoMap;

layout(set = 0, binding = 1) uniform LightData {
	vec3 _pos;
	float _intensity;
	vec3 _color;
	float _range;
} light;

layout(set = 0, binding = 2) uniform samplerCube skyboxCubeMap;
layout(set = 0, binding = 3) uniform samplerCube irradianceCubeMap;
layout(set = 0, binding = 4) uniform sampler2D lookupTableBRDF;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragCamPos;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 getNormalFromNormalMapping()
{
	vec3 T = normalize(fragTangent);
	vec3 N = normalize(fragNormal);
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * (texture(normalMap, fragUV).rgb * 2.0 - 1.0)); 
}

vec3 Fresnel(vec3 f0, float cosTheta, float roughness)
{
	return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cosTheta, 5.0);
}  

float DistributionGGX(float cosAlpha, float roughness)
{
	float roughSqr = roughness * roughness;

	float denom = cosAlpha * cosAlpha * (roughSqr - 1.0) + 1.0;

    return roughSqr / (PI * denom * denom);
}

float GeometrySchlickGGX(float cosRho, float roughness)
{
	float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;

    return cosRho / (cosRho * (1.0 - k) + k);
}

float GeometrySmith(float cosTheta, float cosRho, float roughness)
{
	float ggx1 = GeometrySchlickGGX(cosRho, roughness);
    float ggx2 = GeometrySchlickGGX(cosTheta, roughness);
	
    return ggx1 * ggx2;
}

float ComputeAttenuation(vec3 lightPosition, float lightRange)
{
	float distance = length(lightPosition - fragPos);

	return max(1 - (distance / lightRange), 0.0);
}

vec3 pbrShading()
{
	vec3 albedo     = texture(albedoMap, fragUV).rgb;
    vec3 normal     = getNormalFromNormalMapping();
    float metallic  = texture(metallicMap, fragUV).r;
    float roughness = texture(roughnessMap, fragUV).r;
    float ao        = texture(aoMap, fragUV).r;

	vec3 lightV = normalize(light._pos - fragPos);
	vec3 camV = normalize(fragCamPos - fragPos);

	vec3 f0 = mix(vec3(0.16), albedo, metallic);

	// AMBIENT
	vec3 ambient = light._color  * albedo;

	// BRDF
	float cosTheta = dot(normal, lightV);
	vec3 BRDF = vec3(0.0);

	if(cosTheta > 0.0)
	{
		vec3 halfV = normalize(lightV + camV);
		vec3 F = Fresnel(f0, dot(camV, halfV), roughness);

		// DIFFUSE
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
		vec3 diffuse = (kD * light._color * albedo) / PI;

		// SPECULAR
		float cosAlpha = dot(normal, halfV);
		float cosRho = dot(normal, camV);

		vec3 specular = vec3(0);
		if(cosAlpha > 0.0 && cosRho > 0.0)
		{
			float NDF = DistributionGGX(cosAlpha, roughness);
			float G = GeometrySmith(cosTheta, cosRho, roughness);
		
			specular = light._color * (NDF * G * F) / (4.0 * cosTheta * cosRho);
		}

		BRDF = (diffuse + specular) * cosTheta;
	}

	// IBL
	float cosAlpha = max(dot(camV, normal), 0.0);

	vec3 kS = Fresnel(f0, cosAlpha, roughness);

	vec3 refl = reflect(-camV, normal);
	vec3 prefilteredColor = textureLod(skyboxCubeMap, refl, roughness).rgb;
	vec2 envBRDF  = texture(lookupTableBRDF, vec2(cosAlpha, roughness)).rg;
	vec3 specular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);

	// DIFFUSE
	vec3 irradiance = texture(irradianceCubeMap, normal).xyz;

	vec3 kD = (1.0 - kS) * (1.0 - metallic);
	vec3 diffuse = kD * irradiance * albedo;

	vec3 ibl = (diffuse + specular) * ao;

	return (ambient + BRDF) * ComputeAttenuation(light._pos, light._range) * light._intensity + ibl;
}

void main() 
{
	outColor = vec4(pbrShading(), 1.0);
}