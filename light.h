
#define light(normal, fragPos, diffuseColor, lightPos, outColor) \
   float distance = length(lightPos - fragPos); \
   vec3 lightDir = normalize(lightPos - fragPos); \
   float power = max(dot(normal, lightDir), 0.0) / distance; \
   outColor = vec4((power * 2.0) * diffuseColor, 1.0) 
