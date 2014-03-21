allegro_hand_windows
====================

This application is lightweight control and communication software for the Allegro Hand (AH) using Windows.
Currently this software only works with PEAK CAN-USB interfaces.

myAllegroHand*.exe:
 Allegro Hand control and CAN communication module.

rPanelManipulator.exe 
 GUI Interface for executing different control and grasp modes
 
 
Instructions
============ 

 1. Open up the solution, myAllegroHand.sln, in Visual Studio
 2. Right click the project 'myAllegroHand' in the Solution Explorer and click 'Properties'
 3. At the top of the 'Property Pages' window, set Configuration to 'All Configurations'
 4. Navigate to Configuration Properties > Debugging and set the Working Directory to 'bin'
 5. Open myAllegroHand.cpp and, near the top, find comment '// USER HAND CONFIGURATION' and the constants below it

 6. const bool RIGHT_HAND: Set to 'true' if using a right AH and false if using left
 7. const int HAND_VERSION: For version 2.x, set to '2', for version 3.x, set to '3', etc.
 8. const int enc_offset[MAX_DOF]: Encoder offsets
 9. const double enc_dir[MAX_DOF]: Encoder Directions (Signs)
 10. const double motor_dir[MAX_DOF]: Motor Directions (Signs)

 * Offsets and directions for your hand can be found at simlab.co.kr/wiki/allegrohand, from the DMLs for your hand, or via email <alexalspach@simlab.com>.
 
You are now ready to compile, plug in and turn on your hand, and test the program.

Keyboard commands can be used to execute grasps and other joint configurations. 
See the instructions printed at the beginning of the application.

=====

Allegro Hand Standalone Visual Studio Project and Source

This file contains a summary of what you will find in each of the files that make up your myAllegroHand application.



**myAllegroHand.vcproj:**

This is the main project file for VC++ projects generated using an Application Wizard. It contains information about the version of Visual C++ that generated the file, and information about the platforms, configurations, and project features selected with the Application Wizard.

	
	
**myAllegroHand.cpp:**

This is the main application source file.

	
	
**Other standard files:**

StdAfx.h, StdAfx.cpp

These files are used to build a precompiled header (PCH) file named myAllegroHand.pch and a precompiled types file named StdAfx.obj.

	
	
**Other notes:**

AppWizard uses "TODO:" comments to indicate parts of the source code you should add to or customize.

For more information regarding setting the project properties, etc., please see the included document Allegro_Hand_MSVS_Project_*.pdf.

**Note:** This document is currently provided only in Korean. All important setup information can be gathered deirectly from the screenshots.



**Questions?**

Please email Alex Alspach
alexalspach@simlab.co.kr