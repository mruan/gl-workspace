#include "bvh_utils.hpp"
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
void BvhAnimation::ParseAnimation(FILE*& fd, int skip_frame, int ds_date, int numJoints)
{
  // Do not convert to radian just yet
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

  // Allocate space for root trajectory and channel angles
  _root_traj.reserve((frames - skip_frame) / ds_rate);
  _frames.resize((frames - skip_frame) / ds_rate);

  for(int i=0; i< _frames.size(); i++)
    _frames[i].reserve(numJoints);

  // skip a couple of frames
  for(int i=0; i < skip_frame; i++)
    {
      // it simply reads the whole line
      do{
    	  fgets(buffer, BUFFER_SIZE, fd);
    	  pc = strrchr(buffer, '\0');
      }while(*(pc-1) !='\n');
    }

  Triplet root;      // Root offset
  Triplet angle;   // Joint rotation
  unsigned int frame_counter = 0;
  for(int i=0;i<_frames.size(); i++)
    {
      if (i% ds_rate==0)
	{
	  fscanf(fd, " %f %f %f", &root.x, &root.y, &root.z);
	  _frames[frame_counter].root_offset = root;

	  for(int j=0; j < numJoints; j++)
	    {
	      fscanf(fd, " %f %f %f", &angle.z, &angle.y, &angle.x);
	      _frames[frame_counter].joint_angles.push_back(angle);
	    }
	  ++frame_counter;
	}
      // it reads the rest of the line line
      do{
    	  fgets(buffer, BUFFER_SIZE, fd);
    	  pc = strrchr(buffer, '\0');
      }while(*(pc-1) !='\n');
    }
}

void BvhAnimation::WriteAnimation(FILE*& fd, std::vector<int>& dict)
{
  // for each frame...
  for(int i=0; i<_frames.size(); i++)
    {
      BvhFrame& f = _frames[i];
      fprintf(fd, "%7.4f %7.4f %7.4f ", f.root_offset.x, f.root_offset.y, f.root_offset.z);

      // for each joint that I want...
      for(j=0; j< dict.size(); j++)
	{
	  Triplet& t = f.joint_angles[j];
	  // NOTE: the order to euler angle is same as when read in
	  fprintf(fd, "%7.4f %7.4f %7.4f ", t.z, t.y, t.x);
	}
      fprintf(fd, "\n");
    }
}


/*******************************************************************************
BvhLoader Class
 ******************************************************************************/
void BvhLoad::LoadnDump(FILE* load_fd, FILE* dump_fd, const BvhSkeleton* pref_skel)
{
  // if we are loading the skeleton for the first time
  if (pref_skel != 0)
    {
      // Must read-in the joints in order to map joints to channel index
      _skel.LoadfromFD(load_fd);
      
      printf("Done loading skeleton\n");
      
      MakeDict(*ref_skel);
    }

  BvhAnimation* pBvhAnim = new BvhAnimation;
  pBvhAnim->ParseAnimation(load_fd);

  // dump
  pBvhAnim->WriteAnimation(dump_fd, _boneDict);
  delete pBvhAnim;
}

void BvhLoader::MakeDict(const BvhSkeleton& ref_skel)
{
  // first make a map for its own:
  std::map<string, unsigned int> temp_dict;
  mask.resize(_skel.mBones());
  for (int i=0; i<_skel.mBones(); i++)
    {
      temp_dict[_skel[i].name] = i;
      mask[i] = false;
    }
  
  _boneDict.resize(ref_skel.mBones());
  for (int i=0; i< _boneDict.size(); i++)
    {
      std::string& name = ref_skel[i].name;
      int index = temp_dict[name];
      _boneDict[i] = index;
    }
}
