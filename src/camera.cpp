// Camera class:

#include <math.h>
#include <stdio.h>
#include <GL/glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace glm;

#include "camera.hpp"

Camera::Camera()
{
}

Camera::Camera(unsigned int height, unsigned int width,
	       float fov, float aspr,
	       float m_speed, float k_speed,
	       glm::vec3 init_pos, glm::vec3 init_up, glm::vec3 init_tar)
  :_height(height),_width(width),
   _mouse_speed(m_speed),_keyboard_speed(k_speed)
{
  _init_pos = init_pos;
  _init_dir = glm::normalize(init_tar - init_pos);
  _init_up  = init_up; 
  _init_up  = glm::normalize(_init_up - glm::dot(_init_up, _init_dir)*_init_dir);

  printf("Init pos: %f %f %f\n", _init_pos.x, _init_pos.y, _init_pos.z);
  printf("Init dir: %f %f %f\n", _init_dir.x, _init_dir.y, _init_dir.z);
  printf("Init up:  %f %f %f\n", _init_up.x, _init_up.y, _init_up.z);

  _cur_pos = _init_pos;
  _cur_dir = _init_dir;
  _cur_up  = _init_up;

  // Set up projection and view matrices
  _proj_mat = perspective(fov, aspr, 0.1f, 100.0f);
  _view_mat=lookAt(_cur_pos, _cur_pos+_cur_dir, _cur_up);
}

void
Camera::setPosDir(vec3 pos, vec3 dir)
{
  _cur_pos = pos;
  _cur_dir = dir;
}

void  // View is not changed until this is called
Camera::updateView()
{
  _cur_dir = glm::normalize(_cur_dir);
  _cur_up  = glm::normalize(_cur_up);
  _view_mat=lookAt(_cur_pos, _cur_pos+_cur_dir, _cur_up);
}

void
Camera::resetView(void)
{
  _cur_pos = _init_pos;
  _cur_dir = _init_dir;
  _cur_up  = _init_up;
  updateView();
}

void
Camera::updateViewFromInput()
{
  static double lastTime = glfwGetTime();

  double currentTime = glfwGetTime();
  float del_T = float(currentTime - lastTime);

  mouseRotateUpdate();
  
  mouseTranslateUpdate();

  keyboardUpdate(del_T);

  updateView();

  lastTime = currentTime;
}

glm::mat4
Camera::getMVP(glm::mat4& model_mat)
{
  return _proj_mat* _view_mat * model_mat;
}

void
Camera::keyboardUpdate(float del_T)
{
  glm::vec3 right = glm::cross(_cur_dir, _cur_up);

  if (glfwGetKey('R') == GLFW_PRESS)
    {
      resetView();
      return;
    }
    
  // Move up
  if (glfwGetKey( GLFW_KEY_UP ) == GLFW_PRESS){
    _cur_pos += _cur_up * del_T * _keyboard_speed;
  }
  // Move backward
  if (glfwGetKey( GLFW_KEY_DOWN ) == GLFW_PRESS){
    _cur_pos -= _cur_up * del_T * _keyboard_speed;
  }
  // Strafe right
  if (glfwGetKey( GLFW_KEY_RIGHT ) == GLFW_PRESS){
    _cur_pos += right * del_T * _keyboard_speed;
  }
  // Strafe left
  if (glfwGetKey( GLFW_KEY_LEFT ) == GLFW_PRESS){
    _cur_pos -= right * del_T * _keyboard_speed;
  }
  
}

void
Camera::mouseTranslateUpdate(void)
{
  _cur_pos += _cur_dir * _mouse_speed * 1.0f * float(glfwGetMouseWheel());
  glfwSetMouseWheel(0);

  static bool view_translate_flag = false;
  if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)== GLFW_PRESS)
    {
      // start to rotate the view
      static int prev_x, prev_y, cur_x, cur_y;
      if (view_translate_flag == false)
	{
	  glfwGetMousePos(&prev_x, &prev_y);
	  view_translate_flag = true;
	}
      else
	{
	  glfwGetMousePos(&cur_x, &cur_y);
	  float del_x, del_y;
	  del_x = cur_x - prev_x;
	  del_y = cur_y - prev_y;

	  glm::vec3 right = glm::cross(_cur_dir, _cur_up);

#define FACTOR 0.1f
       	  _cur_pos -= right * del_x * _mouse_speed *FACTOR;
      	  _cur_pos += _cur_up*del_y * _mouse_speed *FACTOR;
	  prev_x = cur_x;
	  prev_y = cur_y;
	}
    }
  else
    view_translate_flag = false;
}

void
Camera::mouseRotateUpdate(void)
{
  static bool view_rotate_flag = false;
  if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT)== GLFW_PRESS)
    {
      // start to rotate the view
      static int prev_x, prev_y, cur_x, cur_y;
      if (view_rotate_flag == false)
	{
	  glfwGetMousePos(&prev_x, &prev_y);
	  view_rotate_flag = true;
	}
      else
	{
	  glfwGetMousePos(&cur_x, &cur_y);
	  float del_x, del_y;
	  del_x = cur_x - prev_x;
	  del_y = cur_y - prev_y;

	  // Pan only
	  if (abs(del_x) > abs(del_y))
	    {
	      _cur_dir = glm::rotate(_cur_dir,
				     del_x*_mouse_speed/3.14f,
				     _cur_up);
	    }
	  else // Tilt only
	    {
	      glm::vec3 right = glm::cross(_cur_dir, _cur_up);
	      _cur_dir = glm::rotate(_cur_dir,
				     del_y *_mouse_speed/3.14f,
				     right);
	      _cur_up  = glm::cross(right, _cur_dir);
	    }
	  prev_x = cur_x;
	  prev_y = cur_y;
	}
    }
  else // Reset rotating
    view_rotate_flag = false;
}
