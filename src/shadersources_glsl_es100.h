
static const char* source_vsh_Main =
"#version 100\n"
"attribute mediump vec4 position;\n"
"attribute mediump vec4 surfacenormal;\n"
"attribute mediump vec4 rgb;\n"
"uniform highp mat4 modelcamviewprojmat;\n"
"uniform highp mat4 modellightviewprojmat;\n"
"uniform highp mat4 modellightviewmat;\n"
"uniform mediump vec4 basecolour;\n"
"uniform mediump float fogintensity;\n"
"varying mediump vec4 shadowcoord;\n"
"varying mediump vec2 shadowcoordcentered;\n"
"varying mediump float attenuation;\n"
"varying lowp vec4 lightcontrib;\n"
"varying lowp vec4 colour;\n"
"const lowp float lightpower = 0.6;\n"
"void main()\n"
"{\n"
"	colour = basecolour * rgb;\n"
"	gl_Position = modelcamviewprojmat * position;\n"
"	attenuation = min( fogintensity * log( max( 1.0, gl_Position.z*gl_Position.z ) ), 1.0 );\n"
"	attenuation = attenuation*attenuation;\n"
"       shadowcoord = modellightviewprojmat * position;\n"
"       shadowcoordcentered = shadowcoord.xy - vec2( 0.5, 0.5 );\n"
"	mediump vec4 sn = surfacenormal;\n"
"	sn.w = 0.0;\n"
"	highp vec4 transformedsurfacenormal = modellightviewmat * sn;\n"
"	highp vec4 litpoint = modellightviewmat * position;\n"
"	highp vec4 tolight  = -normalize( litpoint );\n"
"	highp float incidencescale = dot( tolight.xyz, transformedsurfacenormal.xyz );\n"
"	highp float lightvisibility = min( incidencescale, transformedsurfacenormal.z );\n"
"	highp float scl = max( lightpower * lightvisibility, 0.0 );\n"
"	lightcontrib = vec4( scl, scl, scl, 0.0 );\n"
"}\n";


static const char* source_fsh_Main =
"#version 100\n"
"precision mediump float;\n"
"precision mediump int;\n"
"uniform mediump sampler2D shadowmap;\n"
"varying lowp vec4 colour;\n"
"varying mediump vec4 shadowcoord;\n"
"varying mediump vec2 shadowcoordcentered;\n"
"varying lowp vec4 lightcontrib; // goes from 0,0,0,1 to 1,1,1,1\n"
"varying mediump float attenuation;\n"
"const mediump vec2 poshalf2 = vec2(  0.5,  0.5 );\n"
"const lowp vec4 illum_lo = vec4( 0.5, 0.5, 0.5, 1.0 );\n"
"const lowp vec4 white = vec4( 0.8, 0.8, 0.8, 1.0 );\n"
"const lowp vec4 yello = vec4( 0.6, 0.6, 0.5, 1.0 );\n"
"const lowp vec4 halfl = vec4( 0.5, 0.5, 0.5, 1.0 );\n"
"void main()\n"
"{\n"
"    lowp vec2 inview  = step( abs( shadowcoordcentered ), poshalf2 ); // 0,0 or 0,1 or 1,0 or 1,1\n"
"    mediump float depth = texture2D( shadowmap, shadowcoord.xy ).x;\n"
"    mediump float fragmentdepth = shadowcoord.z / shadowcoord.w;\n"
"    lowp float darkening = inview.x * inview.y * step( depth, fragmentdepth ); // 0,0,0,0 or 1,1,1,1\n"
"    gl_FragColor = ( illum_lo + ( 1.0 - darkening ) * lightcontrib ) * colour;\n"
"    gl_FragColor = mix( gl_FragColor, yello, attenuation );\n"
"    //gl_FragColor = vec4( 0.0, depth, 0.5, 1.0 );\n"
"}\n";

