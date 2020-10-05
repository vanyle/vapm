# D:\\msys64\\usr\\bin\\env MSYSTEM=MSYS /usr/bin/bash -li
# pacman -Qe # query installed packages
# pacman -Ss search
# pacman -S install
# pacman -Rsn remove

import sys
import os
import platform
import subprocess
import time
import pathlib

msysPath = ""

print("Hello. This script will build vapm for your OS.")
print("Note that this is an installation script and might edit your PATH variable. It will however not install anything.")
print("VAPM is supposed to work on windows and linux (we don't support MAC.)")
print()

def executePacmanCommand(command):
	if platform.system() == 'Windows':
		result = subprocess.Popen([os.path.join(msysPath,"usr/bin/env"),"MSYSTEM=MSYS","/usr/bin/bash","-li"],shell=True,stdout=subprocess.PIPE,stdin=subprocess.PIPE)
		time.sleep(2) # Wait for process to be ready.
		outs,errs = None,None
		try:
			outs, errs = result.communicate(input=str.encode(command+"\n","utf8"),timeout=2)
		except subprocess.TimeoutExpired:
			result.kill()
			outs, errs = result.communicate()
		if errs == None:
			errs = b""

		return outs.decode('utf8'),errs.decode('utf8')
	else:
		return None,"unavailable"

def isGccAvailable():
	result = subprocess.run("g++ --version",shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	return result.stderr == b"" 


# Complicated building stuff happens here.

objFiles = []
def performBuild():
	buildDirectory(".","src","build",False,["-Os","-Wall","-Isrc","-Iinclude","-isystemlib","-Wno-write-strings","-fdiagnostics-color","-D NDEBUG"])
	buildDirectory(".","include","build",False,["-Os","-Wall","-Isrc","-Iinclude","-isystemlib","-Wno-write-strings","-fdiagnostics-color","-D NDEBUG"])

	# linking command
	outputFile = "vapm.exe"
	if platform.system() == 'Linux':
		outputFile = "vapm"

	command = ["g++","-o","bin/"+outputFile]+objFiles+["-lstdc++fs"]
	if platform.system() == 'Windows':
		command += ["-lkernel32","-luser32"]
	print(' '.join(command))
	p = subprocess.run(command)
	if p.returncode != 0:
		print("")
		print("Linking failed. cpp_build terminating.")
		sys.exit(1)

	print()
	print()
	print("VAPM is ready to be used. (at bin/"+outputFile+") Add it to your path!")
	print("Open README.md for more info on how to use vapm !")


def buildDirectory(directory_relative_path,src_dir,dest_dir,verbose = False,arguments = []):
	current_full_path = os.path.join(src_dir,directory_relative_path)
	dirlst = os.listdir(current_full_path)
	for i in dirlst:
		current_file_path = pathlib.Path(os.path.join(current_full_path,i))
		if current_file_path.is_dir():
			try:
				os.mkdir(os.path.join(dest_dir,directory_relative_path,i))
			except:
				pass # Ignore errors when directory already exists.
			buildDirectory(os.path.join(directory_relative_path,i),src_dir,dest_dir,verbose,arguments)
		else:
			buildFile(os.path.join(directory_relative_path,i),src_dir,dest_dir,verbose,arguments)

def buildFile(file_relative_path,src_dir,dest_dir,verbose = False,arguments = []):
	input_file_path = os.path.normpath(os.path.join(src_dir,file_relative_path))
	output_file_path = os.path.normpath(os.path.join(dest_dir,file_relative_path))

	input_file_name = os.path.basename(input_file_path)

	if not input_file_name.endswith('.c') and not input_file_name.endswith('.cpp'):
		if verbose:
			print("Skipping ",input_file_path,' : Not a source file.',sep='')
		return
	# convert .cpp or .c file into .o file
	output_file_path = output_file_path.rsplit('.',1)[0] + '.o' 

	global objFiles
	objFiles.append(output_file_path)

	if verbose:
		print("Building",input_file_path,"to",output_file_path)
	# g++ -Wall -g -c file1.cpp
	command = ["g++"] + arguments + ["-c",input_file_path,"-o",output_file_path]
	print(' '.join(command))
	p = subprocess.run(command)
	if p.returncode != 0:
		print("")
		print("Compilation failed. cpp_build terminating.")
		sys.exit(1)








# ----------------------------

if platform.system() == 'Windows': # Windows / Darwin / Linux
	print("You are on Windows.")
	dirs = os.environ["PATH"].split(';')
	msysPath = ""
	found = False

	for el in dirs:
		if "msys" in el:
			found = True
			if msysPath == "" or len(el) < len(msysPath): # Pick the shortest path containing msys to get the root
				msysPath = el

	if not found:
		print("Unable to locate MSYS2 on your system.")
		print("If it is installed, please enter it's path below.")
		print("If not, install it from this link and rerun this script: https://www.msys2.org/#installation")
		msysPath = input("Enter the path to msys (Starting with C:/ or D:/ ): ")
		if msysPath == "":
			input("")
			sys.exit(0)

	# result,_ = executePacmanCommand("pacman -Qe")
	# print(result)

	if not isGccAvailable():
		print("g++ is not available on your system, install it first by running 'pacman -S mingw-w64-x86_64-gcc' in the console that just opened.")
		subprocess.run("msys2")
		input("") # Wait for user input before exiting
		sys.exit(0)

	performBuild()

elif platform.system() == 'Linux':
	print("You are on a UNIX based system.")
	
	if not isGccAvailable():
		print("g++ is not available on your system, install it first with 'sudo apt-get install build-essential'")
		sys.exit(0)
	performBuild()

else:
	print("VAPM is not available on your operating system.")