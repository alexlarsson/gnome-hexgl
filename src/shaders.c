#include "shaders.h"


/* ------------------------------------------------------------------------------------------------
//	Hexagonal Vignette shader
//  by BKcore.com
------------------------------------------------------------------------------------------------ */

static const char *hexvignette_vertex_shader =
  "varying vec2 vUv;\n"
  "void main()\n"
  "{\n"
  "  vUv = uv;\n"
  "  gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );\n"
  "}\n";

static const char *hexvignette_fragment_shader =
  "uniform float size;\n"
  "uniform float rx;\n"
  "uniform float ry;\n"

  "uniform vec3 color;\n"

  "uniform sampler2D tDiffuse;\n"
  "uniform sampler2D tHex;\n"

  "varying vec2 vUv;\n"

  "void main() {\n"

  "  vec4 vcolor = vec4(color,1.0);\n"

  "  vec2 hexuv;\n"
  "  hexuv.x = mod(vUv.x * rx, size) / size;\n"
  "  hexuv.y = mod(vUv.y * ry, size) / size;\n"
  "  vec4 hex = texture2D( tHex, hexuv );\n"

  "  float tolerance = 0.2;\n"
  "  float vignette_size = 0.6;\n"
  "  vec2 powers = pow(abs(vec2(vUv.x - 0.5,vUv.y - 0.5)),vec2(2.0));\n"
  "  float radiusSqrd = vignette_size*vignette_size;\n"
  "  float gradient = smoothstep(radiusSqrd-tolerance, radiusSqrd+tolerance, powers.x+powers.y);\n"

  "  vec2 uv = ( vUv - vec2( 0.5 ) );\n"

  "  vec2 sample = uv * gradient * 0.5 * (1.0-hex.r);\n"

  "  vec4 texel = texture2D( tDiffuse, vUv-sample );\n"
  "  gl_FragColor = (((1.0-hex.r)*vcolor) * 0.5 * gradient) + vec4( mix( texel.rgb, vcolor.xyz*0.7, dot( uv, uv ) ), texel.a );\n"

  "}";

static float rx_def = 1024;
static float ry_def = 768;
static float size_def = 512;
static float color_def[3] = { 0.27058823529411763, 0.5411764705882353, 0.6941176470588235 };
static GthreeUniformsDefinition hexvignette_uniforms_defs[] = {
  {"tDiffuse", GTHREE_UNIFORM_TYPE_TEXTURE, NULL },
  {"tHex", GTHREE_UNIFORM_TYPE_TEXTURE, NULL },
  {"size", GTHREE_UNIFORM_TYPE_FLOAT, &size_def},
  {"rx", GTHREE_UNIFORM_TYPE_FLOAT, &rx_def},
  {"ry", GTHREE_UNIFORM_TYPE_FLOAT, &ry_def},
  {"color", GTHREE_UNIFORM_TYPE_VECTOR3, &color_def },
};
static GthreeUniforms *hexvignette_uniforms;
static GthreeShader *hexvignette_shader;

GthreeShader * hexvignette_shader_clone (void)
{
  if (hexvignette_shader == NULL)
    {
      hexvignette_uniforms = gthree_uniforms_new_from_definitions (hexvignette_uniforms_defs, G_N_ELEMENTS (hexvignette_uniforms_defs));
      hexvignette_shader = gthree_shader_new (NULL, hexvignette_uniforms,
                                              hexvignette_vertex_shader,
                                              hexvignette_fragment_shader);
    }

  return gthree_shader_clone (hexvignette_shader);
}
