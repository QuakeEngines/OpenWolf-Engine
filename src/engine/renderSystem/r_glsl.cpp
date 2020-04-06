////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2006 - 2008 Robert Beckebans <trebor_7@users.sourceforge.net>
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   r_glsl.cpp
// Version:     v1.00
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.h>

idRenderSystemGLSLLocal renderSystemGLSLLocal;

/*
===============
idRenderSystemGLSLLocal::idRenderSystemGLSLLocal
===============
*/
idRenderSystemGLSLLocal::idRenderSystemGLSLLocal( void )
{
}

/*
===============
idRenderSystemGLSLLocal::~idRenderSystemGLSLLocal
===============
*/
idRenderSystemGLSLLocal::~idRenderSystemGLSLLocal( void )
{
}

void idRenderSystemGLSLLocal::PrintLog( U32 programOrShader, glslPrintLog_t type, bool developerOnly )
{
    S32 maxLength = 0, i, printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;
    UTF8* msg;
    static UTF8 msgPart[1024];
    
    switch ( type )
    {
        case GLSL_PRINTLOG_PROGRAM_INFO:
            clientMainSystem->RefPrintf( printLevel, "Program info log:\n" );
            qglGetProgramiv( programOrShader, GL_INFO_LOG_LENGTH, &maxLength );
            break;
            
        case GLSL_PRINTLOG_SHADER_INFO:
            clientMainSystem->RefPrintf( printLevel, "Shader info log:\n" );
            qglGetShaderiv( programOrShader, GL_INFO_LOG_LENGTH, &maxLength );
            break;
            
        case GLSL_PRINTLOG_SHADER_SOURCE:
            clientMainSystem->RefPrintf( printLevel, "Shader source:\n" );
            qglGetShaderiv( programOrShader, GL_SHADER_SOURCE_LENGTH, &maxLength );
            break;
    }
    
    if ( maxLength <= 0 )
    {
        clientMainSystem->RefPrintf( printLevel, "None.\n" );
        return;
    }
    
    if ( maxLength < 1023 )
    {
        msg = msgPart;
    }
    else
    {
        msg = reinterpret_cast<UTF8*>( memorySystem->Malloc( maxLength ) );
    }
    
    switch ( type )
    {
        case GLSL_PRINTLOG_PROGRAM_INFO:
            qglGetProgramInfoLog( programOrShader, maxLength, &maxLength, msg );
            break;
            
        case GLSL_PRINTLOG_SHADER_INFO:
            qglGetShaderInfoLog( programOrShader, maxLength, &maxLength, msg );
            break;
            
        case GLSL_PRINTLOG_SHADER_SOURCE:
            qglGetShaderSource( programOrShader, maxLength, &maxLength, msg );
            break;
    }
    
    if ( maxLength < 1023 )
    {
        msgPart[maxLength + 1] = '\0';
        
        clientMainSystem->RefPrintf( printLevel, "%s\n", msgPart );
    }
    else
    {
        for ( i = 0; i < maxLength; i += 1023 )
        {
            Q_strncpyz( msgPart, msg + i, sizeof( msgPart ) );
            
            clientMainSystem->RefPrintf( printLevel, "%s", msgPart );
        }
        
        clientMainSystem->RefPrintf( printLevel, "\n" );
        
        memorySystem->Free( msg );
    }
    
}

