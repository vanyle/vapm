#include "setup.h"

#include <cstdio>
#include <cstdlib>

int copy_file(const fs::path& src, const fs::path& target,std::string replace){

	// printf("File copy: %s -> %s\n",src.string().c_str(),target.string().c_str());
	
	FILE * fd_to;
	FILE * fd_from;

	fd_from = fopen(src.string().c_str(), "rb");
	if (!fd_from){
		return -1;
	}

	fd_to = fopen(target.string().c_str(), "wb+");
	if (!fd_to){
		fclose(fd_from);
		return -1;
	}
	
	long long int fsize;
	long long int fpos = 0;

	fseek(fd_from, 0, SEEK_END);
	#ifdef _WIN32 // this works 100% even on files containing non unicode characters
		fgetpos(fd_from,&fsize);
	#else // this is untested.
		fsize = ftell(fd_from);
	#endif
	
	fseek(fd_from, 0, SEEK_SET);

	while (fpos < fsize){
		fputc(fgetc(fd_from), fd_to);
		fpos++;
	}

	fclose(fd_to);
	fclose(fd_from);

	return 0;
}

void recursive_copy(const fs::path& src, const fs::path& target,std::string replace){
	if(!fs::exists(src)){
		return;
	}

	if(fs::is_regular_file(src)){

		int error_code = copy_file(src,target,replace);
		if(error_code != 0){
			printf("Issue while copying %s\n",src.string().c_str());
		}

	}else if(fs::is_directory(src)){
		fs::create_directories(target);

		for(const auto& dirEntry : fs::directory_iterator(src)){ // directory_iterator skips . and ..
			const auto& p = dirEntry.path();
			const auto targetPath = target / p.filename();

			recursive_copy(p,targetPath,replace);
		}
	}
}

int setupProject(std::string newProjectName,std::string projectType){

	fs::path vapmPath = fs::path(working_dir) / fs::path("vapm.yaml");

	if(fs::exists(vapmPath)){
		printf("Can't create project inside a directory that is a project itself.\n");
		return 1;
	}

	fs::path projectTemplatePath = fs::path(vapm_dir) / "default_projects" / projectType;
	fs::path projectDestPath = fs::path(working_dir) / newProjectName;

	// Check if projectType is valid
	if(!fs::exists(projectTemplatePath)){
		printf("The project type %s does not exist. No template found at %s\n",projectType.c_str(),projectTemplatePath.string().c_str());
		return 1;
	}
	if(fs::exists(projectDestPath)){
		printf("The project %s already exists at %s\n",newProjectName.c_str(),projectDestPath.string().c_str());
		return 1;
	}

	printf("Creating project: %s at %s of type %s\n",newProjectName.c_str(),projectDestPath.string().c_str(),projectType.c_str());
	
	recursive_copy(projectTemplatePath,projectDestPath,newProjectName);

	printf("Done.\n");
	fflush(stdout);
	return 0;
}