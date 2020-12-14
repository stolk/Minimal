
static const char* source_vsh_Dof =
"#version 300 es\n"
"in mediump vec2 position;\n"
"in mediump vec2 uv;\n"
"out mediump vec2 tmapcoord;\n"
"void main()\n"
"{\n"
"    gl_Position.xy = position;\n"
"    gl_Position.z = 1.0;\n"
"    gl_Position.w = 1.0;\n"
"    tmapcoord = uv;\n"
"}\n";


static const char* source_fsh_Dof =
"#version 300 es\n"
"uniform mediump sampler2D depthmap;\n"
"uniform mediump sampler2D colormap;\n"
"uniform mediump sampler2D blurmap;\n"
"uniform mediump float focaldist;\n"
"uniform mediump float fgndblur;\n"
"uniform mediump float bgndblur;\n"
"in mediump vec2 tmapcoord;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    mediump float depth = texture( depthmap, tmapcoord ).x;\n"
"    lowp vec4 s0 = texture( colormap, tmapcoord );\n"
"    lowp vec4 s1 = texture( blurmap,  tmapcoord );\n"
"    mediump float coc = depth > focaldist ? \n"
"        bgndblur * ( depth - focaldist ) : \n"
"        fgndblur * ( focaldist - depth);\n"
"    coc = clamp( coc, 0.0, 1.0 );\n"
"    fragColor = mix( s0, s1, coc );\n"
"}\n";


static const char* source_vsh_Subsample = source_vsh_Dof;


static const char* source_fsh_Subsample =
"#version 300 es\n"
"uniform mediump sampler2D colormap;\n"
"in mediump vec2 tmapcoord;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    fragColor = texture( colormap, tmapcoord );\n"
"}\n";


static const char* source_vsh_Blur =
"#version 300 es\n"
"uniform mediump vec2 pixshift;\n"
"in mediump vec2 position;\n"
"in mediump vec2 uv;\n"
"out mediump vec2 tmapcoord0;\n"
"out mediump vec2 tmapcoord1;\n"
"out mediump vec2 tmapcoord2;\n"
"out mediump vec2 tmapcoord3;\n"
"out mediump vec2 tmapcoord4;\n"
"out mediump vec2 tmapcoord5;\n"
"out mediump vec2 tmapcoord6;\n"
"void main()\n"
"{\n"
"    gl_Position.xy = position;\n"
"    gl_Position.z = 1.0;\n"
"    gl_Position.w = 1.0;\n"
"    tmapcoord0 = uv + -3.0 * pixshift;\n"
"    tmapcoord1 = uv + -2.0 * pixshift;\n"
"    tmapcoord2 = uv + -1.0 * pixshift;\n"
"    tmapcoord3 = uv;\n"
"    tmapcoord4 = uv +  1.0 * pixshift;\n"
"    tmapcoord5 = uv +  2.0 * pixshift;\n"
"    tmapcoord6 = uv +  3.0 * pixshift;\n"
"}\n";


static const char* source_fsh_Blur =
"#version 300 es\n"
"uniform mediump sampler2D colormap;\n"
"in mediump vec2 tmapcoord0;\n"
"in mediump vec2 tmapcoord1;\n"
"in mediump vec2 tmapcoord2;\n"
"in mediump vec2 tmapcoord3;\n"
"in mediump vec2 tmapcoord4;\n"
"in mediump vec2 tmapcoord5;\n"
"in mediump vec2 tmapcoord6;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    lowp vec4 c0 = texture( colormap, tmapcoord0 );\n"
"    lowp vec4 c1 = texture( colormap, tmapcoord1 );\n"
"    lowp vec4 c2 = texture( colormap, tmapcoord2 );\n"
"    lowp vec4 c3 = texture( colormap, tmapcoord3 );\n"
"    lowp vec4 c4 = texture( colormap, tmapcoord4 );\n"
"    lowp vec4 c5 = texture( colormap, tmapcoord5 );\n"
"    lowp vec4 c6 = texture( colormap, tmapcoord6 );\n"
"    lowp float coc0 = -1.0 + 2.0 * c0.a;\n"
"    lowp float coc1 = -1.0 + 2.0 * c1.a;\n"
"    lowp float coc2 = -1.0 + 2.0 * c2.a;\n"
"    lowp float coc3 = -1.0 + 2.0 * c3.a;\n"
"    lowp float coc4 = -1.0 + 2.0 * c4.a;\n"
"    lowp float coc5 = -1.0 + 2.0 * c5.a;\n"
"    lowp float coc6 = -1.0 + 2.0 * c6.a;\n"
"    lowp float w0 = 0.10;\n"
"    lowp float w1 = 0.14;\n"
"    lowp float w2 = 0.16;\n"
"    lowp float w3 = 0.20;\n"
"    lowp float w4 = 0.16;\n"
"    lowp float w5 = 0.14;\n"
"    lowp float w6 = 0.10;\n"
"    if ( c0.a <= c3.a+0.1 ) w0 = w0 * abs(coc0);\n"
"    if ( c1.a <= c3.a+0.1 ) w1 = w1 * abs(coc1);\n"
"    if ( c2.a <= c3.a+0.1 ) w2 = w2 * abs(coc2);\n"
"                        w3 = w3 * abs(coc3);\n"
"    if ( c4.a <= c3.a+0.1 ) w4 = w4 * abs(coc4);\n"
"    if ( c5.a <= c3.a+0.1 ) w5 = w5 * abs(coc5);\n"
"    if ( c6.a <= c3.a+0.1 ) w6 = w6 * abs(coc6);\n"
"    fragColor.xyz = w0 * c0.xyz + w1 * c1.xyz + w2 * c2.xyz + w3 * c3.xyz + w4 * c4.xyz + w5 * c5.xyz + w6 * c6.xyz;\n"
"    mediump float totalw = 1.0 / ( w0+w1+w2+w3+w4+w5+w6 );\n"
"    fragColor.xyz = totalw * fragColor.xyz;\n"
"    fragColor.a = c3.a;\n"
"}\n";


static const char* source_vsh_Coc = source_vsh_Dof;

static const char* source_fsh_Coc =
"#version 300 es\n"
"uniform mediump sampler2D colormap;\n"
"uniform mediump sampler2D depthmap;\n"
"uniform mediump float focaldist;\n"
"uniform mediump float fgndblur;\n"
"uniform mediump float bgndblur;\n"
"in mediump vec2 tmapcoord;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    lowp vec4 s = texture( colormap, tmapcoord );\n"
"    mediump float depth = texture( depthmap, tmapcoord ).x;\n"
"    mediump float v = depth > focaldist ? \n"
"        fgndblur * ( depth - focaldist ) :\n"
"        bgndblur * ( depth - focaldist );\n"
"    lowp float coc = clamp( v, -1.0, 1.0 );\n"
"    s.a = 0.5 + 0.5 * (coc);\n"
"    fragColor = s;\n"
"}\n";

