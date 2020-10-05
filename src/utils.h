#ifndef UTILS_H
#define UTILS_H

// Every cpp file of this project includes utils.h so this is where we put our globals.
// Their declaration is in utils.cpp

#include "ryml.hpp"
#include "ryml_std.hpp"

#include <vector>
#include <string>
#include <cassert>
#include <dirent.h>
#include <stdio.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;


#ifdef _WIN32
	#include <libloaderapi.h> // windows only, used to getExecutableDir
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif


// globals, should be mostly read-only.
extern std::string working_dir; // every path is either absolute or it is relative to this path
extern std::string vapm_dir; // path to vapm.exe used because vapm has a g++ compiler.

// globals used for tasks
extern std::string project_name;
extern std::vector<std::string> available_task_list;
extern ryml::Tree vapm_yaml_tree;
extern ryml::NodeRef vapm_yaml_tasks;

// requires a handle because the caller should manage errors if the file was not found.
std::string getFileContent(FILE * handle);
std::string getCurrentWorkingDir();
std::string getExecutableDir();

// Used for string comparison

bool matchPaths(fs::path& path1,std::vector<std::string>& matchArray);
inline bool endsWith(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
inline bool isTruthful(std::string s){
	// all other values are falsy.
	return s == "true" || s == "True" || s == "TRUE" || s == "yes" || s == "Yes" || s == "y" || s == "t" || s == "1";
}

bool endsWith(std::string const & value, std::vector<std::string> const & endings);

// Used for yaml decoding
inline std::string nodeRefToStr(ryml::NodeRef* n){
	return std::string(n->val().data(),0,n->val().len);
} 
inline std::string csubstrToStr(c4::csubstr s){
	return std::string(s.data(),0,s.len);
}
std::vector<std::string> sequenceToVector(ryml::NodeRef seq);

// writes to result if n[childName] exists
inline void retreiveNodeVal(ryml::NodeRef n,std::string * result,c4::csubstr childName){
	if(n.has_child(childName)){
		ryml::NodeRef nodeName = n[childName]; // O(n) complexity
		if(nodeName.has_val()){
			*result = nodeRefToStr(&nodeName); // global set
		}
	}
}
inline void retreiveNodeVal(ryml::NodeRef n,bool * result,c4::csubstr childName){
	if(n.has_child(childName)){
		ryml::NodeRef nodeName = n[childName]; // O(n) complexity
		if(nodeName.has_val()){
			*result = isTruthful(nodeRefToStr(&nodeName)); // global set
		}
	}
}
inline void retreiveNodeVal(ryml::NodeRef n,std::vector<std::string> * result,c4::csubstr childName){
	if(n.has_child(childName)){
		ryml::NodeRef seq = n[childName];
		if(seq.is_seq()){
			result->clear();
			for(ryml::NodeRef seqNode : seq.children()){
				if(seqNode.has_val()){
					result->push_back(csubstrToStr(seqNode.val()));
				}
			}
		}else{
			printf("(WARNING) Malformed YAML: %s should be a sequence inside %s/vapm.yaml\n",childName.data(),working_dir.c_str());
		}
	}
}

// windows/linux compatible was of running command. stdout gets put in result. Preserves terminal colors :)
int exec(const char* cmd,std::string * result,FILE * output = 0);

// windows only. Do not use, windows api ununderstandable, unable to detect timeout when reading stdout, stderr support missing 
// int exec2(const char* cmd,std::string * result);

#endif