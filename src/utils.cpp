#include "utils.h"

std::string working_dir;
std::string vapm_dir;

std::string project_name = "project";
std::vector<std::string> available_task_list;
ryml::Tree vapm_yaml_tree;
ryml::NodeRef vapm_yaml_tasks;

bool matchPaths(fs::path& path1,std::vector<std::string>& matchArray){
	std::string pathString = path1.string();
	for(std::string match : matchArray){
		if (pathString.find(match) != std::string::npos) {
		   return true;
		}
	}
	return false;
}

bool endsWith(std::string const & value, std::vector<std::string> const & endings){
	for(std::string end : endings){
		if(endsWith(value,end)){
			return true;
		}
	}

	return false;
}

int exec(const char * cmd,std::string * result,FILE * outputStream){
	char buffer[128];
	buffer[127] = 0; // setup null terminated buffer.

	*result = "";
	FILE * pipe = popen(cmd, "r");
	if (pipe == NULL){
		printf("Unable to open pipe with command: %s\n",cmd);
		fflush(stdout);
		assert("popen failed!");
	}
	try {
		long readCount = 0;
		while( (readCount = fread(buffer,sizeof(char),sizeof(buffer)-1, pipe)) ){
			*result += buffer;
			if(outputStream != 0){
				fwrite(buffer,sizeof(char),readCount,outputStream);
			}

			if(feof(pipe) == EOF || ferror(pipe) != 0){
				break;
			}
		}
	} catch (...) {
		return pclose(pipe);
	}
	return pclose(pipe);
}

std::vector<std::string> sequenceToVector(ryml::NodeRef seq){
	std::vector<std::string> result;
	std::string keyName = csubstrToStr(seq.key());

	if(seq.is_seq()){
		for(ryml::NodeRef seqNode : seq.children()){
			if(seqNode.has_val()){
				result.push_back(csubstrToStr(seqNode.val()));
			}
		}
	}else{
		printf("(WARNING) Malformed YAML: %s should be a sequence inside %s/vapm.yaml\n",keyName.c_str(),working_dir.c_str());
	}
	return result;
}

std::string getCurrentWorkingDir(){
	char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	std::string current_working_dir(buff);
	return current_working_dir;
}

// Windows only.
std::string getExecutableDir(){ // returns path to vapm.exe
	#ifdef _WIN32
		HMODULE hModule = GetModuleHandle(NULL);
		if (hModule != NULL){
			char ownPath[2048];
			GetModuleFileName(hModule, ownPath, 2048); 
			return fs::path(std::string(ownPath)).parent_path().string();
		}else{
			return "";
		}
	#else
		constexpr unsigned int PATH_MAX_LENGTH = 2048;
		char ownPath[ PATH_MAX_LENGTH ];
 		ssize_t count = readlink( "/proc/self/exe", ownPath, PATH_MAX_LENGTH );
 		if(count > 0){
			return fs::path(std::string(ownPath)).parent_path().string();
		}else{
			return "";
		}
	#endif
}

std::string getFileContent(FILE * handle){
	fseek(handle, 0, SEEK_END);
	
	long long int fsize;
	#ifdef _WIN32 // this works 100% even on files containing non unicode characters
		fgetpos(handle,&fsize);
	#else // this is untested.
		fsize = ftell(handle);
	#endif

	char * fileContent = new char[fsize];
	char c;
	unsigned int index = 0;
	fseek(handle, 0, SEEK_SET);
	while((c = fgetc(handle)) != EOF){
		fileContent[index] = c;
		index ++;
	}

	std::string result = std::string(fileContent,index);
	delete[] fileContent;
	return result;
}