void idRenderSystemGLSLLocal::GetShaderHeader( GLenum shaderType, StringEntry extra, UTF8* dest, S32 size )
{
    F32 fbufWidthScale, fbufHeightScale;
    
    dest[0] = '\0';
    
    // HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones
    if ( glRefConfig.glslMajorVersion > 1 || ( glRefConfig.glslMajorVersion == 1 && glRefConfig.glslMinorVersion >= 30 ) )
    {
        Q_strcat( dest, size, "#version 130\n" );
        
        if ( shaderType == GL_VERTEX_SHADER )
        {
            Q_strcat( dest, size, "#define attribute in\n" );
            Q_strcat( dest, size, "#define varying out\n" );
        }
        else
        {
            Q_strcat( dest, size, "#define varying in\n" );
            
            Q_strcat( dest, size, "out vec4 out_Color;\n" );
            Q_strcat( dest, size, "#define gl_FragColor out_Color\n" );
            Q_strcat( dest, size, "#define texture2D texture\n" );
            Q_strcat( dest, size, "#define shadow2D texture\n" );
        }
    }
    else
    {
        Q_strcat( dest, size, "#version 120\n" );
        Q_strcat( dest, size, "#define shadow2D(a,b) shadow2D(a,b).r \n" );
    }
    
    Q_strcat( dest, size, "#ifndef M_PI\n#define M_PI 3.14159265358979323846\n#endif\n" );
    
    Q_strcat( dest, size,
              "#ifndef deformGen_t\n"
              "#define deformGen_t\n"
              "#define DGEN_WAVE_SIN %i\n"
              "#define DGEN_WAVE_SQUARE %i\n"
              "#define DGEN_WAVE_TRIANGLE %i\n"
              "#define DGEN_WAVE_SAWTOOTH %i\n"
              "#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
              "#define DGEN_BULGE %i\n"
              "#define DGEN_MOVE %i\n"
              "#endif\n",
              DGEN_WAVE_SIN,
              DGEN_WAVE_SQUARE,
              DGEN_WAVE_TRIANGLE,
              DGEN_WAVE_SAWTOOTH,
              DGEN_WAVE_INVERSE_SAWTOOTH,
              DGEN_BULGE,
              DGEN_MOVE );
              
    Q_strcat( dest, size,
              "#ifndef tcGen_t\n"
              "#define tcGen_t\n"
              "#define TCGEN_LIGHTMAP %i\n"
              "#define TCGEN_TEXTURE %i\n"
              "#define TCGEN_ENVIRONMENT_MAPPED %i\n"
              "#define TCGEN_FOG %i\n"
              "#define TCGEN_VECTOR %i\n"
              "#endif\n",
              TCGEN_LIGHTMAP,
              TCGEN_TEXTURE,
              TCGEN_ENVIRONMENT_MAPPED,
              TCGEN_FOG,
              TCGEN_VECTOR );
              
    Q_strcat( dest, size,
              "#ifndef colorGen_t\n"
              "#define colorGen_t\n"
              "#define CGEN_LIGHTING_DIFFUSE %i\n"
              "#endif\n",
              CGEN_LIGHTING_DIFFUSE );
              
    Q_strcat( dest, size,
              "#ifndef alphaGen_t\n"
              "#define alphaGen_t\n"
              "#define AGEN_LIGHTING_SPECULAR %i\n"
              "#define AGEN_PORTAL %i\n"
              "#endif\n",
              AGEN_LIGHTING_SPECULAR,
              AGEN_PORTAL );
              
    Q_strcat( dest, size,
              "#ifndef texenv_t\n"
              "#define texenv_t\n"
              "#define TEXENV_MODULATE %i\n"
              "#define TEXENV_ADD %i\n"
              "#define TEXENV_REPLACE %i\n"
              "#endif\n",
              GL_MODULATE,
              GL_ADD,
              GL_REPLACE );
              
    fbufWidthScale = 1.0f / ( ( F32 )glConfig.vidWidth );
    fbufHeightScale = 1.0f / ( ( F32 )glConfig.vidHeight );
    Q_strcat( dest, size, "#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale );
    
    if ( r_pbr->integer )
    {
        Q_strcat( dest, size, "#define USE_PBR\n" );
    }
    
    if ( r_cubeMapping->integer )
    {
        //copy in tr_backend for prefiltering the mipmaps
        S32 cubeMipSize = r_cubemapSize->integer;
        S32 numRoughnessMips = 0;
        
        while ( cubeMipSize )
        {
            cubeMipSize >>= 1;
            numRoughnessMips++;
        }
        numRoughnessMips = MAX( 1, numRoughnessMips - 2 );
        
        Q_strcat( dest, size, "#define ROUGHNESS_MIPS float(%i)\n", numRoughnessMips - 4 );
    }
    
    if ( r_horizonFade->integer )
    {
        F32 fade = ( F32 )( 1 + ( 0.1f * r_horizonFade->integer ) );
        Q_strcat( dest, size, "#define HORIZON_FADE float(%f)\n", fade );
    }
    
    if ( extra )
    {
        Q_strcat( dest, size, extra );
    }
    
    // OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
    // so we have to reset the line counting
    Q_strcat( dest, size, "#line 0\n" );
}

S32 idRenderSystemGLSLLocal::CompileGPUShader( U32 program, U32* prevShader, StringEntry buffer, S32 size, U32 shaderType )
{
    S32 compiled;
    U32 shader;
    
    shader = qglCreateShader( shaderType );
    
    qglShaderSource( shader, 1, ( StringEntry* )&buffer, &size );
    
    // compile shader
    qglCompileShader( shader );
    
    // check if shader compiled
    qglGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
    if ( !compiled )
    {
        PrintLog( shader, GLSL_PRINTLOG_SHADER_SOURCE, false );
        PrintLog( shader, GLSL_PRINTLOG_SHADER_INFO, false );
        Com_Error( ERR_DROP, "Couldn't compile shader" );
        return 0;
    }
    
    if ( *prevShader )
    {
        qglDetachShader( program, *prevShader );
        qglDeleteShader( *prevShader );
    }
    
    // attach shader to program
    qglAttachShader( program, shader );
    
    *prevShader = shader;
    
    return 1;
}

S32 idRenderSystemGLSLLocal::LoadGPUShaderText( StringEntry name, U32 shaderType, UTF8* dest, S32 destSize )
{
    S32 size, result;
    UTF8 filename[MAX_QPATH], * buffer = nullptr;
    StringEntry shaderText = nullptr;
    
    if ( shaderType == GL_VERTEX_SHADER )
    {
        Q_snprintf( filename, sizeof( filename ), "renderProgs/%s.vertex", name );
    }
    else
    {
        Q_snprintf( filename, sizeof( filename ), "renderProgs/%s.fragment", name );
    }
    
    size = fileSystem->ReadFile( filename, ( void** )&buffer );
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
    shaderText = buffer;
    size += 1;
    
    if ( size > destSize )
    {
        result = 0;
    }
    else
    {
        Q_strncpyz( dest, shaderText, size + 1 );
        result = 1;
    }
    
    if ( buffer )
    {
        fileSystem->FreeFile( buffer );
    }
    
    return result;
}

void idRenderSystemGLSLLocal::LinkProgram( U32 program )
{
    S32 linked;
    
    qglLinkProgram( program );
    
    qglGetProgramiv( program, GL_LINK_STATUS, &linked );
    if ( !linked )
    {
        PrintLog( program, GLSL_PRINTLOG_PROGRAM_INFO, false );
        Com_Error( ERR_DROP, "shaders failed to link" );
    }
}

void idRenderSystemGLSLLocal::ValidateProgram( U32 program )
{
    S32 validated;
    
    qglValidateProgram( program );
    
    qglGetProgramiv( program, GL_VALIDATE_STATUS, &validated );
    if ( !validated )
    {
        PrintLog( program, GLSL_PRINTLOG_PROGRAM_INFO, false );
        Com_Error( ERR_DROP, "shaders failed to validate" );
    }
}

void idRenderSystemGLSLLocal::ShowProgramUniforms( U32 program )
{
    S32 i, count, size;
    U32 type;
    UTF8 uniformName[1000];
    
    // query the number of active uniforms
    qglGetProgramiv( program, GL_ACTIVE_UNIFORMS, &count );
    
    // Loop over each of the active uniforms, and set their value
    for ( i = 0; i < count; i++ )
    {
        qglGetActiveUniform( program, i, sizeof( uniformName ), NULL, &size, &type, uniformName );
        
        clientMainSystem->RefPrintf( PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName );
    }
}

S32 idRenderSystemGLSLLocal::InitGPUShader2( shaderProgram_t* program, StringEntry name, S32 attribs, StringEntry vpCode, StringEntry fpCode )
{
    clientMainSystem->RefPrintf( PRINT_DEVELOPER, "------- idRenderSystemGLSLLocal::InitGPUShader2 -------\n" );
    
    if ( ::strlen( name ) >= MAX_QPATH )
    {
        Com_Error( ERR_DROP, "idRenderSystemGLSLLocal::InitGPUShader2: \"%s\" is too long", name );
    }
    
    Q_strncpyz( program->name, name, sizeof( program->name ) );
    
    program->program = qglCreateProgram();
    program->attribs = attribs;
    
    if ( !( CompileGPUShader( program->program, &program->vertexShader, vpCode, ( S32 )::strlen( vpCode ), GL_VERTEX_SHADER ) ) )
    {
        clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemGLSLLocal::InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER\n", name );
        qglDeleteProgram( program->program );
        return 0;
    }
    
    if ( fpCode )
    {
        if ( !( CompileGPUShader( program->program, &program->fragmentShader, fpCode, ( S32 )::strlen( fpCode ), GL_FRAGMENT_SHADER ) ) )
        {
            clientMainSystem->RefPrintf( PRINT_ALL, "idRenderSystemGLSLLocal::InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER\n", name );
            qglDeleteProgram( program->program );
            return 0;
        }
    }
    
    if ( attribs & ATTR_POSITION )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_POSITION, "attr_Position" );
    }
    
    if ( attribs & ATTR_TEXCOORD )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_TEXCOORD, "attr_TexCoord0" );
    }
    
    if ( attribs & ATTR_LIGHTCOORD )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_LIGHTCOORD, "attr_TexCoord1" );
    }
    
    if ( attribs & ATTR_TANGENT )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_TANGENT, "attr_Tangent" );
    }
    
    if ( attribs & ATTR_NORMAL )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_NORMAL, "attr_Normal" );
    }
    
    if ( attribs & ATTR_COLOR )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_COLOR, "attr_Color" );
    }
    
    if ( attribs & ATTR_PAINTCOLOR )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_PAINTCOLOR, "attr_PaintColor" );
    }
    
    if ( attribs & ATTR_LIGHTDIRECTION )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_LIGHTDIRECTION, "attr_LightDirection" );
    }
    
    if ( attribs & ATTR_BONE_INDEXES )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_BONE_INDEXES, "attr_BoneIndexes" );
    }
    
    if ( attribs & ATTR_BONE_WEIGHTS )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_BONE_WEIGHTS, "attr_BoneWeights" );
    }
    
    if ( attribs & ATTR_POSITION2 )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_POSITION2, "attr_Position2" );
    }
    
    if ( attribs & ATTR_NORMAL2 )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_NORMAL2, "attr_Normal2" );
    }
    
    if ( attribs & ATTR_TANGENT2 )
    {
        qglBindAttribLocation( program->program, ATTR_INDEX_TANGENT2, "attr_Tangent2" );
    }
    
    LinkProgram( program->program );
    
    return 1;
}


