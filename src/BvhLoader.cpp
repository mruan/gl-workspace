#include "BvhLoader.hpp"
#include <string.h>
#include <assert.h>

using namespace std;

#define BUFFER_SIZE 512

/*******************************************************************************
BvkSkeleton Class
 *******************************************************************************/
bool BvhSkeleton::ParseSkeleton(FILE*& fd)
{
  char buffer[BUFFER_SIZE];

  // Read in "Hierarchy"
  fscanf(fd, " %s", buffer);
  if (strcmp(buffer, "HIERARCHY") !=0)
    {
      fprintf(stderr, "BVH file must start with HIERARCHY\n");
      return false;
    }
  // finish the rest of the line
  fgets(buffer, BUFFER_SIZE, fd);
  // read next line
  fgets(buffer, BUFFER_SIZE, fd);

  char bone_type[8], root_name[32];
  sscanf(buffer, " %s %s", bone_type, root_name);

  if (strcmp(bone_type, "ROOT")!=0 && strcmp(bone_type, "Root")!=0)
    {
      fprintf(stderr, "The first node must be a root node\n");
      return false;
    }
  else
    {
      BoneNode rn(-1, root_name);
      rn.offset = ReadOffset(fd, buffer);
      assert(GetNumChannels(fd, buffer) == 6);

      _skeleton.push_back(rn);
    }

  // recursively parse all subsequent nodes
  while(ParseNode(fd, 0, buffer) !=0);

  // Debug printf: -> TODO: somehow adding these printfs crashes the program quite often
  /*
  for(unsigned int i=0; i< _skeleton.size(); i++)
    {
      printf("%2d BoneName: %-13s, ParentName: %-13s, Offset: %7.4f %7.4f %7.4f\n", i,
	     _skeleton[i].name.c_str(), _skeleton[_skeleton[i].parentIdx].name.c_str(),
	     _skeleton[i].offset.x, _skeleton[i].offset.y, _skeleton[i].offset.z);
    }
*/
  return true;
}

int BvhSkeleton::ParseNode(FILE*& fd, const int parentIdx, char* buffer)
{
  // What is it?
  fgets(buffer, BUFFER_SIZE, fd);
  char delim;
  sscanf(buffer, " %c", &delim);

  if (delim == '}')
    return 0;

  char bone_type[8], bone_name[32];
  sscanf(buffer, " %s %s", bone_type, bone_name);
  if(strcmp(bone_type, "END")==0 || strcmp(bone_type, "End")==0)
    {
      fgets(buffer, BUFFER_SIZE, fd);
      ReadOffset(fd, buffer);
      fgets(buffer, BUFFER_SIZE, fd);
      return 1;
    }
  else if (strcmp(bone_type, "JOINT")==0)
    {
      // Create a new node with the joint name and offset
      BoneNode bn(parentIdx, bone_name);
      bn.offset = ReadOffset(fd, buffer);
      assert(GetNumChannels(fd, buffer) == 3);
      int curIdx = _skeleton.size();
      _skeleton[parentIdx].childIdx.push_back(curIdx);
      _skeleton.push_back(bn);
//      printf("Joint: %s added\n", bone_name);
      while(ParseNode(fd, curIdx, buffer) != 0);
    }
  return 1;
}

Triplet BvhSkeleton::ReadOffset(FILE*& fd, char* buffer)
{
  do
    {
      fscanf(fd, " %s", buffer);
    }while(strcmp(buffer, "OFFSET")!=0 && strcmp(buffer, "Offset")!=0);

  Triplet tr;
  fscanf(fd, " %f %f %f", &tr.x, &tr.y, &tr.z);
  fgets(buffer, BUFFER_SIZE, fd);

  return tr;
}

int BvhSkeleton::GetNumChannels(FILE*& fd, char* buffer)
{
  int m;
  do{
    fscanf(fd, " %s", buffer);
  }while(strcmp(buffer, "CHANNELS")!=0 && strcmp(buffer, "Channels")!=0);

  fscanf(fd, " %d", &m);
  fgets(buffer, BUFFER_SIZE, fd);
  return m;
}

