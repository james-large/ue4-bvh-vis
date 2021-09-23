# ue4-bvh-vis
Tools for visualising motion capture data (BvH) in UE4.

As of initialisation of this project, this repo contains mainly the BvHAnimator blueprint class under Content/, and the C++ files for the PoseUpdate compenent of the BvHAnimator under Source/

This repo does NOT (yet) contain an entire project to be opened and used, however following investigations on how best to distribute the tools this may be the way forward. 

As a final end goal to this project, we aim to reach this functionality:  
  * Take BvH data as input (either streamed or in a batch file), retarget the skeleteon described in the BvH data to the UE4 default mannequin (automatically or as a stage to be done manually by the user), and play the animation data in a simple UE4 scene. 
  * Export the scene and the played-out animation to video for sharing, with multiple views
  * Read in audio data also (typically recorded speech that the animations are generated from, for our purposes), and play alongside the animation
  * Neatly wrap the functionality into an easy migratable blueprint or installable plugin for use in different projects

Current functionality: 
  * Parsing of a static BvH file and generation of the skeleton representation
  * A rough mapping/retargeting of the BvH skeleton to the UE4 mannequin skeleton
  * Playing of the animation data, subject to the constraints of the mapping/retargeting