S32 idRenderSystemGLSLLocal::InitGPUShader( shaderProgram_t* program, StringEntry name, S32 attribs, bool fragmentShader, StringEntry extra, bool addHeader )
{
    S32 size, result;
    UTF8 vpCode[170000], fpCode[170000], * postHeader;
    
    size = sizeof( vpCode );
    if ( addHeader )
    {
        GetShaderHeader( GL_VERTEX_SHADER, extra, vpCode, size );
        postHeader = &vpCode[strlen( vpCode )];
        size -= ( S32 )::strlen( vpCode );
    }
    else
    {
        postHeader = &vpCode[0];
    }
    
    if ( !LoadGPUShaderText( name, GL_VERTEX_SHADER, postHeader, size ) )
    {
        return 0;
    }
    
    if ( fragmentShader )
    {
        size = sizeof( fpCode );
        if ( addHeader )
        {
            GetShaderHeader( GL_FRAGMENT_SHADER, extra, fpCode, size );
            postHeader = &fpCode[strlen( fpCode )];
            size -= ( S32 )::strlen( fpCode );
        }
        else
        {
            postHeader = &fpCode[0];
        }
        
        if ( !LoadGPUShaderText( name,  GL_FRAGMENT_SHADER, postHeader, size ) )
        {
            return 0;
        }
    }
    
    result = InitGPUShader2( program, name, attribs, vpCode, fragmentShader ? fpCode : NULL );
    
    return result;
}

void idRenderSystemGLSLLocal::InitUniforms( shaderProgram_t* program )
{
    S32 i, size;
    
    S32* uniforms = program->uniforms;
    
    size = 0;
    for ( i = 0; i < UNIFORM_COUNT; i++ )
    {
        uniforms[i] = qglGetUniformLocation( program->program, uniformsInfo[i].name );
        
        if ( uniforms[i] == -1 )
        {
            continue;
        }
        
        program->uniformBufferOffsets[i] = size;
        
        switch ( uniformsInfo[i].type )
        {
            case GLSL_INT:
                size += sizeof( S32 );
                break;
            case GLSL_FLOAT:
                size += sizeof( F32 );
                break;
            case GLSL_FLOAT5:
                size += sizeof( F32 ) * 5;
                break;
            case GLSL_VEC2:
                size += sizeof( F32 ) * 2;
                break;
            case GLSL_VEC3:
                size += sizeof( F32 ) * 3;
                break;
            case GLSL_VEC4:
                size += sizeof( F32 ) * 4;
                break;
            case GLSL_MAT16:
                size += sizeof( F32 ) * 16;
                break;
            case GLSL_MAT16_BONEMATRIX:
                size += sizeof( F32 ) * 16 * glRefConfig.glslMaxAnimatedBones;
                break;
            default:
                break;
        }
    }
    
    program->uniformBuffer = reinterpret_cast< UTF8* >( memorySystem->Malloc( size ) );
}

void idRenderSystemGLSLLocal::FinishGPUShader( shaderProgram_t* program )
{
    ShowProgramUniforms( program->program );
    idRenderSystemInitLocal::CheckErrors( __FILE__, __LINE__ );
}

void idRenderSystemGLSLLocal::SetUniformInt( shaderProgram_t* program, S32 uniformNum, S32 value )
{
    S32* uniforms = program->uniforms;
    S32* compare = ( S32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_INT )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( value == *compare )
    {
        return;
    }
    
    *compare = value;
    
    qglProgramUniform1iEXT( program->program, uniforms[uniformNum], value );
}

void idRenderSystemGLSLLocal::SetUniformFloat( shaderProgram_t* program, S32 uniformNum, F32 value )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_FLOAT )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( value == *compare )
    {
        return;
    }
    
    *compare = value;
    
    qglProgramUniform1fEXT( program->program, uniforms[uniformNum], value );
}

void idRenderSystemGLSLLocal::SetUniformVec2( shaderProgram_t* program, S32 uniformNum, const vec2_t v )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_VEC2 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( v[0] == compare[0] && v[1] == compare[1] )
    {
        return;
    }
    
    compare[0] = v[0];
    compare[1] = v[1];
    
    qglProgramUniform2fEXT( program->program, uniforms[uniformNum], v[0], v[1] );
}

void idRenderSystemGLSLLocal::SetUniformVec3( shaderProgram_t* program, S32 uniformNum, const vec3_t v )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_VEC3 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( VectorCompare( v, compare ) )
    {
        return;
    }
    
    VectorCopy( v, compare );
    
    qglProgramUniform3fEXT( program->program, uniforms[uniformNum], v[0], v[1], v[2] );
}

void idRenderSystemGLSLLocal::SetUniformVec4( shaderProgram_t* program, S32 uniformNum, const vec4_t v )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_VEC4 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( idRenderSystemMathsLocal::VectorCompare4( v, compare ) )
    {
        return;
    }
    
    VectorCopy4( v, compare );
    
    qglProgramUniform4fEXT( program->program, uniforms[uniformNum], v[0], v[1], v[2], v[3] );
}

void idRenderSystemGLSLLocal::SetUniformFloat5( shaderProgram_t* program, S32 uniformNum, const vec5_t v )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_FLOAT5 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( idRenderSystemMathsLocal::VectorCompare5( v, compare ) )
    {
        return;
    }
    
    VectorCopy5( v, compare );
    
    qglProgramUniform1fvEXT( program->program, uniforms[uniformNum], 5, v );
}

void idRenderSystemGLSLLocal::SetUniformMat4( shaderProgram_t* program, S32 uniformNum, const mat4_t matrix )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_MAT16 )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformMat4: wrong type for uniform %i in program %s\n", uniformNum, program->name );
        return;
    }
    
    if ( idRenderSystemMathsLocal::Mat4Compare( matrix, compare ) )
    {
        return;
    }
    
    idRenderSystemMathsLocal::Mat4Copy( matrix, compare );
    
    qglProgramUniformMatrix4fvEXT( program->program, uniforms[uniformNum], 1, GL_FALSE, matrix );
}

