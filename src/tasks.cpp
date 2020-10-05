#include "tasks.h"

#include <chrono>
// Code used for the build task:

struct BuildTask
{
	std::vector<std::string> source_directory_path = {"src"};
	std::string output_directory_path = "build";
	std::string output_file = "a.exe";
	std::vector<std::string> compiler_arguments = {"-Wall", "-g", "-O0"};
	std::vector<std::string> linker_arguments = {}; // linker is well made, no arguments is a good default

	std::vector<std::string> ignore_paths;
	bool print_commands = true;
	bool debug = false;
	bool timer = false;
	bool color = true;
	std::string compiler = "g++";
	std::string linker = "g++";

	std::string output_delimiter = "-o";
	std::string output_file_extension = ".o"; // .o (for linker)
	std::vector<std::string> input_file_extension = {".cpp", ".c"};
};

// Recursive, returns true if success
bool buildFilesInDirectory(std::string rootSrc, fs::path directoryPathRelative, BuildTask *bt, std::vector<std::string> &objFilesGen)
{
	// check if the same directory exist inside build and create it if this is not the case.
	fs::path srcDirectoryPathAbsolute = fs::path(working_dir) / rootSrc / directoryPathRelative;
	fs::path outputDirectoryPathAbsolute = fs::path(working_dir) / bt->output_directory_path / directoryPathRelative;

	if (!fs::exists(srcDirectoryPathAbsolute))
	{
		printf("Source directory for project %s not found at %s.\n Unable to build.\n\n",
			   project_name.c_str(),
			   srcDirectoryPathAbsolute.string().c_str());
		return false;
	}
	if (!fs::exists(outputDirectoryPathAbsolute))
	{
		if (bt->debug)
		{
			printf("%s does not exist, creating it.", outputDirectoryPathAbsolute.string().c_str());
		}
		fs::create_directory(outputDirectoryPathAbsolute);
	}

	for (fs::directory_entry it : fs::directory_iterator(srcDirectoryPathAbsolute))
	{
		if (fs::is_directory(it.path()))
		{

			// explore if not inside ignore:
			fs::path newPath = directoryPathRelative / it.path().filename();
			if (!matchPaths(newPath, bt->ignore_paths))
			{
				if (bt->debug)
				{
					printf("Exploring %s\n", newPath.string().c_str());
				}
				bool isEverythingOk = buildFilesInDirectory(rootSrc, newPath, bt, objFilesGen);
				if (!isEverythingOk)
				{
					return false;
				}
			}
			else if (bt->debug)
			{
				printf("Ignoring %s (blacklisted)\n", newPath.string().c_str());
			}
		}
		else if (fs::is_regular_file(it.path()))
		{

			std::string inPath = it.path().string();
			std::string outPath = (outputDirectoryPathAbsolute / it.path().stem()).string() + bt->output_file_extension; // .cpp -> .o (usually)

			if (bt->debug)
			{
				printf("Found: %s\n", inPath.c_str());
			}

			if (endsWith(inPath, bt->input_file_extension))
			{ // add option for this in yaml file.

				// Rebuild only if: outPath does not exist or outPath is older than inPath modification time

				if (fs::exists(fs::path(outPath)))
				{

					fs::file_time_type sourceTime = fs::last_write_time(fs::path(inPath));
					fs::file_time_type buildTime = fs::last_write_time(fs::path(outPath));
					// typeof ftime is std::chrono::time_point</*trivial-clock*/>
					if (buildTime > sourceTime)
					{ // build is more recent than source.
						bool forceRebuild = false;

						// TODO: don't do this if inPath is included by a file recently edited.
						// ie: read the content of inPath, indentify all #include "thing" and check the last time thing was edited.

						FILE *handle = fopen(inPath.c_str(), "r");
						if (handle)
						{
							// We load the entire yaml file into memory. This is not too bad as yaml files are supposted to be readable by humans so they shoul'nt be too long
							std::string fileContent = getFileContent(handle);
							fclose(handle);

							const std::string includeLabel = "#include \"";

							for (unsigned int i = 0; i < fileContent.size(); i++)
							{
								if (fileContent.compare(i, includeLabel.size(), includeLabel) == 0)
								{
									i += includeLabel.size();
									std::string filenamePath = "";
									while (i < fileContent.size() && fileContent[i] != '\"')
									{
										filenamePath += fileContent[i];
										i++;
									}
									// find filenamePath and check the last_write_time.
									fs::path includedFile = srcDirectoryPathAbsolute / filenamePath;
									if (bt->debug)
									{
										printf("File %s included %s\n", inPath.c_str(), includedFile.string().c_str());
									}
									if (fs::exists(includedFile))
									{
										try
										{
											fs::file_time_type sourceIncludeTime = fs::last_write_time(includedFile);
											if (buildTime < sourceIncludeTime)
											{
												forceRebuild = true;
												break; // from for loop
											}
										}
										catch (...)
										{
										}
									}
								}
							}
						}

						if (!forceRebuild)
						{
							objFilesGen.push_back(outPath);
							if (bt->debug)
							{
								printf("Skipping: %s (recently built)\n", inPath.c_str());
							}
							continue;
						}
					}
				}

				std::string compilerArguments = "";

				for (unsigned int i = 0; i < bt->compiler_arguments.size(); i++)
				{
					compilerArguments = compilerArguments + bt->compiler_arguments[i] + " ";
				}
				std::string command = bt->compiler + " -c " + compilerArguments + inPath + " " + bt->output_delimiter + " " + outPath;

				// #define ANSI_COLOR_MAGENTA \x1b[35m

				if (bt->print_commands)
				{
					printf("%s\n", command.c_str());
				}

				std::string result = "";
				int returnCode = exec(command.c_str(), &result); // 0 means ok
				printf("%s", result.c_str());

				// printf(ANSI_COLOR_GREEN "Done running.\n" ANSI_COLOR_RESET);

				if (returnCode != 0)
				{
					//printf("Build failed (return code: %s)\n",returnCode);
					return false;
				}
				else
				{
					objFilesGen.push_back(outPath);
				}
			}
		}
	}
	return true;
}

