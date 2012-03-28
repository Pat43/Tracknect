/***************************************************************************
 *   Copyright (C) 2011 by Teyssier Loïc                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _TRACKNECT_H
#define _TRACKNECT_H

#include <XnCppWrapper.h>
#include <XnTypes.h>
#include <iostream>

/*----- Definitions -----*/

//Modes :
// Track the head :
#define TN_MODE_HEAD_TRACKING 				1
// Track the vector head/right hand :
#define TN_MODE_HAND_TRACKING 				2
// Track the feet & torse (game) :
#define TN_MODE_RUNNING_GAME				3

// Calibration States :
#define TN_NOT_CALIBRATED 					-1
#define TN_CALIBRATION_STARTED				0
#define TN_CALIBRATED						1

// Tracking States :
#define TN_STATE_NOTHING_TRACKED 			0
#define TN_STATE_HEAD_TRACKED 				10
#define TN_STATE_NO_HAND_TRACKED 			20
#define TN_STATE_LEFT_HAND_TRACKED 			21
#define TN_STATE_RIGHT_HAND_TRACKED 		22
#define TN_STATE_BOTH_HANDS_TRACKED 		23
#define TN_STATE_HANDS_LOST					24
#define TN_STATE_JUMPING					30


/*----- Functions -----*/

// Initializes TN & the Kinect : MUST BE CALLED BEFORE USING ANY OTHER FUNCTION
int TnInitialization(int mode);

// Starts the skeleton tracking :
void TnStartTracking();
// Stops the skeleton tracking :
void TnStopTracking();
// Updates the tracking state which arm is stretched or puts it to TN_STATE_NOTHING_TRACKED if no player is tracked :
void TnUpdateTrackingState();
// Process the skeleton detection:
void TnProcess();
// Displays the images :
void TnDisplay();
// Pauses the program :
void TnPause();
// Quits properly the program :
void TnCleanupExit();


/*----- Getters -----*/

// Returns the X coordinates of the tracked object :
int Tn_X();
// Returns the Y coordinates of the tracked object :
int Tn_Y();
// Returns the Z coordinates of the tracked object :
int Tn_Z();

// is suposed to be used only in the game mode, returns true if the user is doing a X sign with his arms above his shoulders
bool TnIsProtecting();

// Returns the context :
xn::Context TnGetContext();
// Returns the Depth Generator :
xn::DepthGenerator TnGetDepthGenerator();
// Returns the User Generator :
xn::UserGenerator TnGetUserGenerator();
// Returns the Pause state. true if the program is paused, false if not :
XnBool TnGetPauseState();

// Returns the tracking mode, 0 for TN_HAND_TRACKING, 1 for TN_HEAD_TRACKING :
int TnGetTrackingMode();
// Returns true if the tracking is enabled, false if not :
XnBool TnIsTrackingEnable();
// Returns an integer representing what the kinect is tracking at the moment. (see above : Definitions/Tracking States)
int TnGetTrackingState();
// Returns true if the depth is drawn when the skeleton is tracked (Default : false) :
XnBool TnGetDrawDepthWhileTrackingSkeleton();



// Returns the ID of the tracked user. Returns 0 if no user is tracked :
XnUserID TnGetTrackedPlayerID();
// Returns the calibration state. true if the Kinect is calibrated, false if not :
int TnGetCalibrationState();
// Returns the coordinates of the tracked objects (head or vector head/hand) :
XnPoint3D TnGetTrackedObjectCoordonates();
// Returns the current location of the Data files (Default : ./Data/User.xml) :
std::string TnGetDataFilesLocation();
// Returns the distance to consider that the arm is stretched (Default : 400) :
float TnGetStrechDistance();

// Returns the number of users detected
int TnGetNbUsers();


/*----- Setters -----*/

// Set the location of the Data files (Default : ./Data/User.xml) :
void TnSetDataFilesLocation(char* location);
// Set the distance to consider that the arm is stretched (Default : 400) :
void TnSetStrechDistance(float distance);
// Set if the depth is drawn while tracking a skeleton (Default : false) :
void TnSetDrawDepthWhileTrackingSkeleton(XnBool draw);


#endif
