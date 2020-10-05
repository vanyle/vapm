#include <iostream>
#include <cstdio>
#include <cassert>

std::string getFileContent(FILE * handle){
	fseek(handle, 0, SEEK_END);
	long long int fsize;
	fsize = ftell(handle);
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

int main(int argc,char ** argv){
	std::cout << "file.txt:" << std::endl;
	FILE * handle = fopen("file.txt","r");
	if(handle){
		std::cout << getFileContent(handle) << std::endl;
		fclose(handle);
	}else{
		std::cout << "Unable to open file.txt" << std::endl;
	}
}