============================================== How to Build =======================================================
1- Go to the root directory, where CMakeLists.txt resides.

Using VSCode:
2- Open VSCode. Make sure to install these extensions by pressing F1 > Install Extensions:
	C/C++ [by Microsoft]
	CMake [by twxs]
	CMake Tools [by Microsoft]
3- Press F1 > CMake: Configure. Use the given CMake file in the project to build.

Using Command Line:
2- Open commandline and cd to CMakeLists.txt directory
3- Execute the following command (while changing to your respective paths):
	"C:\Program Files\CMake\bin\cmake.EXE" --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -H d:/Programming/Github/EE569_HW1 -B d:/Programming/Github/EE569_HW1/build -G "Visual Studio 16 2019"

4- Below is how you can run the executables with arguments from the command line.
	
=============================================== How to Run ========================================================
1- Open commandline and cd to the executable directory.
2- Execute the command from the respective assignment. For example, for Q1, run:
	.\EE569_HW3_Q1.exe Forky 328 328 3
3- Profit!!!
	
=========================================== Notes on Arguments ====================================================
1- The file paths can be either relative to the executable or absolute paths.
2- The image filenames SHOULD NOT include an extension like .raw, the program will add that automatically.
3- All arguments are mandatory, some may have defaults.

================================================== Q1 ============================================================
Arguments:
    programName inputFilenameNoExtension width height channels
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q1.exe Forky 328 328 3
    .\EE569_HW3_Q1.exe 22 328 328 3

================================================== Q2a ============================================================
Arguments:
    programName leftInputFilenameNoExtension middleInputFilenameNoExtension rightInputFilenameNoExtension width height channels
    *InputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q2.exe left middle right 576 432 3
	
================================================== Q3a ============================================================
Arguments:
    programName inputFilenameNoExtension width height channels
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q3a.exe spring 252 252 1
    .\EE569_HW3_Q3a.exe flower 247 247 1
    .\EE569_HW3_Q3a.exe jar 252 252 1

================================================== Q3b ============================================================
Arguments:
    programName inputFilenameNoExtension width height channels [defectSizeThreshold=50]
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q3b.exe deer 550 691 1 50

================================================== Q3c ============================================================
Arguments:
    programName inputFilenameNoExtension width height channels
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q3c.exe beans 494 82 3