void idRenderSystemGLSLLocal::SetUniformMat4BoneMatrix( shaderProgram_t* program, S32 uniformNum, /*const*/ mat4_t* matrix, S32 numMatricies )
{
    S32* uniforms = program->uniforms;
    F32* compare = ( F32* )( program->uniformBuffer + program->uniformBufferOffsets[uniformNum] );
    
    if ( uniforms[uniformNum] == -1 )
    {
        return;
    }
    
    if ( uniformsInfo[uniformNum].type != GLSL_MAT16_BONEMATRIX )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformMat4BoneMatrix: wrong type for uniform %i in program %s\n",
                                     uniformNum, program->name );
        return;
    }
    
    if ( numMatricies > glRefConfig.glslMaxAnimatedBones )
    {
        clientMainSystem->RefPrintf( PRINT_WARNING, "idRenderSystemGLSLLocal::SetUniformMat4BoneMatrix: too many matricies (%d/%d) for uniform %i in program %s\n",
                                     numMatricies, glRefConfig.glslMaxAnimatedBones, uniformNum, program->name );
        return;
    }
    
    if ( !::memcmp( matrix, compare, numMatricies * sizeof( mat4_t ) ) )
    {
        return;
    }
    
    ::memcpy( compare, matrix, numMatricies * sizeof( mat4_t ) );
    
    qglProgramUniformMatrix4fvEXT( program->program, uniforms[uniformNum], numMatricies, GL_FALSE, &matrix[0][0] );
}

void idRenderSystemGLSLLocal::DeleteGPUShader( shaderProgram_t* program )
{
    if ( program->program )
    {
        if ( program->vertexShader )
        {
            qglDetachShader( program->program, program->vertexShader );
            qglDeleteShader( program->vertexShader );
        }
        
        if ( program->fragmentShader )
        {
            qglDetachShader( program->program, program->fragmentShader );
            qglDeleteShader( program->fragmentShader );
        }
        
        qglDeleteProgram( program->program );
        
        if ( program->uniformBuffer )
        {
            memorySystem->Free( program->uniformBuffer );
        }
        
        ::memset( program, 0, sizeof( *program ) );
    }
}

