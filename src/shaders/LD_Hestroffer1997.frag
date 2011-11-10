#version 120
# Power law limb darkening implemented according to Hestroffer (1997)
varying vec3 normal;
varying vec4 color;
uniform float alpha;

void main(void)
{
    float mu = dot(normal, vec3(0.0, 0.0, 1.0));
    float intensity = pow(mu, alpha);

    gl_FragColor = intensity * color;
}