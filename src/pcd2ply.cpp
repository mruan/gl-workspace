#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>

int
main (int argc, char** argv)
{
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);

  if (argc < 2)
    {
      printf("pcd2ply FILE_NAME_WITHOUT_EXTENSION\n");
      return -1;
    }

  char in_file[64];
  char outfile[64];
  sprintf(in_file, "%s.pcd", argv[1]);
  sprintf(outfile, "%s.ply", argv[1]);

  if (pcl::io::loadPCDFile<pcl::PointXYZ> (in_file, *cloud) == -1) //* load the file
  {
    PCL_ERROR ("Couldn't read file %s \n", in_file);
    return (-1);
  }

  pcl::PLYWriter writer;
  writer.write(outfile, *cloud);
  
  printf("Write file to %s\n", outfile);

  return (0);
}
