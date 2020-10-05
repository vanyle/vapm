#define VAPM_VERSION "1.0"
#define NOMINMAX // remove min / max macros defined in windows.h

#include <csignal>
#include <algorithm>
#include "utils.h"
#include "tasks.h"
#include "setup.h"

/**
Overview

VAPM (Vanyle's Awesome Project Manager) is a project manager made for C++ projects but it can also handle other languages<br/>
It needs Msys2 to work on windows and uses the MinGW compiler.

To install VAPM, just run the python script provided inside the zip file.
The python script will ask you if you want Msys2 to be installed and other stuff like that.

VAPM also works on linux as long as you have pacman installed.

<br/><br/>
VAPM is not a library and is not designed to be integrated in another C++ codebase.
*/

// Custom cpp/header file for this kind of functions?
/* aa */ void printHelp() {
	// -h -help
	// -i -info
	// -v -version
	// -s -scan
	printf("List of available options:\n");
	printf("\n");

	printf("task [list of task to perform]\n");
	printf("	Run the tasks listed\n");
	printf("\n");

	printf("new [project type] [project name]\n");
	printf("	Create a new vapm project at the current location.\n");
	printf("\n");

	printf("-h or -help\n");
	printf("	Display this list.\n");
	printf("\n");

	printf("-v or -version\n");
	printf("	Print the version of vapm.\n");
	printf("\n");

	printf("-i or -info or -c or -credit or -a or -about\n");
	printf("	Display credit.\n");
	printf("\n");

	fflush(stdout);
}

void yaml_crash(int sig) {
	printf("Unable to parse yaml file %s/vapm.yaml: It contains a yaml syntax error somewhere\n", working_dir.c_str());
	printf("(Note that yaml uses only spaces and no tabs for indentation, also, look out for unclosed quotes)\n");
	fflush(stdout);
	exit(sig);
}

int main(int argc, char** argv) {

	if (argc < 2) {
		printf("Not enough arguments provided.\n");
		printf("Usage:    vapm <action> [optional parameters]\n");
		printf("For info: vapm -help\n");
		fflush(stdout);
		return 1;
	}
	std::string firstargv = std::string(argv[1]);

	if (firstargv == "-help" || firstargv == "-h") {
		printHelp();
		return 0;
	}
	if (firstargv == "-version" || firstargv == "-v") {
		printf("vapm. version: %s\n", VAPM_VERSION);
		return 0;
	}
	if (firstargv == "-i" || firstargv == "-info" || firstargv == "-about" || firstargv == "-a" || firstargv == "-credit") {
		printf("vapm by vanyle. A project manager\n");
		return 0;
	}

	working_dir = getCurrentWorkingDir(); // global set
	vapm_dir = getExecutableDir();		  // global set

	if (firstargv == "new") {
		std::string newprojectName = "";
		if (argc < 4) {
			printf("No enough arguments provided for new:\n");
			printf("Usage: vapm new [project type] [project name]");
			return 1;
		}
		return setupProject(std::string(argv[3]), std::string(argv[2])); // defined in setup.cpp
	}

	if (firstargv != "task") {
		printf("Option '%s' not recognised.\nUse vapm -h to list available options.", argv[1]);
		return 1;
	}

	// Define globals used by task

	// A yaml file is required to procede.
	// Every path considered should be relative to the location of vapm.yaml in working_dir
	std::string yaml_path = working_dir + "/vapm.yaml";
	std::string fileContent;
	FILE* handle = fopen(yaml_path.c_str(), "r");
	if (handle) {
		// We load the entire yaml file into memory. This is not too bad as yaml files are supposted to be readable by humans so they shoul'nt be too long
		fileContent = getFileContent(handle);
		fclose(handle);
		// Preprocess includes before parsing.
		const std::string includeLabel = "@include \"";
		unsigned int labelStart = 0;

		for (unsigned int i = 0; i < fileContent.size(); i++) {
			if (fileContent.compare(i, includeLabel.size(), includeLabel) == 0) {
				labelStart = i;
				i += includeLabel.size();
				std::string filenamePath = "";
				while (i < fileContent.size() && fileContent[i] != '\"') {
					filenamePath += fileContent[i];
					i++;
				}
				std::string fullPath = (working_dir + "/" + filenamePath);
				handle = fopen(fullPath.c_str(), "r");
				if (handle) {
					std::string substitutionValue = getFileContent(handle);
					fclose(handle);
					i = std::min(i + 1, (unsigned int)fileContent.size());
					fileContent = fileContent.substr(0, labelStart) + substitutionValue + fileContent.substr(i, fileContent.size() - i);
					// (the replacing is recusive ie the file included can have includes itself. Infinite loops are possible, this is a simple sanity check to avoid them)
					if (fileContent.size() > 100'000) {
						printf("Max vapm.yaml size reached when including files. Stopping inclustion. You might have an inclusion loop somewhere.\n");
						break;
					}
				} else {
					printf("(WARNING) Unable to include the file %s. Does the file exist ?\n", fullPath.c_str());
					i = std::min(i, (unsigned int)fileContent.size());
					fileContent = fileContent.substr(0, labelStart) + fileContent.substr(i, fileContent.size() - i);
				}
			}
		}

		try {
			// This does not work. signal(ARBT) think neither. Don't know how to manage errors with this. Maybe look into c4/config.hpp or something ?
			vapm_yaml_tree = ryml::parse(c4::substr(&fileContent[0], (int)fileContent.size())); // global set
		} catch (...) {
			yaml_crash(1);
		}
	} else {
		printf("vapm.yaml file not found at %s\nCreate one to use vapm here.", yaml_path.c_str());
		return 1;
	}

	ryml::NodeRef root = vapm_yaml_tree.rootref();
	retreiveNodeVal(root, &project_name, "name");

	if (root.has_child("tasks")) {
		vapm_yaml_tasks = root["tasks"]; // global set
		for (ryml::NodeRef taskSubNode : vapm_yaml_tasks.children()) {
			std::string currentTaskName = csubstrToStr(taskSubNode.key());
			available_task_list.push_back(currentTaskName); // global set
		}
	} else {
		printf("No tasks listed inside %s\n", yaml_path.c_str());
		return 1;
	}

	std::vector<std::string> requestedTasks = { "default" };
	if (argc >= 3) {
		// arg0: vapm.exe
		// arg1: task
		// arg2..n: task list
		requestedTasks.clear();
		for (int i = 2; i < argc; i++) {
			requestedTasks.push_back(std::string(argv[i]));
		}
	}
	doTasks(requestedTasks); // defined in tasks.cpp

	fflush(stdout);
}