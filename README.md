# VAPM

VAPM (vanyle's awesome project manager) is a project manager for windows and linux.
Get VAPM using the python installer provided or just grab the `.exe` located in bin if you are on windows.
VAPM is designed to handle C++ and Web projects but can be used for anything.

Features:
 - an easy build system to get started with C++
 - an automatic documentation system

You can add files to your project without changing the build settings or anything. Just focus on the code, not the building process.
Don't use a Turing-complete sub-language to build your C++, just use simple YAML config files.

## Installation / Getting Started

To use vapm, just run the installer with python 3. I would also recommand adding `vapm/bin` to your PATH.
Follow the instructions displayed on the screen to make vapm work. It will walk you through it's installation.

Download: https://www.python.org/downloads/

Also installing [gdbgui](https://www.gdbgui.com/) (install with `pip install gdbgui`) can help with debugging. (to run debugging vapm tasks)

VAPM uses YAML for it's files. YAML is quite to learn and intuitive to use but if you've never seen to before, you might want quickly glance through this:
https://learnxinyminutes.com/docs/yaml/


## Creating new projects with vapm


When, to create a new vapm project, just type into a terminal:

`vapm new <project_type> <project_name>`<br/>
with `<project_type>` being `cpp_window` or `cpp_console` or anything inside the `default_projects` directory


This will create a new directory called `<project_name>` that contains the default layout for a `<project_type>` project.


When inside the new project directory, you can type:

`vapm task <list of tasks names>`

Like for example: `vapm task clean release run`
This will build your project in release mode from scratch and run it.

## Using existing vapm projects

List of common task names you can expect to find in (most C++) projects:
 - run
 - run_debug
 - release # build release for windows
 - debug
 - debugger # lauches gdb / gdbgui on your project
 - clean # Empties the build directory
 - doc # Build a documentation for the project
 - build_release_linux
 - test # run the tests

Of course, any project can have any tasks, those are just common names (and the recommended naming convention) when using vapm.

## Building projects with vapm

Let's say you have a `/src` directory containing your code inside your project directory (usually C++ but can be something else.) You can run `vapm task` and execute the default task inside
`vapm.yaml` which will usually compile the code. For an existing project, a good default starter file is described below.

### Options inside vapm.yaml

A starting building configuration generated will look something like this:

```yaml
# Every path inside vapm.yaml is absolute or relative to vapm.yaml
# So, it behaves exaclty the same way to matter where your current working directory is.
name: Project name # optional
version: 1.0 # optional
author: some guy # optional
tasks:
    default: # Describes default behaviour when calling vapm build
        type: build
        source_path: src # this is relative to vapm.yaml
        output_path: build
        output_file: a.exe
        linker_arguments: # Arguments given to the linker
        	- -Llibs
        	#- -lgdi32 # needed for most gui application	
         	# - -lmylibrary
            # etc...
        ignore: [] # Directories / Files inside src to not build
        arguments: # Arguments given to the compiler
            - -Wall
            - -g
            - -O0
            - -Isrc
           #- -Iinclude # used for access to libraries headers
    
```


This describes a simple debug build (`-Os` and `-g` are used) named default. Because *vapm* can execute scripts to build stuff, we need to say we want to build hence the line `type: build`. You can describe multiple possible tasks inside a build file (the build file above only has the default task).

```yaml
tasks:
    debug:
        type: build
        # ... same as before
        arguments:
            - -Wall
            - -g
            - -O0
    release:
        type: build
        # ...
        arguments:
            - Os
            - -Wall
    run:
        type: command
        timer: true # vbuild can time your execution if you want.
        commands:
            - build/a.exe
    build_and_run:
        type: task # tasks are a bundle of tasks to execute in order.
        # If a task executes itself, the build will never finish.
        tasks:
            - release
            - run
```

To start building, do: `vapm task <tasks to perform>` If no task is provided, the task named `default` will be run. If a given task is not defined inside `build.yaml`, vapm will complain.

Note that these three will have the same effect: 
 - `vapm task release & vapm build run`
 - `vapm task release run`
 - `vapm task build_and_run`


If type is not given, the type of an task is `build` by default.

Available fields for a build task

- output_file
- source_path
- output_path
- ignore (list of directories inside source_path to ignore)
- print_commands (prints which commands are ran)
- linker (g++ by default)
- compiler (g++ by default)
- arguments
- linker_arguments
- debug (used to display for info about the building process like which files are being processed, tasks executed, ... mostly used if you are developing vapm or is curious about what is going on if print_command is not enough for you)
- timer (boolean, prints the duration of the build if true, useful to brag about how slow the C++ compile times are)
- output_delimiter (-o by default, used if you are not using c++ but something else)
- output_file_extension (.o by default)
- input_file_extension (list of allowed file extension to compile. `.c` and `.cpp` by default)


Often, your release and debug builds will be very similar but with only some minor differences. To shorten your vapm file, you can write:

```yaml
tasks:
  release:
    type: build
    timer: true
    # Some arguments ...
    ...
  debug:
    copy: release # same as release but some fields here are overwritten
    output_file: executable/vapm_debug.exe
    arguments:
      # ...

```

Here, debug and release do the same thing but with sligtly different arguments for the compiler (`-Os` and `-O0` for example)

Let's say that you want to store your library options in a seperate file, you can write:

```yaml
tasks:
  release:
    type: build
    timer: true
    # Some arguments ...
    linker_arguments: @include "filename"

```

vapm will replace the `@include "filename"` with the content of the `filename` file (which should be located in the same directory as the vapm file.)
This is useful for custom scripts that can dynamically edit the libraries for your project without editing the `vapm.yaml` file.

### Using python scripts

```yaml
tasks:
	script_task:
		type: script
		name: print_arg.py # The name of the python file containing the script
		arg: hello

```

Python scripts are contained inside `vapm/python_scripts`. They receive as argument:
 - the path to the current project
 - the name of the current project
 - the content of the arg argument 
You can thing of scripts as commands that are relative to the vapm executable instead of the project.

### Building for something else then C++

Let's say you want to build a web project with some typescript files and sass files. Here is a possible `vapm.yaml `file for it:

```yaml
tasks:
    default: # Build your server and starts it.
        type: task
        tasks:
            - build_ts
            - build_sass
            - start
    build_ts: # No linker because tsc smartly figures out import statements.
        type: build
        compiler: "tsc"
        output_delimiter: "--outFile"
        output_extensions: js
        output_path: /build
        source_path: /src
        linker: "" # No linker means no output_file
        arguments:
            - --strict
        extensions: ["main.ts","client.ts"] # You probably only want to compile main and your client and use imports
    build_sass:
        type: build
        compiler: "sass"
        linker: ""
        output_path: /build
        source_path: /src
        output_delimiter: "" # -o by default.
        extensions: [".sass",".scss"]
    start:
        type: command
        commands:
            - run node /build/main.js
```

Using this file, you can do `vapm build start` to just run the server without rebuilding your project.


## Project Layout philosophy

Then creating a new project with `new`, a new directory with a bunch of files will be created.
This project structure is not required to use vapm, all you need is a `vapm.yaml` file.
However, this project structure is nice to work with. The purpose of this structure is decribed below.

This applies mostly for C++ but is also true for web projects, the only difference being that
with js projects, there should be no `include` folder and all libraries should be inside `lib`.
Also, don't use `npm`.

### General
A project should be able to be located anywhere on a computer. You could have all
your projects inside the same directory but this is not mandatory.

### Libraries (lib and include)
A project should contain all the libraries that it uses that are not provided by default by vapm. They should be located inside
the `lib` folder for static libraries and in the `include` folder for header-only libraries, uncompiled libraries with C++ code and the headers for the lib files.
The headers of the project should be inside `src` except if the project is a library itself.
Inside the lib folder, every library should be seperated inside its own folder and libraries should do includes only with relative paths

### Source (src)
A project should contain all its code inside a `src` directory with sub-folders inside it if more organization
is needed for bigger projects. This directory should not contain any asset and be trackable by a versionning program.

### Builds (executable)
A project should contain all the releases ie the thing that can be shipped inside the `executable` directory. This includes the assets
of the program (image, source, everything). Debug and release build executable should be putted here.
If building the project requires to produce intermediate code representation / file, these should be located inside `build`. This folder
should also contain any debug file like `.pdb` files. The build system should only contain temporary auto-generated files because
its content should be able to be removed to do a clean build

### Documentation (doc)
The documentation should be contained inside `doc`. You should be able to auto-generate the documentation of your project with `vapm task doc`
VAPM contains out of the box an auto-documentation system but you can use your own one like `doxygen` if you want.
The vapm documentation system build the code inside your documentation to make sure it is always up to date.

### Other
If your project needs other things to be **MANAGED**, these should be contained inside the top level directory along side the `vapm.yaml` file.
For example: `.gitignore`, `.vs`, `name.sublime-project`, etc...
If your project needs other things to be **RAN**, these should be contained inside the `executable` directory. The project should work only with
the executable directory.

## Custom building features

A demo of how the documentation script works

```cpp

// This is inside a .cpp file

/**
Overview
This is a vector library that does vector stuff.
Some more text.
Give an example of a small use case.
Write good documentation, don't just restate the name of the function / parameters when documenting.
*/

class Vector {
	float x;
	float y;
}

// a documentation comment for the add method
// a documentation comment MUST be followed by a method/variable declaration.
// documentation is not aware of namespaces, so when using a namespace, mention it in the library overview or something.
/**
Adds 2 vector or something (write good documentation, not like this)
Example: (example is real code that will be attempted to be compiled)
If example cannot be compiled, an error will appear when generating the doc and a note that this code is not up to date
fill appear below
@code
#include "vector.h"

int main(){
	Vector v;
	v.add();
	return 0;
}
@endcode
*/

Vector::add(){

}


```

## Default cpp libraries

These libraries have mainly been chosen to do game developpement (glfw or lua for example)
but also contain libraries useful for any kind of project (qt or sqlite3)

- GLFW (window management)
- GLEW (opengl bindingw)
- ZLIB (data compression/decompression)
- SNDFILE (sound decoding)
- portaudio (sound playing)
- SDL2 (game stuff)
- RYML (json parsing)
- sfml (game stuff)
- violet (game engine)
- qt5 (gui)
- opencv (image recognition)
- lua (scripting language)
- imgui (gui)
- freetype (font rendering)
- assimp (3d model import)
- ... (many other)

Be careful, some of these libraries cannot be used in proprietary project without paying a license