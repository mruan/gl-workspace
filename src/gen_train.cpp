#include "bvh_utils.hpp"

struct params
{
  int skip_frame;
  int donwsample_rate;
  std::string path_to_root;
};
// global parameters
params gp;

void help()
{
  printf("Usage of this program:\n");
  printf("gen_train -r ref_skeleton -c configure file -d dump_file\n");
}

bool getNextBvhFile(FILE*& conf_fd, FILE*& bvh_fd)
{
  char full_path[128];
  char file_path[128];
  strcpy(path, gp.path_to_root.c_str());

  if(fscanf(conf_fd, " %s", file_path) == EOF)
    return false;

  strcat(full_path, file_path);

  //just in case it wasn't shut correctly
  if (bvh_fd)
    fclose(bvh_fd);

  bvh_fd = fopen(full_path, "r");

  if (bvh_fd == 0)
    {
      printf("Error loading bvh file %s\n", full_path);
      return false;
    }
  return true;
}

int main(int argc, char* argv[])
{
  if(argc !=3)
    {
      help();
      return 0;
    }

  int i = 1;
  FILE* ref_fd=0, conf_fd=0, dump_fd=0;
  bool r_flag= false, c_flag= false, d_flag=false;
  while(i < argc)
    {
      if(strcmp(argv[i],"-c")==0)
	{
	  ref_fd = fopen(argv[i+1], "r");
	  if (ref_fd ==0)
	    {
	      printf("Could not load ref file %s\n", argv[i+1]);
	      return -1;
	    }
	  i+=2;
	  r_flag = true;
	}
      else if (strcmp(argv[i], "-r")==0)
	{
	  conf_fd = fopen(argv[i+1], "r");
	  if (conf_fd ==0)
	    {
	      printf("Could not load conf file %s\n", argv[i+1]);
	      return -1;
	    }
	  i+=2;
	  c_flag = true;
	}
      else if (strcmp(argv[i], "-d")==0)
	{
	  dump_fd = fopen(argv[i+1], "w");
	  if (dump_fd ==0)
	    {
	      printf("Could not open dump file %s\n", argv[i+1]);
	      return -1;
	    }
	  i+=2;
	  d_flag = true;
	}
    }

  // assert all flags must be true
  assert(r_flag && c_flag && d_flag);

  // Load the reference skeleton
  BvhSkeleton ref_sk;
  ref_sk.LoadFromFD(ref_fd);

  // Extract the parameters from the configuration file
  gp = getParamFromConfig(conf_fd);

  // Load the first bvh in the list to build the mask
  BvhLoader bvhloader(gp.skip_frame, gp.downsample_rate);
  FILE* bvh_fd;

  getNextBvhFile(conf_fd, bvh_fd);
  // this will cause bvhloader to create mask internally
  bvhloader.Load(bvh_fd, ref_sk);
  fclose(bvh_fd); 
  bvh_fd=0;

  bvhloader.Dump(dump_fd);

  // Read in all other bvh files
  while(getNextBvhFile(conf_fd, bvh_fd) != NULL)
    {
      bvhloader.Load(bvh_fd);
      fclose(bvh_fd);
      bvh_fd = 0;

      bvhloader.Dump(dump_fd);
    }
  fclose(dump_fd);

  return 0;
}