static const char* source_vsh_MainNS =
"#version 100\n"
"attribute mediump vec4 position;\n"
"attribute mediump vec4 surfacenormal;\n"
"attribute mediump vec4 rgb;\n"
"uniform highp mat4 modelcamviewprojmat;\n"
"uniform highp mat4 modellightviewprojmat;\n"
"uniform highp mat4 modellightviewmat;\n"
"uniform mediump vec4 basecolour;\n"
"uniform mediump float fogintensity;\n"
"varying mediump float attenuation;\n"
"varying lowp vec4 lightcontrib;\n"
"varying lowp vec4 colour;\n"
"const lowp float lightpower = 0.6;\n"
"void main()\n"
"{\n"
"	colour = basecolour * rgb;\n"
"	gl_Position = modelcamviewprojmat * position;\n"
"	attenuation = min( fogintensity * log( max( 1.0, gl_Position.z*gl_Position.z ) ), 1.0 );\n"
"	attenuation = attenuation*attenuation;\n"
"	mediump vec4 sn = surfacenormal;\n"
"	sn.w = 0.0;\n"
"	highp vec4 transformedsurfacenormal = modellightviewmat * sn;\n"
"	highp vec4 litpoint = modellightviewmat * position;\n"
"	highp vec4 tolight  = -normalize( litpoint );\n"
"	highp float incidencescale = dot( tolight.xyz, transformedsurfacenormal.xyz );\n"
"	highp float lightvisibility = min( incidencescale, transformedsurfacenormal.z );\n"
"	highp float scl = max( lightpower * lightvisibility, 0.0 );\n"
"	lightcontrib = vec4( scl, scl, scl, 0.0 );\n"
"}\n";


static const char* source_vsh_Edge =
"#version 100\n"
"attribute mediump vec4 position;\n"
"uniform highp mat4 modelcamviewprojmat;\n"
"uniform mediump float fogintensity;\n"
"varying mediump float attenuation;\n"
"void main()\n"
"{\n"
"	gl_Position = modelcamviewprojmat * position;\n"
"	attenuation = min( fogintensity * log( max( 1.0, gl_Position.z*gl_Position.z ) ), 1.0 );\n"
"	attenuation = attenuation*attenuation;\n"
"}\n";


static const char* source_fsh_Edge =
"#version 100\n"
"const lowp vec4 yello = vec4( 0.6, 0.6, 0.5, 1.0 );\n"
"uniform lowp vec4 linecolour;\n"
"varying mediump float attenuation;\n"
"void main()\n"
"{\n"
"    gl_FragColor = linecolour;\n"
"    gl_FragColor = mix( gl_FragColor, yello, attenuation );\n"
"}\n";


static const char* source_vsh_Font =
"#version 100\n"
"attribute mediump vec2 position;\n"
"varying lowp vec4 clr;\n"
"uniform mediump vec2 rotx;\n"
"uniform mediump vec2 roty;\n"
"uniform mediump vec2 translation;\n"
"uniform lowp vec4 colour;\n"
"uniform mediump float fadedist;\n"
"const lowp vec4 nul = vec4( 0.8, 0.8, 0.7, 1.0 );\n"
"void main()\n"
"{\n"
"    gl_Position.x = dot( position, rotx ) + translation.x;\n"
"    gl_Position.y = dot( position, roty ) + translation.y;\n"
"    gl_Position.z = 1.0;\n"
"    gl_Position.w = 1.0;\n"
"    clr = colour;\n"
"    if ( abs( gl_Position.x ) > fadedist || abs( gl_Position.y ) > fadedist )\n"
"        clr = nul;\n"
"}\n";


static const char* source_fsh_Font =
"#version 100\n"
"varying  lowp vec4 clr;\n"
"void main()\n"
"{\n"
"    gl_FragColor = clr;\n"
"}\n";


static const char* source_vsh_Shadow =
"#version 100\n"
"attribute mediump vec4 position;\n"
"uniform highp mat4 modellightviewprojmat;\n"
"void main()\n"
"{\n"
"    gl_Position = modellightviewprojmat * position;\n"
"}\n";


static const char* source_fsh_Shadow =
"#version 100\n"
"const lowp vec4 wht = vec4( 1.0, 1.0, 1.0, 1.0 );\n"
"void main()\n"
"{\n"
"    gl_FragColor = gl_FragCoord.zzzz;\n"
"    //gl_FragDepth = gl_FragCoord.z;\n"
"}\n";


static const char* source_vsh_Hud =
"#version 100\n"
"attribute mediump vec2 position;\n"
"attribute mediump vec2 uv;\n"
"uniform mediump vec2 rotx;\n"
"uniform mediump vec2 roty;\n"
"uniform mediump vec2 translation;\n"
"varying mediump vec2  tmapcoord;\n"
"void main()\n"
"{\n"
"    gl_Position.x = dot( position, rotx ) + translation.x;\n"
"    gl_Position.y = dot( position, roty ) + translation.y;\n"
"    gl_Position.z = 1.0;\n"
"    gl_Position.w = 1.0;\n"
"    tmapcoord = uv;\n"
"}\n";


static const char* source_fsh_Hud =
"#version 100\n"
"uniform mediump sampler2D texturemap;\n"
"varying mediump vec2 tmapcoord;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D( texturemap, tmapcoord );\n"
"}\n";


