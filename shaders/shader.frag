#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D albedoMap;
layout(set = 0, binding = 2) uniform sampler2D metallicMap;
layout(set = 0, binding = 3) uniform sampler2D normalMap;
layout(set = 0, binding = 4) uniform sampler2D roughnessMap;
layout(set = 0, binding = 5) uniform sampler2D aoMap;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 lightColor = vec3(0.8, 1.0, 0.8);
vec3 lightPos = vec3(0.0, 0.0, 3.0);
float lightRang = 5.0;
vec3 viewPos = vec3(0.0, 0.0, 20.0);

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

vec3 phongShading()
{
	float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

	vec3 norm = normalize(texture(normalMap, fragUV).rgb);
	vec3 lightDir = normalize(lightPos - fragPos);  
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	float specularStrength = 100.0;
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);  

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;  

	return ambient + diffuse + specular;
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
	vec3 albedo     = pow(texture(albedoMap, fragUV).rgb, vec3(2.2, 2.2, 2.2));
    vec3 normal     = getNormalFromNormalMapping();
    float metallic  = texture(metallicMap, fragUV).r;
    float roughness = texture(roughnessMap, fragUV).r;
    float ao        = texture(aoMap, fragUV).r;

	vec3 lightV = lightPos - fragPos;
	vec3 camV = viewPos - fragPos;

	vec3 f0 = mix(vec3(0.16 * 1.0 * 1.0), albedo, metallic);

	// AMBIENT
	vec3 ambient = lightColor * 1.0 * albedo;

	// BRDF
	float cosTheta = dot(normal, lightV);
	vec3 BRDF = vec3(0.0);

	if(cosTheta > 0.0)
	{
		vec3 halfV = normalize(lightV + camV);
		vec3 F = Fresnel(f0, dot(camV, halfV), roughness);

		// DIFFUSE
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
		vec3 diffuse = kD * lightColor * 1.0 * albedo / PI;

		// SPECULAR
		float cosAlpha = dot(normal, halfV);
		float cosRho = dot(normal, camV);

		vec3 specular = vec3(0);
		if(cosAlpha > 0.0 && cosRho > 0.0)
		{
			float NDF = DistributionGGX(cosAlpha, roughness);
			float G = GeometrySmith(cosTheta, cosRho, roughness);
		
			specular = lightColor * 1.0 * (NDF * G * F) / (4.0 * cosTheta * cosRho);
		}

		BRDF = (diffuse + specular) * cosTheta;
	}

	return (ambient + BRDF) * ComputeAttenuation(lightPos, lightRang);
}

void main() 
{
	outColor = vec4(pbrShading(), 1.0);
}