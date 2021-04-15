# STARTcraft

Get started with Starcraft AI Development as fast as possible. 

New to Starfcraft or AI Programming? Watch the tutorial video(s): 
* Starcraft AI Intro: https://www.youtube.com/watch?v=czhNqUxmLks
* STARTcraft Programming: https://www.youtube.com/watch?v=FEEkO6__GKw

Currently Supported (more coming soon):
* Windows / C++ Development using BWAPI

# Setup Instructions:

## Windows / C++

StartCraft comes with a StarterBot written in C++ using BWAPI 4.4.0. This repo comes with BWAPI, and uses Injectory to launch Starcraft with BWAPI, so Chaoslauncher is not required.

1. Download / Clone this repo to your computer
2. Download and unzip [Starcraft Broodwar 1.16.1](http://www.cs.mun.ca/~dchurchill/startcraft/scbw_bwapi440.zip) to the included `windows/starcraft` folder
3. Run `windows/RunC++BotAndStarcraft.bat` which will launch the bot executable and Starcraft / BWAPI
4. Open `windows/c++/visualstudio/StartCraft.sln` in Visual Studio 2019 to modify / recompile the code

Note: Visual Studio 2019 MUST be updated to the most recent version to be able to link against the included BWAPI libraries