void doBuild(ryml::NodeRef currentTaskNode, std::string buildRequested)
{

	// recusivly explore the src dir and build stuff.

	std::vector<std::string> objectFilesGenerated = {};

	printf("Building %s with %s\n", project_name.c_str(), buildRequested.c_str());

	// generate build task
	BuildTask bt;

	if (currentTaskNode.has_child("source_path"))
	{
		ryml::NodeRef spNode = currentTaskNode["source_path"];
		if (spNode.is_seq())
		{
			retreiveNodeVal(currentTaskNode, &bt.source_directory_path, "source_path");
		}
		else
		{
			std::string source_path;
			retreiveNodeVal(currentTaskNode, &source_path, "source_path");
			bt.source_directory_path.clear();
			bt.source_directory_path.push_back(source_path);
		}
	}
	else
	{
	}

	// PATH="%PATH%%CD%\executable\mingw64\bin;"
	// std::string setup_path_command = "set PATH=%PATH%" + vapm_dir + "\\mingw64\\bin;";

	// Assume a compiler is in the path.
	bt.compiler = "g++";
	bt.linker = "g++";

	retreiveNodeVal(currentTaskNode, &bt.output_directory_path, "output_path");
	retreiveNodeVal(currentTaskNode, &bt.output_file, "output_file");
	retreiveNodeVal(currentTaskNode, &bt.compiler, "compiler");
	retreiveNodeVal(currentTaskNode, &bt.linker, "linker");
	retreiveNodeVal(currentTaskNode, &bt.output_delimiter, "output_delimiter");
	retreiveNodeVal(currentTaskNode, &bt.output_file_extension, "output_file_extension");
	retreiveNodeVal(currentTaskNode, &bt.print_commands, "print_commands");
	retreiveNodeVal(currentTaskNode, &bt.timer, "timer");
	retreiveNodeVal(currentTaskNode, &bt.debug, "debug");
	retreiveNodeVal(currentTaskNode, &bt.color, "color");

	retreiveNodeVal(currentTaskNode, &bt.compiler_arguments, "arguments");
	retreiveNodeVal(currentTaskNode, &bt.linker_arguments, "linker_arguments");
	retreiveNodeVal(currentTaskNode, &bt.input_file_extension, "input_file_extension");
	retreiveNodeVal(currentTaskNode, &bt.ignore_paths, "ignore");

	auto start = std::chrono::high_resolution_clock::now();
	bool success = true;

	for (unsigned int i = 0; i < bt.source_directory_path.size(); i++)
	{
		success = buildFilesInDirectory(bt.source_directory_path[i], fs::path("."), &bt, objectFilesGenerated);
		if (!success)
		{
			break;
		}
	}

#define ANSI_COLOR_GREEN "\x1b[32m" // color are optional because sublime text console does not render colors
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

	if (success)
	{ // linking.
		std::string objectPaths = "";
		for (unsigned int i = 0; i < objectFilesGenerated.size(); i++)
		{
			objectPaths += objectFilesGenerated[i] + " ";
		}
		for (unsigned int i = 0; i < bt.linker_arguments.size(); i++)
		{
			objectPaths += bt.linker_arguments[i] + " ";
		}

		if (bt.linker != "")
		{ // if linking required.

			std::string outputFilePath = (fs::path(working_dir) / bt.output_file).string();

			std::string command = bt.linker + " -o " + outputFilePath + " " + objectPaths;

			printf("%s\n", command.c_str());
			std::string result;
			int returnCode = exec(command.c_str(), &result); // 0 means ok
			printf("%s\n", result.c_str());

			if (returnCode == 0)
			{
				if (!bt.color)
				{
					printf("Build succeeded\n");
				}
				else
				{
					printf(ANSI_COLOR_GREEN);
					printf("Build succeeded\n");
					printf(ANSI_COLOR_RESET);
				}
			}
			else
			{
				if (!bt.color)
				{
					printf("Build failed (return code: %i)\n", returnCode);
				}
				else
				{
					printf(ANSI_COLOR_RED);
					printf("Build failed (return code: %i)\n", returnCode);
					printf(ANSI_COLOR_RESET);
				}
			}
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	if (bt.timer)
	{
		printf(" - Execution time: %.3fs\n\n", (double)(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.);
	}
}

// code used for other tasks including task management

void doCommand(ryml::NodeRef currentTaskNode, std::string taskName)
{
	std::vector<std::string> commands = sequenceToVector(currentTaskNode["commands"]);

	if (commands.size() <= 0)
	{
		printf("No commands to run for %s\n", taskName.c_str());
		return;
	}
	for (unsigned int i = 0; i < commands.size(); i++)
	{
		std::string commandToRun = commands[i];
		std::string stdoutResult;
		printf("%s\n", commandToRun.c_str());
		auto start = std::chrono::high_resolution_clock::now();

		int outputCode = exec(commandToRun.c_str(), &stdoutResult, stdout);
		printf("\n(return code: %i)\n", outputCode);
		auto end = std::chrono::high_resolution_clock::now();

		bool isTimer = false;
		retreiveNodeVal(currentTaskNode, &isTimer, "timer");
		if (isTimer)
		{
			printf(" - Execution time: %.3fs\n\n", (double)(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.);
		}
	}
}

void doScript(ryml::NodeRef currentTaskNode, std::string taskName)
{
	std::string script_name = "";
	std::string script_arg = "";
	bool isTimer = false;
	bool isDebug = false;

	retreiveNodeVal(currentTaskNode, &script_name, "name");
	retreiveNodeVal(currentTaskNode, &script_arg, "arg");
	retreiveNodeVal(currentTaskNode, &isTimer, "timer");
	retreiveNodeVal(currentTaskNode, &isDebug, "debug");

	std::string commandToRun = "python " + vapm_dir + "/python_scripts/" + script_name + " " + working_dir + " " + project_name + " " + script_arg;
	if (isDebug)
	{
		printf("%s\n", commandToRun.c_str());
	}

	std::string stdoutResult;

	auto start = std::chrono::high_resolution_clock::now();
	exec(commandToRun.c_str(), &stdoutResult, stdout);
	if (isTimer)
	{
		auto end = std::chrono::high_resolution_clock::now();
		printf(" - Execution time: %.3fs\n\n", (double)(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.);
	}
}

#include <map>

void doTask(std::string taskName)
{
	// retreive yaml object describing the task.
	ryml::NodeRef currentTaskNode;
	bool found = false;

	for (ryml::NodeRef taskSubNode : vapm_yaml_tasks.children())
	{
		std::string currentTaskName = csubstrToStr(taskSubNode.key());
		if (taskName == currentTaskName)
		{
			currentTaskNode = taskSubNode;
			found = true;
			break;
		}
	}

	if (!found)
	{
		printf("Unable to build %s, no task named '%s' found in vapm.yaml\n", project_name.c_str(), taskName.c_str());
		printf("Available options:\n");
		if (available_task_list.size() <= 0)
		{
			printf("  No action found inside vapm.yaml\n");
		}
		else
		{
			for (unsigned int i = 0; i < available_task_list.size(); i++)
			{
				printf(" '%s'\n", available_task_list[i].c_str());
			}
		}
		return;
	}

	{
		std::string copiedTask = "";
		retreiveNodeVal(currentTaskNode, &copiedTask, "copy");
		if (copiedTask != "")
		{
			// copy fields from the task to the currentTaskNode.
			ryml::NodeRef tocopyTaskNode;
			found = false;
			for (ryml::NodeRef taskSubNode : vapm_yaml_tasks.children())
			{
				std::string currentTaskName = csubstrToStr(taskSubNode.key());
				if (copiedTask == currentTaskName)
				{
					tocopyTaskNode = taskSubNode;
					found = true;
					break;
				}
			}
			// This error is fatal for the task:
			// If the copied field are not given, the default value might do something crazy in this context
			if (!found)
			{
				printf("Unable to copy task %s inside %s, the task does not exist.", copiedTask.c_str(), taskName.c_str());
				return;
			}

			for (ryml::NodeRef currentRef : tocopyTaskNode.children())
			{
				if (!currentTaskNode.has_child(currentRef.key()))
				{
					// if not found, copy it from tocopyTaskNode
					// shallow copy but this is fine.

					if (currentRef.is_seq())
					{
						ryml::NodeRef copyArray = currentTaskNode[currentRef.key()];
						copyArray |= ryml::SEQ;
						for (ryml::NodeRef seqNode : currentRef.children())
						{
							copyArray.append_child() << seqNode.val();
						}
					}
					else if (currentRef.is_keyval())
					{
						currentTaskNode[currentRef.key()] << currentRef.val();
					}
				}
			}
		}
	}
	// Analyse build type and perform the corresponding action.
	// Possible types are:
	// build (classic recusive file search)
	// task (bundle of other actions)
	// command (run bash command)

	std::string type = "build";
	retreiveNodeVal(currentTaskNode, &type, "type");

	if (type == "build")
	{
		doBuild(currentTaskNode, taskName);
	}
	else if (type == "task")
	{
		doTasks(sequenceToVector(currentTaskNode["tasks"]));
	}
	else if (type == "command")
	{
		doCommand(currentTaskNode, taskName);
	}
	else if (type == "script")
	{
		doScript(currentTaskNode, taskName);
	}
	else
	{
		printf("Unknown task type %s. Ignoring task %s", type.c_str(), taskName.c_str());
	}

	// After building, attempt to see if exported files where built.
	// If this is the case, add the path of the project to cache for other projects to require
	// The exported file do not depend on the build action. Fix this?
	// Add a tag for build actions: export:true/false to inform that the result of the build is exportable?
}

void doTasks(std::vector<std::string> listOfTasks)
{
	for (unsigned int i = 0; i < listOfTasks.size(); i++)
	{
		doTask(listOfTasks[i]);
	}
}