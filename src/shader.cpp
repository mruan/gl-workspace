#include <stdio.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <glfx.h>

#include "shader.hpp"

using namespace std;

Shader::Shader(const char* pEffectFile)
{
  m_pEffectFile = pEffectFile;
  m_shaderProg = 0;
  m_effect = glfxGenEffect();
}

Shader::~Shader()
{
  if (m_shaderProg != 0)
    {
      glDeleteProgram(m_shaderProg);
      m_shaderProg = 0;
    }
    
  glfxDeleteEffect(m_effect); 
}

bool Shader::CompileProgram(const char* pProgram)
{
    if (!glfxParseEffectFromFile(m_effect, m_pEffectFile)) {
        string log = glfxGetEffectLog(m_effect);
        printf("Error creating effect from file '%s':\n", m_pEffectFile);
        printf("%s\n", log.c_str());
        return false;
    }
    
    m_shaderProg = glfxCompileProgram(m_effect, pProgram);
    
    if (m_shaderProg < 0) {
        string log = glfxGetEffectLog(m_effect);
        printf("Error compiling program '%s' in effect file '%s':\n", pProgram, m_pEffectFile);
        printf("%s\n", log.c_str());
        return false;
    }
    
    return true;
}

void Shader::Enable()
{
    glUseProgram(m_shaderProg);
}


GLint Shader::GetUniformLocation(const char* pUniformName)
{
    GLuint Location = glGetUniformLocation(m_shaderProg, pUniformName);

    if (Location == INVALID_OGL_VALUE) {
        fprintf(stderr, "Warning! Unable to get the location of uniform '%s'\n", pUniformName);
    }

    return Location;
}

GLint Shader::GetProgramParam(GLint param)
{
    GLint ret;
    glGetProgramiv(m_shaderProg, param, &ret);
    return ret;
}

/*************************************************************************************************
Technique class
*************************************************************************************************/
bool Shader::Init()
{
  if(!CompileProgram("Shading"))
    return false;

  m_WVPLocation = GetUniformLocation("gWVP");
  m_ColorTextureLocation = GetUniformLocation("gColorMap");

  for(unsigned int i=0; i< MAX_BONES; i++)
    {
      char name[32];
      memset(name, 0, sizeof(name));
      sprintf(name, "gBones[%d]", i);
      m_boneLocation[i] = GetUniformLocation(name);
    }

  return true;
}


void Shader::SetWVP(const glm::mat4& wvp)
{
  glUniformMatrix4fv(m_WVPLocation, 1, GL_FALSE, (const GLfloat*) &wvp[0][0]);
}

void Shader::SetColorTextureUnit(GLuint Texture)
{
  // Bind our texture in Texture Unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, Texture);
  // Set our "gcolormap" sampler to user Texture Unit 0
  glUniform1i(m_ColorTextureLocation, 0);
}

void Shader::SetBoneTransform(unsigned int idx, const glm::mat4& Tf)
{
  assert(idx < MAX_BONES);
  glUniformMatrix4fv(m_boneLocation[idx], 1, GL_FALSE, (const GLfloat*) &Tf[0][0]);
}
