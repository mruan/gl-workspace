/*

Camera control class

*/

#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <glm/glm.hpp>

class Camera{

public:
  Camera();

  Camera(unsigned int height, unsigned int width,
	 float fov, float aspr,
	 float m_speed, float k_speed,
	 glm::vec3 init_pos, glm::vec3 init_up, glm::vec3 init_dir);

  void setPosDir(glm::vec3 pos, glm::vec3 dir);

  void resetView(void);
  void updateViewFromInput();

  glm::mat4 getMVP(glm::mat4& model_mat);
private:
  void keyboardUpdate(float);
  void mouseTranslateUpdate();
  void mouseRotateUpdate();
  void updateView(void);
  
private:
  unsigned int _height, _width;
  float _mouse_speed, _keyboard_speed;
  glm::vec3 _init_pos, _init_dir, _init_up;
  glm::vec3 _cur_pos, _cur_dir, _cur_up;

  glm::mat4 _proj_mat, _view_mat;
};

#endif
