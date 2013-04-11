#ifndef SHADER_H
#define SHADER_H

#include <list>
#include <GL/glew.h>

#include <glm/glm.hpp>

#define MAX_BONES 40

#ifndef INVALID_OGL_VALUE
#define INVALID_OGL_VALUE 0xffffffff
#endif

class Shader
{
public:
  Shader(const char* pEffectFile);

  ~Shader();

  bool Init();

  void Enable();

  void SetWVP(const glm::mat4& wvp);

  void SetColorTextureUnit(GLuint Texture);

  void SetBoneTransform(unsigned int idx, const glm::mat4& Tf);
protected:
  bool CompileProgram(const char* pProgram);

  GLint GetUniformLocation(const char* pUniformName);

  GLint GetProgramParam(GLint param);

private:
  GLint m_effect;
  GLint m_shaderProg;

  const char* m_pEffectFile;

  GLuint m_WVPLocation;
  GLuint m_ColorTextureLocation;
  GLuint m_boneLocation[MAX_BONES];
};

#define INVALID_UNIFORM_LOCATION 0xFFFFFFFF

#endif