/*******************************************************************************
BvhAnimation Class
*******************************************************************************/
void BvhAnimation::ParseAnimation(FILE*& fd, const int numJoints,
				                       std::vector<bool>& mask)
{
  unsigned int frames;
  double frameRate;
  char buffer[BUFFER_SIZE];

  char token[32];
  do{
    fgets(buffer, BUFFER_SIZE, fd);
    sscanf(buffer, " %s", token); 
  }while(strcmp(token, "MOTION")!=0);

  fgets(buffer, BUFFER_SIZE, fd);
  sscanf(buffer, " %s %u", token, &frames); // token== "Frames:"

  fgets(buffer, BUFFER_SIZE, fd);
  sscanf(buffer, " %s %lf", token,  &frameRate);

  // Down sample the frame rate
  _sample_rate = frameRate * _skip_frame;

  // Allocate space for root trajectory and channel angles
  _root_traj.reserve(frames/_skip_frame);
  _frames.resize(frames/_skip_frame +1);

  for(unsigned int i=0; i< _frames.size(); i++)
    _frames[i].reserve(numJoints);

  float x, y, z;      // Root offset
  float rx, ry, rz;   // Joint rotation
  unsigned int frame_counter = 0;
  for(unsigned int i=0; i< frames; i++)
    {
      // down sample
      if ( (i+1) % _skip_frame == 0)
      {
    	  // Read in the root offset;
    	  fscanf(fd, " %f %f %f", &x, &y, &z);
    	  _root_traj.push_back(Triplet(x, y, z));

    	  // For all the joints, read in the animation
    	  // but do not waste capacity on inactive joints

    	  for(unsigned int j=0; j< mask.size(); j++)
    	  {
    		  // Assume the bvh file uses the Z->Y->X convention
    		  fscanf(fd, " %f %f %f", &rz, &ry, &rx);

    		  if (mask[j])
    		  {
    			  Triplet rot(rx,ry,rz);
    			  _frames[frame_counter].push_back(rot);
    		  }
    	  }
	  ++frame_counter;
      }
      // Finish current line and move on to the next one
      char* pc;
      do{
    	  fgets(buffer, BUFFER_SIZE, fd);
    	  pc = strrchr(buffer, '\0');
      }while(*(pc-1) !='\n');
    }
}

/*******************************************************************************
BvhLoader Class
 ******************************************************************************/
/*
bool BvhLoader::LoadBvhSkeleton(const char* filepath)
{
	FILE* fd = fopen(filepath, "r");

	if(fd ==0)
	{
		fprintf(stderr, "Error loading file %s\n", filepath);
		return false;
	}

	// Must read-in the joints in order to map joints to channel index
	_skel.ParseSkeleton(fd);

	fclose(fd);
	fd = 0;
	return true;
}
*/
bool BvhLoader::LoadBvhMotion(const char* filepath,
		const BvhSkeleton& ref_skel,
		std::vector<unsigned int>& dict, int skip)
{
  FILE* fd = fopen(filepath, "r");
  
  if(fd ==0)
    {
      fprintf(stderr, "Error loading file %s\n", filepath);
      return false;
    }

  // Must read-in the joints in order to map joints to channel index
  _skel.ParseSkeleton(fd);

  printf("Done loading skeleton\n");

  // down sample by 8.
  _anim._skip_frame = skip;

  std::vector<bool> mask;
  MakeMasknMap(mask, ref_skel._skeleton, dict);

  _anim.ParseAnimation(fd, ref_skel._skeleton.size(), mask);

  fclose(fd);
  fd = NULL;
  return true;
}

// Somewhat inefficient way to build correspondence between two skeletons
void BvhLoader::MakeMasknMap(std::vector<bool>& mask,
		const std::vector<BoneNode>& ref_skel,
		std::vector<unsigned int>& dict)
{
	// first make a map for its own:
	std::map<string, unsigned int> temp_dict;
	mask.resize(_skel._skeleton.size());
	for (unsigned int i=0; i<_skel._skeleton.size(); i++)
	{
		temp_dict[_skel._skeleton[i].name] = i;
		mask[i] = false;
	}

	// first pass build the mask
	for (unsigned int j=0; j< ref_skel.size(); j++)
	{
		// if a correspondence is found, that bit of the mask is set to true
		if (temp_dict.find(ref_skel[j].name) != temp_dict.end())
		{
			unsigned int idx = temp_dict[ref_skel[j].name];
			mask[idx] = true;
		}
	}

	// second pass build an intermediate representation:
	std::vector<unsigned int> accumulator;
	accumulator.resize(mask.size());
	unsigned int counter =0;
	for (unsigned int i=0; i< mask.size(); i++)
	{
//		printf("Mask[%d] = %u\n", i, mask[i]);
		if (mask[i])
		{
			accumulator[i] = counter++;
		}
//		printf("Acc[%u]=%u\n", i, accumulator[i]);
	}

	dict.resize(ref_skel.size());
	// third pass builds the dictionary
	for (unsigned int j=0; j< ref_skel.size(); j++)
	{
		// if a correspondence is found, that bit of the mask is set to true
		if (temp_dict.find(ref_skel[j].name) != temp_dict.end())
		{
			dict[j] = accumulator[temp_dict[ref_skel[j].name]];
		}

		// printf("Ref:%u -> Target:%u\n", j, dict[j]);
	}
}
