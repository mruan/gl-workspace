#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glfw.h>

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

namespace bu
{
  void saveColorBuffer(int counter)
  {
    int w, h;
    glfwGetWindowSize(&w, &h);
    cv::Mat img(h, w, CV_8UC3);

    glFinish(); //finish all commands of OpenGL  
    //use fast 4-byte alignment (default anyway) if possible
    glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
    //set length of one complete row in destination data (doesn't need to equal img.cols)
    glPixelStorei(GL_PACK_ROW_LENGTH, img.step/img.elemSize());

    glPixelStorei(GL_PACK_SKIP_ROWS, 0);  
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0); 

    glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);

    cv::Mat flipped;
    cv::flip(img, flipped, 0);

    char path[64];
    sprintf(path, "i%04d.png", counter);
    cv::imwrite(path, flipped);

    printf("Saving color image: %s\n", path);
  }

  void saveDepthBufferToPCD(int counter)
  {
    int w, h;
    glfwGetWindowSize(&w, &h);
    float* dimg = new float[w*h];

    glFinish(); //finish all commands of OpenGL

    glPixelStorei(GL_PACK_SKIP_ROWS, 0);  
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0); 
    //GL_DEPTH_COMPONENT
    glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, dimg);

    char path[64];
    sprintf(path, "d%04d.pcd", counter);
    FILE* fd = fopen(path, "w");

    fprintf(fd, "# .PCD v.7 - Point Cloud Data file format\n");
    fprintf(fd, "VERSION .7\n");
    fprintf(fd, "FIELDS x y z\nSIZE 4 4 4\nTYPE F F F\nCOUNT 1 1 1\n");
    fprintf(fd, "WIDTH %d\nHEIGHT %d\n", w, h);
    fprintf(fd, "VIEWPOINT 0 0 0 1 0 0 0\nPOINTS %d\nDATA ascii\n", w*h);

    float* p= &dimg[0];
    int x0 = w/2;
    int y0 = h/2;
    float fx = 1.0f/w;
    float fy = 1.0f/h;

    for(int i = 0; i < h; ++i)
      {
        for (int j = 0; j < w; ++j)
	  {
            fprintf(fd, "%.4f %.4f %.4f\n", (i-x0)*fx, (j-y0)*fy, 20*(*p++));
	  }
      }
    fclose(fd);
    printf("Saving depth image: %s\n", path);
  }
}
