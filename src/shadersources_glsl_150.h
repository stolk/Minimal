static const char* source_vsh_Main =
"#version 150\n"
"in mediump vec4 position;\n"
"in mediump vec4 surfacenormal;\n"
"in mediump vec4 rgb;\n"
"uniform highp mat4 modelcamviewprojmat;\n"
"uniform highp mat4 modellightviewprojmat;\n"
"uniform highp mat4 modellightviewmat;\n"
"uniform mediump vec4 basecolour;\n"
"uniform mediump float fogintensity;\n"
"out mediump vec4 shadowcoord;\n"
"out mediump vec2 shadowcoordcentered;\n"
"out mediump float attenuation;\n"
"out mediump vec4 transformedsurfacenormal;\n"
"out mediump vec4 tolight;\n"
"out lowp vec4 colour;\n"
"void main()\n"
"{\n"
"	colour = basecolour * rgb;\n"
"	gl_Position = modelcamviewprojmat * position;\n"
"	attenuation = min( fogintensity * log( max( 1.0, gl_Position.z*gl_Position.z ) ), 1.0 );\n"
"	attenuation = attenuation * attenuation;\n"
"       shadowcoord = modellightviewprojmat * position;\n"
"       shadowcoordcentered = shadowcoord.xy - vec2( 0.5, 0.5 );\n"
"	tolight = -normalize( modellightviewmat * position );\n"
"	mediump vec4 sn = surfacenormal;\n"
"	sn.w = 0.0;\n"
"	transformedsurfacenormal = modellightviewmat * sn;\n"
"}\n";


static const char* source_fsh_Main =
"#version 150\n"
"precision mediump float;\n"
"precision mediump int;\n"
"uniform mediump sampler2D shadowmap;\n"
"in lowp vec4 colour;\n"
"in mediump vec4 shadowcoord;\n"
"in mediump vec2 shadowcoordcentered;\n"
"in mediump vec4 transformedsurfacenormal;\n"
"in mediump vec4 tolight;\n"
"in mediump float attenuation;\n"
"out lowp vec4 fragColor;\n"
"\n"
"const mediump vec2 poshalf2 = vec2(  0.5,  0.5 );\n"
"const lowp float lightpower = 1.0;\n"
"const lowp vec4 illum_lo = vec4( 0.2, 0.2, 0.2, 1.0 );\n"
"const lowp vec4 white = vec4( 0.8, 0.8, 0.8, 1.0 );\n"
"const lowp vec4 fogcl = vec4( 0.8, 0.8, 0.8, 1.0 );\n"
"const lowp vec4 halfl = vec4( 0.5, 0.5, 0.5, 1.0 );\n"
"\n"
"void main()\n"
"{\n"
"	mediump float incidencescale = dot( tolight.xyz, transformedsurfacenormal.xyz );\n"
"	mediump float lightvisibility = min( incidencescale, transformedsurfacenormal.z );\n"
"	mediump float scl = max( lightpower * lightvisibility, 0.0 ); //  was *lightvisibility\n"
"	mediump vec4 lightcontrib = vec4( scl, scl, scl, 0.0 ); // goes from 0,0,0,0 to 1,1,1,0\n"
"	lowp vec2 inview  = step( abs( shadowcoordcentered ), poshalf2 );\n"
"	mediump float depth = texture( shadowmap, shadowcoord.xy ).x;\n"
"	mediump float fragmentdepth = shadowcoord.z / shadowcoord.w;\n"
"	lowp float darkening = inview.x * inview.y * step( depth, fragmentdepth ); // 0,0,0,0 or 1,1,1,1\n"
"	mediump float spec = pow( scl, 18 );\n"
"	lowp vec4 specColor = vec4( spec, spec, spec, 0.0f ) * 0.6;\n"
"	fragColor = ( illum_lo + ( 1.0 - darkening ) * lightcontrib ) * ( colour + specColor );\n"
"	fragColor = mix( fragColor, fogcl, attenuation );\n"
"}\n";


static const char* source_vsh_Edge =
"#version 150\n"
"in mediump vec4 position;\n"
"uniform highp mat4 modelcamviewprojmat;\n"
"uniform mediump float fogintensity;\n"
"out mediump float attenuation;\n"
"void main()\n"
"{\n"
"	gl_Position = modelcamviewprojmat * position;\n"
"	attenuation = min( fogintensity * log( max( 1.0, gl_Position.z*gl_Position.z ) ), 1.0 );\n"
"	attenuation = attenuation * attenuation;\n"
"}\n";


static const char* source_fsh_Edge =
"#version 150\n"
"const lowp vec4 fogcl = vec4( 0.8, 0.8, 0.8, 1.0 );\n"
"uniform lowp vec4 linecolour;\n"
"in mediump float attenuation;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    fragColor = linecolour;\n"
"    fragColor = mix( fragColor, fogcl, attenuation );\n"
"}\n";


static const char* source_vsh_Font = 
"#version 150\n"
"in mediump vec2 position;\n"
"out lowp vec4 clr;\n"
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
"#version 150\n"
"in  lowp vec4 clr;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    fragColor = clr;\n"
"}\n";


static const char* source_vsh_Shadow =
"#version 150\n"
"in mediump vec4 position;\n"
"uniform highp mat4 modellightviewprojmat;\n"
"void main()\n"
"{\n"
"    gl_Position = modellightviewprojmat * position;\n"
"}\n";


static const char* source_fsh_Shadow =
"#version 150\n"
"out vec4 fragColor;\n"
"const lowp vec4 wht = vec4( 1.0, 1.0, 1.0, 1.0 );\n"
"void main()\n"
"{\n"
"    //fragColor = gl_FragCoord.zzzz;\n"
"    gl_FragDepth = gl_FragCoord.z;\n"
"}\n";


static const char* source_vsh_Hud =
"#version 150\n"
"in mediump vec2 position;\n"
"in mediump vec2 uv;\n"
"uniform mediump vec2 rotx;\n"
"uniform mediump vec2 roty;\n"
"uniform mediump vec2 translation;\n"
"out mediump vec2  tmapcoord;\n"
"void main()\n"
"{\n"
"    gl_Position.x = dot( position, rotx ) + translation.x;\n"
"    gl_Position.y = dot( position, roty ) + translation.y;\n"
"    gl_Position.z = 1.0;\n"
"    gl_Position.w = 1.0;\n"
"    tmapcoord = uv;\n"
"}\n";


static const char* source_fsh_Hud =
"#version 150\n"
"uniform mediump sampler2D texturemap;\n"
"in mediump vec2 tmapcoord;\n"
"out lowp vec4 fragColor;\n"
"void main()\n"
"{\n"
"    fragColor = texture( texturemap, tmapcoord.xy );\n"
"}\n";

