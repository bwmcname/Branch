
#define light(normal, fragPos, diffuseColor, lightPos, outColor)	\
   float distance = length(lightPos - fragPos);				\
   vec3 lightDir = normalize(lightPos - fragPos);			\
   float power = max(dot(normal, lightDir), 0.0) / distance;		\
   vec3 ambient = 0.2 * diffuseColor;					\
   outColor = vec4(((power * 2.0) * diffuseColor) + ambient, 1.0)
