varying vec3 normal;
varying vec4 pos;
varying vec4 vertexColor;
uniform sampler2D diffuse;
/*
float calculateAttenuation(in int i, in float dist)
{
    return(1.0 / (gl_LightSource[i].constantAttenuation +
                  gl_LightSource[i].linearAttenuation * dist +
                  gl_LightSource[i].quadraticAttenuation * dist * dist));
}

void pointLight(in int i, in vec3 normal, in vec4 pos, inout vec4 diffuse, inout vec4 specular) {
	vec4 color = gl_FrontMaterial.diffuse;
	vec4 matspec = gl_FrontMaterial.specular;
	float shininess = gl_FrontMaterial.shininess;
	vec4 lightspec = gl_LightSource[i].specular;
	vec4 lpos = gl_LightSource[i].position;
	vec4 s = pos-lpos; 
	vec4 sn = -normalize(s);
	
	vec3 light = sn.xyz;
	vec3 n = normalize(normal);
	vec3 r = -reflect(light, n);
	r = normalize(r);
	vec3 v = -pos.xyz;
	v = normalize(v);

	float nDotL = dot(n, sn.xyz);
	if(nDotL > 0.0) {
		float dist = length(s);    
		float attenuation = calculateAttenuation(i, dist);

		diffuse  += color * max(0.0, nDotL) * gl_LightSource[i].diffuse * attenuation;

	  if (shininess != 0.0) {
    	specular += lightspec * matspec * pow(max(0.0,dot(r, v)), shininess) * attenuation;
	  }
	}
}


void spotLight(in int i, in vec3 normal, in vec4 pos, inout vec4 diffuse, inout vec4 specular) {
	vec4 color = gl_FrontMaterial.diffuse;
	vec4 matspec = gl_FrontMaterial.specular;
	float shininess = gl_FrontMaterial.shininess;
	vec4 lightspec = gl_LightSource[i].specular;
	vec4 lpos = gl_LightSource[i].position;
	vec4 s = pos-lpos; 
	vec4 sn = -normalize(s);

	vec3 light = sn.xyz;
	vec3 n = normalize(normal);
	vec3 r = -reflect(light, n);
	r = normalize(r);
	vec3 v = -pos.xyz;
	v = normalize(v);

	float cos_outer_cone_angle = gl_LightSource[i].spotExponent;
	float cos_cur_angle = dot(-normalize(gl_LightSource[i].spotDirection), sn.xyz);
	float cos_inner_cone_angle = gl_LightSource[i].spotCosCutoff;

	float cos_inner_minus_outer_angle = cos_inner_cone_angle - cos_outer_cone_angle;
	float spot = 0.0;
	spot = clamp((cos_cur_angle - cos_outer_cone_angle) / cos_inner_minus_outer_angle, 0.0, 1.0);
	       
	float nDotL = dot(n, sn.xyz);
	if(nDotL > 0.0) {
		float dist = length(s);    
		float attenuation = calculateAttenuation(i, dist);
		diffuse  += color * max(0.0, nDotL) * gl_LightSource[i].diffuse * attenuation * spot;

	  if (shininess != 0.0) {
    	specular += lightspec * matspec * pow(max(0.0,dot(r, v)), shininess) * attenuation * spot;
	  }
	}
}

void doLights(in int numLights, in vec3 normal, in vec4 pos, inout vec4 diffuse, inout vec4 specular) {
	for (int i = 0; i < numLights; i++) {
		if (gl_LightSource[i].spotCutoff == 180.0) {
			pointLight(i, normal, pos, diffuse, specular);
		} else {
			spotLight(i, normal, pos, diffuse, specular);
		}
    }
}
*/

void main()
{
	vec4 texColor = texture2D(diffuse, gl_TexCoord[0].st);		

#if 1
	float atten = dot(normal, vec3(1,-1,1));
	vec4 shadeColor = vec4(0.5,0.5,0.5,0.5) + vec4(atten,atten,atten,1.0)/2.0;
	texColor = texColor * shadeColor;
#endif

    gl_FragColor = clamp(vertexColor*texColor, 0.0, 1.0);
    // fog
	/*
	const float LOG2 = 1.442695;
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = exp2( -gl_Fog.density * 
				   gl_Fog.density * 
				   z * 
				   z * 
				   LOG2 );

	fogFactor = clamp(fogFactor, 0.0, 1.0);
	
	gl_FragColor = mix(gl_Fog.color, color, fogFactor );
	*/
}