void idRenderSystemGLSLLocal::InitGPUShaders( void )
{
    S32 i, startTime, endTime, attribs, numGenShaders = 0, numLightShaders = 0, numEtcShaders = 0;
    UTF8 extradefines[1024];
    
    clientMainSystem->RefPrintf( PRINT_ALL, "------- idRenderSystemGLSLLocal::InitGPUShaders -------\n" );
    
    idRenderSystemCmdsLocal::IssuePendingRenderCommands();
    
    startTime = clientMainSystem->ScaledMilliseconds();
    
    /////////////////////////////////////////////////////////////////////////////
    for ( i = 0; i < GENERICDEF_COUNT; i++ )
    {
        if ( ( i & GENERICDEF_USE_VERTEX_ANIMATION ) && ( i & GENERICDEF_USE_BONE_ANIMATION ) )
        {
            continue;
        }
        
        if ( ( i & GENERICDEF_USE_BONE_ANIMATION ) && !glRefConfig.glslMaxAnimatedBones )
        {
            continue;
        }
        
        attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_NORMAL | ATTR_COLOR;
        extradefines[0] = '\0';
        
        if ( i & GENERICDEF_USE_DEFORM_VERTEXES )
        {
            Q_strcat( extradefines, 1024, "#define USE_DEFORM_VERTEXES\n" );
        }
        
        if ( i & GENERICDEF_USE_TCGEN_AND_TCMOD )
        {
            Q_strcat( extradefines, 1024, "#define USE_TCGEN\n" );
            Q_strcat( extradefines, 1024, "#define USE_TCMOD\n" );
        }
        
        if ( i & GENERICDEF_USE_VERTEX_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_VERTEX_ANIMATION\n" );
            attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
        }
        else if ( i & GENERICDEF_USE_BONE_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones );
            attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
        }
        
        if ( i & GENERICDEF_USE_FOG )
        {
            Q_strcat( extradefines, 1024, "#define USE_FOG\n" );
        }
        
        if ( i & GENERICDEF_USE_RGBAGEN )
        {
            Q_strcat( extradefines, 1024, "#define USE_RGBAGEN\n" );
        }
        
        if ( !InitGPUShader( &tr.genericShader[i], "generic", attribs, true, extradefines, true ) )
        {
            Com_Error( ERR_FATAL, "Could not load generic shader!" );
        }
        
        InitUniforms( &tr.genericShader[i] );
        
        SetUniformInt( &tr.genericShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
        SetUniformInt( &tr.genericShader[i], UNIFORM_LIGHTMAP,   TB_LIGHTMAP );
#ifdef _DEBUG
        FinishGPUShader( &tr.genericShader[i] );
#endif
        numGenShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    
    if ( !InitGPUShader( &tr.textureColorShader, "texturecolor", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load texturecolor shader!" );
    }
    
    InitUniforms( &tr.textureColorShader );
    SetUniformInt( &tr.textureColorShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.textureColorShader );
#endif
    
    numEtcShaders++;
    
    for ( i = 0; i < FOGDEF_COUNT; i++ )
    {
        if ( ( i & FOGDEF_USE_VERTEX_ANIMATION ) && ( i & FOGDEF_USE_BONE_ANIMATION ) )
        {
            continue;
        }
        
        if ( ( i & FOGDEF_USE_BONE_ANIMATION ) && !glRefConfig.glslMaxAnimatedBones )
        {
            continue;
        }
        
        attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;
        extradefines[0] = '\0';
        
        if ( i & FOGDEF_USE_DEFORM_VERTEXES )
        {
            Q_strcat( extradefines, 1024, "#define USE_DEFORM_VERTEXES\n" );
        }
        
        if ( i & FOGDEF_USE_VERTEX_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_VERTEX_ANIMATION\n" );
            attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
        }
        else if ( i & FOGDEF_USE_BONE_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones );
            attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
        }
        
        if ( !InitGPUShader( &tr.fogShader[i], "fogpass", attribs, true, extradefines, true ) )
        {
            Com_Error( ERR_FATAL, "Could not load fogpass shader!" );
        }
        
        InitUniforms( &tr.fogShader[i] );
#ifdef _DEBUG
        FinishGPUShader( &tr.fogShader[i] );
#endif
        
        numEtcShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    for ( i = 0; i < DLIGHTDEF_COUNT; i++ )
    {
        attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;
        extradefines[0] = '\0';
        
        if ( i & DLIGHTDEF_USE_DEFORM_VERTEXES )
        {
            Q_strcat( extradefines, 1024, "#define USE_DEFORM_VERTEXES\n" );
        }
        
        if ( !InitGPUShader( &tr.dlightShader[i], "dlight", attribs, true, extradefines, false ) )
        {
            Com_Error( ERR_FATAL, "Could not load dlight shader!" );
        }
        
        InitUniforms( &tr.dlightShader[i] );
        SetUniformInt( &tr.dlightShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
        FinishGPUShader( &tr.dlightShader[i] );
#endif
        
        numEtcShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    for ( i = 0; i < LIGHTDEF_COUNT; i++ )
    {
        S32 lightType = i & LIGHTDEF_LIGHTTYPE_MASK;
        bool fastLight = !( r_normalMapping->integer || r_specularMapping->integer );
        
        // skip impossible combos
        if ( ( i & LIGHTDEF_USE_PARALLAXMAP ) && !r_parallaxMapping->integer )
        {
            continue;
        }
        
        if ( ( i & LIGHTDEF_USE_SHADOWMAP ) && ( !lightType || !r_sunlightMode->integer ) )
        {
            continue;
        }
        
        if ( ( i & LIGHTDEF_ENTITY_VERTEX_ANIMATION ) && ( i & LIGHTDEF_ENTITY_BONE_ANIMATION ) )
        {
            continue;
        }
        
        if ( ( i & LIGHTDEF_ENTITY_BONE_ANIMATION ) && !glRefConfig.glslMaxAnimatedBones )
        {
            continue;
        }
        
        attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR | ATTR_NORMAL;
        
        extradefines[0] = '\0';
        
        if ( r_dlightMode->integer >= 2 )
        {
            Q_strcat( extradefines, 1024, "#define USE_SHADOWMAP\n" );
        }
        
        if ( glRefConfig.swizzleNormalmap )
        {
            Q_strcat( extradefines, 1024, "#define SWIZZLE_NORMALMAP\n" );
        }
        
        if ( lightType )
        {
            Q_strcat( extradefines, 1024, "#define USE_LIGHT\n" );
            
            if ( fastLight )
            {
                Q_strcat( extradefines, 1024, "#define USE_FAST_LIGHT\n" );
            }
            
            switch ( lightType )
            {
                case LIGHTDEF_USE_LIGHTMAP:
                    Q_strcat( extradefines, 1024, "#define USE_LIGHTMAP\n" );
                    if ( r_deluxeMapping->integer && !fastLight )
                    {
                        Q_strcat( extradefines, 1024, "#define USE_DELUXEMAP\n" );
                    }
                    attribs |= ATTR_LIGHTCOORD | ATTR_LIGHTDIRECTION;
                    break;
                case LIGHTDEF_USE_LIGHT_VECTOR:
                    Q_strcat( extradefines, 1024, "#define USE_LIGHT_VECTOR\n" );
                    if ( r_dlightMode->integer >= 2 )
                    {
                        Q_strcat( extradefines, sizeof( extradefines ), "#define USE_DSHADOWS\n" );
                    }
                    break;
                case LIGHTDEF_USE_LIGHT_VERTEX:
                    Q_strcat( extradefines, 1024, "#define USE_LIGHT_VERTEX\n" );
                    attribs |= ATTR_LIGHTDIRECTION;
                    break;
                default:
                    break;
            }
            
            if ( r_normalMapping->integer )
            {
                Q_strcat( extradefines, 1024, "#define USE_NORMALMAP\n" );
                
                attribs |= ATTR_TANGENT;
                
                if ( ( i & LIGHTDEF_USE_PARALLAXMAP ) && !( i & LIGHTDEF_ENTITY_VERTEX_ANIMATION ) && !( i & LIGHTDEF_ENTITY_BONE_ANIMATION ) && r_parallaxMapping->integer )
                {
                    Q_strcat( extradefines, 1024, "#define USE_PARALLAXMAP\n" );
                    if ( r_parallaxMapping->integer > 1 )
                    {
                        Q_strcat( extradefines, 1024, "#define USE_RELIEFMAP\n" );
                    }
                    
                    if ( r_parallaxMapShadows->integer )
                    {
                        Q_strcat( extradefines, 1024, "#define USE_PARALLAXMAP_SHADOWS\n" );
                    }
                }
            }
            
            if ( r_specularMapping->integer )
            {
                Q_strcat( extradefines, 1024, "#define USE_SPECULARMAP\n" );
            }
            
            if ( r_cubeMapping->integer )
            {
                Q_strcat( extradefines, 1024, "#define USE_CUBEMAP\n" );
            }
            else if ( r_deluxeSpecular->value > 0.000001f )
            {
                Q_strcat( extradefines, 1024, "#define r_deluxeSpecular %f\n", r_deluxeSpecular->value );
            }
            
            switch ( r_glossType->integer )
            {
                case 0:
                default:
                    Q_strcat( extradefines, 1024, "#define GLOSS_IS_GLOSS\n" );
                    break;
                case 1:
                    Q_strcat( extradefines, 1024, "#define GLOSS_IS_SMOOTHNESS\n" );
                    break;
                case 2:
                    Q_strcat( extradefines, 1024, "#define GLOSS_IS_ROUGHNESS\n" );
                    break;
                case 3:
                    Q_strcat( extradefines, 1024, "#define GLOSS_IS_SHININESS\n" );
                    break;
            }
        }
        
        if ( i & LIGHTDEF_USE_SHADOWMAP )
        {
            Q_strcat( extradefines, 1024, "#define USE_SHADOWMAP\n" );
            
            if ( r_sunlightMode->integer == 1 )
            {
                Q_strcat( extradefines, 1024, "#define SHADOWMAP_MODULATE\n" );
            }
            else if ( r_sunlightMode->integer == 2 )
            {
                Q_strcat( extradefines, 1024, "#define USE_PRIMARY_LIGHT\n" );
            }
        }
        
        if ( i & LIGHTDEF_USE_TCGEN_AND_TCMOD )
        {
            Q_strcat( extradefines, 1024, "#define USE_TCGEN\n" );
            Q_strcat( extradefines, 1024, "#define USE_TCMOD\n" );
        }
        
        if ( i & LIGHTDEF_ENTITY_VERTEX_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_VERTEX_ANIMATION\n#define USE_MODELMATRIX\n" );
            attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
            
            if ( r_normalMapping->integer )
            {
                attribs |= ATTR_TANGENT2;
            }
        }
        else if ( i & LIGHTDEF_ENTITY_BONE_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_MODELMATRIX\n" );
            Q_strcat( extradefines, 1024, "#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones );
            attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
        }
        
        if ( !InitGPUShader( &tr.lightallShader[i], "lightall", attribs, true, extradefines, true ) )
        {
            Com_Error( ERR_FATAL, "Could not load lightall shader!" );
        }
        
        InitUniforms( &tr.lightallShader[i] );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_DIFFUSEMAP,  TB_DIFFUSEMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_LIGHTMAP,    TB_LIGHTMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_NORMALMAP,   TB_NORMALMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_DELUXEMAP,   TB_DELUXEMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_SPECULARMAP, TB_SPECULARMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_SHADOWMAP,   TB_SHADOWMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_SHADOWMAP2,  TB_SHADOWMAP2 );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_CUBEMAP,     TB_CUBEMAP );
        SetUniformInt( &tr.lightallShader[i], UNIFORM_ENVBRDFMAP,  TB_ENVBRDFMAP );
#ifdef _DEBUG
        FinishGPUShader( &tr.lightallShader[i] );
#endif
        
        numLightShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    for ( i = 0; i < SHADOWMAPDEF_COUNT; i++ )
    {
        if ( ( i & SHADOWMAPDEF_USE_VERTEX_ANIMATION ) && ( i & SHADOWMAPDEF_USE_BONE_ANIMATION ) )
        {
            continue;
        }
        
        if ( ( i & SHADOWMAPDEF_USE_BONE_ANIMATION ) && !glRefConfig.glslMaxAnimatedBones )
        {
            continue;
        }
        
        attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;
        
        extradefines[0] = '\0';
        
        if ( i & SHADOWMAPDEF_USE_VERTEX_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_VERTEX_ANIMATION\n" );
            attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
        }
        
        if ( i & SHADOWMAPDEF_USE_BONE_ANIMATION )
        {
            Q_strcat( extradefines, 1024, "#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones );
            attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
        }
        
        if ( !InitGPUShader( &tr.shadowmapShader[i], "shadowfill", attribs, true, extradefines, true ) )
        {
            Com_Error( ERR_FATAL, "Could not load shadowfill shader!" );
        }
        
        InitUniforms( &tr.shadowmapShader[i] );
#ifdef _DEBUG
        FinishGPUShader( &tr.shadowmapShader[i] );
#endif
        
        numEtcShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    if ( !InitGPUShader( &tr.ssrShader, "ssr", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load ssr shader!" );
    }
    
    InitUniforms( &tr.ssrShader );
    SetUniformInt( &tr.ssrShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    SetUniformInt( &tr.ssrShader, UNIFORM_GLOWMAP, TB_GLOWMAP );
    SetUniformInt( &tr.ssrShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.ssrShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    if ( !InitGPUShader( &tr.ssrCombineShader, "ssrCombine", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load ssrCombine shader!" );
    }
    
    InitUniforms( &tr.ssrCombineShader );
    SetUniformInt( &tr.ssrCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    SetUniformInt( &tr.ssrCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.ssrCombineShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_NORMAL;
    extradefines[0] = '\0';
    
    Q_strcat( extradefines, 1024, "#define USE_PCF\n#define USE_DISCARD\n" );
    
    if ( !InitGPUShader( &tr.pshadowShader, "pshadow", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load pshadow shader!" );
    }
    
    InitUniforms( &tr.pshadowShader );
    SetUniformInt( &tr.pshadowShader, UNIFORM_SHADOWMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.pshadowShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.down4xShader, "down4x", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load down4x shader!" );
    }
    
    InitUniforms( &tr.down4xShader );
    SetUniformInt( &tr.down4xShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.down4xShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.bokehShader, "bokeh", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load bokeh shader!" );
    }
    
    InitUniforms( &tr.bokehShader );
    SetUniformInt( &tr.bokehShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.bokehShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.tonemapShader, "tonemap", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load tonemap shader!" );
    }
    
    InitUniforms( &tr.tonemapShader );
    SetUniformInt( &tr.tonemapShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.tonemapShader, UNIFORM_LEVELSMAP,  TB_LEVELSMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.tonemapShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    for ( i = 0; i < 2; i++ )
    {
        attribs = ATTR_POSITION | ATTR_TEXCOORD;
        extradefines[0] = '\0';
        
        if ( !i )
            Q_strcat( extradefines, 1024, "#define FIRST_PASS\n" );
            
        if ( !InitGPUShader( &tr.calclevels4xShader[i], "calclevels4x", attribs, true, extradefines, true ) )
        {
            Com_Error( ERR_FATAL, "Could not load calclevels4x shader!" );
        }
        
        InitUniforms( &tr.calclevels4xShader[i] );
        SetUniformInt( &tr.calclevels4xShader[i], UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
        FinishGPUShader( &tr.calclevels4xShader[i] );
#endif
        
        numEtcShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( r_shadowFilter->integer >= 1 )
    {
        Q_strcat( extradefines, 1024, "#define USE_SHADOW_FILTER\n" );
    }
    
    if ( r_shadowFilter->integer >= 2 )
    {
        Q_strcat( extradefines, 1024, "#define USE_SHADOW_FILTER2\n" );
    }
    
    if ( r_shadowCascadeZFar->integer != 0 )
    {
        Q_strcat( extradefines, 1024, "#define USE_SHADOW_CASCADE\n" );
    }
    
    Q_strcat( extradefines, 1024, "#define r_shadowMapSize %f\n", r_shadowMapSize->value );
    Q_strcat( extradefines, 1024, "#define r_shadowCascadeZFar %f\n", r_shadowCascadeZFar->value );
    
    if ( !InitGPUShader( &tr.shadowmaskShader, "shadowmask", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load shadowmask shader!" );
    }
    
    InitUniforms( &tr.shadowmaskShader );
    SetUniformInt( &tr.shadowmaskShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP );
    SetUniformInt( &tr.shadowmaskShader, UNIFORM_SHADOWMAP,  TB_SHADOWMAP );
    SetUniformInt( &tr.shadowmaskShader, UNIFORM_SHADOWMAP2, TB_SHADOWMAP2 );
    SetUniformInt( &tr.shadowmaskShader, UNIFORM_SHADOWMAP3, TB_SHADOWMAP3 );
    SetUniformInt( &tr.shadowmaskShader, UNIFORM_SHADOWMAP4, TB_SHADOWMAP4 );
#ifdef _DEBUG
    FinishGPUShader( &tr.shadowmaskShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.ssaoShader, "ssao", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load ssao shader!" );
    }
    
    InitUniforms( &tr.ssaoShader );
    SetUniformInt( &tr.ssaoShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.ssaoShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    for ( i = 0; i < 4; i++ )
    {
        attribs = ATTR_POSITION | ATTR_TEXCOORD;
        extradefines[0] = '\0';
        
        if ( i & 1 )
        {
            Q_strcat( extradefines, 1024, "#define USE_VERTICAL_BLUR\n" );
        }
        else
        {
            Q_strcat( extradefines, 1024, "#define USE_HORIZONTAL_BLUR\n" );
        }
        
        if ( !( i & 2 ) )
        {
            Q_strcat( extradefines, 1024, "#define USE_DEPTH\n" );
        }
        
        if ( !InitGPUShader( &tr.depthBlurShader[i], "depthblur", attribs, true, extradefines, true ) )
        {
            Com_Error( ERR_FATAL, "Could not load depthBlur shader!" );
        }
        
        InitUniforms( &tr.depthBlurShader[i] );
        SetUniformInt( &tr.depthBlurShader[i], UNIFORM_SCREENIMAGEMAP, TB_COLORMAP );
        SetUniformInt( &tr.depthBlurShader[i], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
#ifdef _DEBUG
        FinishGPUShader( &tr.depthBlurShader[i] );
#endif
        
        numEtcShaders++;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.prefilterEnvMapShader, "prefilterEnvMap", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load prefilterEnvMap shader!" );
    }
    
    InitUniforms( &tr.prefilterEnvMapShader );
    SetUniformInt( &tr.prefilterEnvMapShader, UNIFORM_CUBEMAP, TB_CUBEMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.prefilterEnvMapShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.prefilterEnvMapShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.darkexpandShader, "darkexpand", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load darkexpand shader!" );
    }
    
    InitUniforms( &tr.darkexpandShader );
    SetUniformInt( &tr.darkexpandShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.darkexpandShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.darkexpandShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.darkexpandShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.darkexpandShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.multipostShader, "multipost", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load darkexpand shader!" );
    }
    
    InitUniforms( &tr.multipostShader );
    SetUniformInt( &tr.multipostShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    SetUniformInt( &tr.multipostShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP );
    SetUniformInt( &tr.multipostShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP );
    SetUniformInt( &tr.multipostShader, UNIFORM_LIGHTMAP, TB_LIGHTMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.multipostShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.lensflareShader, "lensflare", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load lensflare shader!" );
    }
    
    InitUniforms( &tr.lensflareShader );
    SetUniformInt( &tr.lensflareShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.lensflareShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.anamorphicDarkenShader, "anamorphic_darken", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load anamorphic_darken shader!" );
    }
    
    InitUniforms( &tr.anamorphicDarkenShader );
    SetUniformInt( &tr.anamorphicDarkenShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.anamorphicDarkenShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.anamorphicBlurShader, "anamorphic_blur", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load anamorphic_blur shader!" );
    }
    
    InitUniforms( &tr.anamorphicBlurShader );
    SetUniformInt( &tr.anamorphicBlurShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.anamorphicBlurShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.anamorphicCombineShader, "anamorphic_combine", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load anamorphic_combine shader!" );
    }
    
    InitUniforms( &tr.anamorphicCombineShader );
    SetUniformInt( &tr.anamorphicCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    SetUniformInt( &tr.anamorphicCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.anamorphicCombineShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.hdrShader, "truehdr", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load hdr shader!" );
    }
    
    InitUniforms( &tr.hdrShader );
    SetUniformInt( &tr.hdrShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.hdrShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.hdrShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.hdrShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    if ( !InitGPUShader( &tr.ssgiShader, "ssgi", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load ssgi shader!" );
    }
    
    InitUniforms( &tr.ssgiShader );
    SetUniformInt( &tr.ssgiShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.ssgiShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.ssgiShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    if ( !InitGPUShader( &tr.ssgiBlurShader, "ssgiBlur", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load ssgi_blur shader!" );
    }
    
    InitUniforms( &tr.ssgiBlurShader );
    SetUniformInt( &tr.ssgiBlurShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
#ifdef _DEBUG
    FinishGPUShader( &tr.ssgiBlurShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.dofShader, "depthOfField", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load depthOfField shader!" );
    }
    
    InitUniforms( &tr.dofShader );
    SetUniformInt( &tr.dofShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.dofShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.dofShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.dofShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.dofShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.esharpeningShader, "esharpening", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load esharpening shader!" );
    }
    
    InitUniforms( &tr.esharpeningShader );
    SetUniformInt( &tr.esharpeningShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.esharpeningShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.esharpeningShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.esharpeningShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.esharpeningShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.esharpening2Shader, "esharpening2", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load esharpening2 shader!" );
    }
    
    InitUniforms( &tr.esharpening2Shader );
    SetUniformInt( &tr.esharpening2Shader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.esharpening2Shader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.esharpening2Shader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.esharpening2Shader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.esharpening2Shader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    if ( !InitGPUShader( &tr.texturecleanShader, "textureclean", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load textureclean shader!" );
    }
    
    InitUniforms( &tr.texturecleanShader );
    SetUniformInt( &tr.texturecleanShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.texturecleanShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.texturecleanShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.texturecleanShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.texturecleanShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.vibrancyShader, "vibrancy", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load vibrancy shader!" );
    }
    
    InitUniforms( &tr.vibrancyShader );
    SetUniformInt( &tr.vibrancyShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.vibrancyShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.vibrancyShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.anaglyphShader, "anaglyph", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load anaglyph shader!" );
    }
    
    InitUniforms( &tr.anaglyphShader );
    SetUniformInt( &tr.anaglyphShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.anaglyphShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.anaglyphShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.anaglyphShader, UNIFORM_DIMENSIONS, screensize );
    }
    
    {
        vec4_t local0;
        VectorSet4( local0, r_trueAnaglyphSeparation->value, r_trueAnaglyphRed->value, r_trueAnaglyphGreen->value, r_trueAnaglyphBlue->value );
        SetUniformVec4( &tr.anaglyphShader, UNIFORM_LOCAL0, local0 );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.anaglyphShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.texturedetailShader, "texturedetail", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load texturedetail shader!" );
    }
    
    InitUniforms( &tr.texturedetailShader );
    SetUniformInt( &tr.texturedetailShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.texturedetailShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.texturedetailShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.texturedetailShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    
    FinishGPUShader( &tr.texturedetailShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.rbmShader, "rbm", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load rbm shader!" );
    }
    
    InitUniforms( &tr.rbmShader );
    SetUniformInt( &tr.rbmShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP );
    SetUniformInt( &tr.rbmShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    SetUniformInt( &tr.rbmShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    
#ifdef _DEBUG
    FinishGPUShader( &tr.rbmShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.contrastShader, "contrast", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load bloom shader!" );
    }
    
    InitUniforms( &tr.contrastShader );
    SetUniformInt( &tr.contrastShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP );
    SetUniformFloat( &tr.contrastShader, UNIFORM_BRIGHTNESS, r_brightness->value );
    SetUniformFloat( &tr.contrastShader, UNIFORM_CONTRAST, r_contrast->value );
    SetUniformFloat( &tr.contrastShader, UNIFORM_GAMMA, r_gamma->value );
    
#ifdef _DEBUG
    FinishGPUShader( &tr.contrastShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    if ( !InitGPUShader( &tr.fxaaShader, "fxaa", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load fxaa shader!" );
    }
    
    InitUniforms( &tr.fxaaShader );
    SetUniformInt( &tr.fxaaShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    SetUniformInt( &tr.fxaaShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.fxaaShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.fxaaShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.fxaaShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.bloomDarkenShader, "bloom_darken", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load bloom_darken shader!" );
    }
    
    InitUniforms( &tr.bloomDarkenShader );
    SetUniformInt( &tr.bloomDarkenShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    
#ifdef _DEBUG
    FinishGPUShader( &tr.bloomDarkenShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.bloomBlurShader, "bloom_blur", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load bloom_blur shader!" );
    }
    
    InitUniforms( &tr.bloomBlurShader );
    SetUniformInt( &tr.bloomBlurShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    
#ifdef _DEBUG
    FinishGPUShader( &tr.bloomBlurShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.bloomCombineShader, "bloom_combine", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load bloom_combine shader!" );
    }
    
    InitUniforms( &tr.bloomCombineShader );
    SetUniformInt( &tr.bloomCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    SetUniformInt( &tr.bloomCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP );
    
#ifdef _DEBUG
    FinishGPUShader( &tr.bloomCombineShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.waterShader, "water", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load water shader!" );
    }
    
    InitUniforms( &tr.waterShader );
    SetUniformInt( &tr.waterShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.waterShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.waterShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.waterShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_TEXCOORD;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.underWaterShader, "underwater", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load water shader!" );
    }
    
    InitUniforms( &tr.underWaterShader );
    SetUniformInt( &tr.underWaterShader, UNIFORM_TEXTUREMAP, TB_COLORMAP );
    
    {
        vec4_t viewInfo;
        
        F32 zmax = backEnd.viewParms.zFar;
        F32 zmin = r_znear->value;
        
        VectorSet4( viewInfo, zmax / zmin, zmax, 0.0, 0.0 );
        
        SetUniformVec4( &tr.underWaterShader, UNIFORM_VIEWINFO, viewInfo );
    }
    
    {
        vec2_t screensize;
        screensize[0] = ( F32 )glConfig.vidWidth;
        screensize[1] = ( F32 )glConfig.vidHeight;
        
        SetUniformVec2( &tr.underWaterShader, UNIFORM_DIMENSIONS, screensize );
    }
    
#ifdef _DEBUG
    FinishGPUShader( &tr.underWaterShader );
#endif
    
    numEtcShaders++;
    
    /////////////////////////////////////////////////////////////////////////////
    attribs = ATTR_POSITION | ATTR_COLOR | ATTR_NORMAL | ATTR_TANGENT | ATTR_LIGHTDIRECTION | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_TANGENT2;
    extradefines[0] = '\0';
    
    if ( !InitGPUShader( &tr.sunPassShader, "sun", attribs, true, extradefines, true ) )
    {
        Com_Error( ERR_FATAL, "Could not load water shader!" );
    }
    
    InitUniforms( &tr.sunPassShader );
    SetUniformInt( &tr.sunPassShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP );
    
#ifdef _DEBUG
    FinishGPUShader( &tr.sunPassShader );
#endif
    
    numEtcShaders++;
    
    endTime = clientMainSystem->ScaledMilliseconds();
    
    clientMainSystem->RefPrintf( PRINT_ALL, "loaded %i GLSL shaders (%i gen %i light %i etc) in %5.2f seconds\n",
                                 numGenShaders + numLightShaders + numEtcShaders, numGenShaders, numLightShaders,
                                 numEtcShaders, ( endTime - startTime ) / 1000.0f );
}

void idRenderSystemGLSLLocal::ShutdownGPUShaders( void )
{
    S32 i;
    
    clientMainSystem->RefPrintf( PRINT_ALL, "------- idRenderSystemGLSLLocal::ShutdownGPUShaders -------\n" );
    
    for ( i = 0; i < ATTR_INDEX_COUNT; i++ )
    {
        qglDisableVertexAttribArray( i );
    }
    
    idRenderSystemDSALocal::BindNullProgram();
    
    for ( i = 0; i < GENERICDEF_COUNT; i++ )
    {
        DeleteGPUShader( &tr.genericShader[i] );
    }
    
    DeleteGPUShader( &tr.textureColorShader );
    
    for ( i = 0; i < FOGDEF_COUNT; i++ )
    {
        DeleteGPUShader( &tr.fogShader[i] );
    }
    
    for ( i = 0; i < DLIGHTDEF_COUNT; i++ )
    {
        DeleteGPUShader( &tr.dlightShader[i] );
    }
    
    for ( i = 0; i < LIGHTDEF_COUNT; i++ )
    {
        DeleteGPUShader( &tr.lightallShader[i] );
    }
    
    for ( i = 0; i < SHADOWMAPDEF_COUNT; i++ )
    {
        DeleteGPUShader( &tr.shadowmapShader[i] );
    }
    
    DeleteGPUShader( &tr.pshadowShader );
    DeleteGPUShader( &tr.down4xShader );
    DeleteGPUShader( &tr.bokehShader );
    DeleteGPUShader( &tr.tonemapShader );
    
    for ( i = 0; i < 2; i++ )
    {
        DeleteGPUShader( &tr.calclevels4xShader[i] );
    }
    
    DeleteGPUShader( &tr.shadowmaskShader );
    DeleteGPUShader( &tr.ssaoShader );
    
    for ( i = 0; i < 4; i++ )
    {
        DeleteGPUShader( &tr.depthBlurShader[i] );
    }
    
    DeleteGPUShader( &tr.darkexpandShader );
    DeleteGPUShader( &tr.multipostShader );
    DeleteGPUShader( &tr.lensflareShader );
    DeleteGPUShader( &tr.anamorphicDarkenShader );
    DeleteGPUShader( &tr.anamorphicBlurShader );
    DeleteGPUShader( &tr.anamorphicCombineShader );
    DeleteGPUShader( &tr.hdrShader );
    DeleteGPUShader( &tr.dofShader );
    DeleteGPUShader( &tr.esharpeningShader );
    DeleteGPUShader( &tr.esharpening2Shader );
    DeleteGPUShader( &tr.texturecleanShader );
    DeleteGPUShader( &tr.anaglyphShader );
    DeleteGPUShader( &tr.vibrancyShader );
    DeleteGPUShader( &tr.fxaaShader );
    DeleteGPUShader( &tr.bloomDarkenShader );
    DeleteGPUShader( &tr.bloomBlurShader );
    DeleteGPUShader( &tr.bloomCombineShader );
    DeleteGPUShader( &tr.texturedetailShader );
    DeleteGPUShader( &tr.contrastShader );
    DeleteGPUShader( &tr.rbmShader );
    DeleteGPUShader( &tr.ssrShader );
    DeleteGPUShader( &tr.ssrCombineShader );
    DeleteGPUShader( &tr.ssgiShader );
    DeleteGPUShader( &tr.ssgiBlurShader );
    DeleteGPUShader( &tr.prefilterEnvMapShader );
    DeleteGPUShader( &tr.waterShader );
    DeleteGPUShader( &tr.sunPassShader );
}

void idRenderSystemGLSLLocal::BindProgram( shaderProgram_t* program )
{
    U32 programObject = program ? program->program : 0;
    StringEntry name = program ? program->name : "NULL";
    
    if ( r_logFile->integer )
    {
        // don't just call idRenderSystemGlimpLocal::LogComment, or we will get a call to va() every frame!
        idRenderSystemGlimpLocal::LogComment( reinterpret_cast< UTF8* >( va( "--- idRenderSystemGLSLLocal::BindProgram( %s ) ---\n", name ) ) );
    }
    
    if ( idRenderSystemDSALocal::UseProgram( programObject ) )
    {
        backEnd.pc.c_glslShaderBinds++;
    }
}

shaderProgram_t* idRenderSystemGLSLLocal::GetGenericShaderProgram( S32 stage )
{
    shaderStage_t* pStage = tess.xstages[stage];
    S32 shaderAttribs = 0;
    
    if ( tess.fogNum && pStage->adjustColorsForFog )
    {
        shaderAttribs |= GENERICDEF_USE_FOG;
    }
    
    switch ( pStage->rgbGen )
    {
        case CGEN_LIGHTING_DIFFUSE:
            shaderAttribs |= GENERICDEF_USE_RGBAGEN;
            break;
        default:
            break;
    }
    
    switch ( pStage->alphaGen )
    {
        case AGEN_LIGHTING_SPECULAR:
        case AGEN_PORTAL:
            shaderAttribs |= GENERICDEF_USE_RGBAGEN;
            break;
        default:
            break;
    }
    
    if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE )
    {
        shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
    }
    
    if ( tess.shader->numDeforms && !idRenderSystemShadeLocal::ShaderRequiresCPUDeforms( tess.shader ) )
    {
        shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
    }
    
    if ( glState.vertexAnimation )
    {
        shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
    }
    
    if ( pStage->bundle[0].numTexMods )
    {
        shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
    }
    
    return &tr.genericShader[shaderAttribs];
}
