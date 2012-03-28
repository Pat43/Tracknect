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

#include "Tracknect.h"
#include <GL/glut.h>
#include <math.h>

extern void DrawDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd, XnUserID player);

xn::Context g_Context;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;

XnSkeletonJointPosition jointHead, jointLeftShoulder, jointRightShoulder, jointHip;

XnUserID g_nPlayer = 0;

XnBool g_bCalibrated = FALSE;
XnBool g_bPause = FALSE;
XnBool g_bQuit = FALSE;
XnBool g_bSkel = TRUE;
XnBool g_bDrawDepthWhileTracking = FALSE;

int g_iMode;
int g_iTrackingState = TN_STATE_NOTHING_TRACKED;
int g_iCalibrationState = TN_NOT_CALIBRATED;
float g_fStrechDistance = 400;
std::string XmlPath = "./Data/User.xml";

int iTn_X;
int iTn_Y;
int iTn_Z;

void TnCleanupExit()
{
	g_Context.Shutdown();
	exit (1);
}


XnBool AssignPlayer(XnUserID user)
{
	if (!g_bSkel) // booleen pour arreter de chercher un squelette
	{
		return FALSE;
	}

	if (g_nPlayer != 0)
	{
		return FALSE;
	}

	XnPoint3D com;
	g_UserGenerator.GetCoM(user, com);
	if (com.Z == 0)
	{
		return FALSE;
	}

	printf("Matching for existing calibration : \n");
	g_UserGenerator.GetSkeletonCap().LoadCalibrationData(user, 0);
	g_UserGenerator.GetSkeletonCap().StartTracking(user);

	g_nPlayer = user;
	return TRUE;

}
void XN_CALLBACK_TYPE NewUser(xn::UserGenerator& generator, XnUserID user, void* pCookie)
{
	if (!g_bCalibrated) // check on player0 is enough
	{
		printf("Look for pose\n");
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection("Psi", user);
		return;
	}

	AssignPlayer(user);

}
void FindPlayer()
{
	if (g_nPlayer != 0)
	{
		return;
	}
	XnUserID aUsers[20];
	XnUInt16 nUsers = 20;
	g_UserGenerator.GetUsers(aUsers, nUsers);

	for (int i = 0; i < nUsers; ++i)
	{
		if (AssignPlayer(aUsers[i]))
			return;
	}
}
void LostPlayer()
{
	g_nPlayer = 0;
	FindPlayer();

}
void XN_CALLBACK_TYPE LostUser(xn::UserGenerator& generator, XnUserID user, void* pCookie)
{
	printf("Lost user %d\n", user);
	if (g_nPlayer == user)
	{
		LostPlayer();
	}
}
void XN_CALLBACK_TYPE PoseDetected(xn::PoseDetectionCapability& pose, const XnChar* strPose, XnUserID user, void* cxt)
{
	printf("Found pose \"%s\" for user %d\n", strPose, user);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(user, TRUE);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(user);
}

void XN_CALLBACK_TYPE CalibrationStarted(xn::SkeletonCapability& skeleton, XnUserID user, void* cxt)
{
	printf("Calibration started\n");
	g_iCalibrationState = TN_CALIBRATION_STARTED;
}

void XN_CALLBACK_TYPE CalibrationEnded(xn::SkeletonCapability& skeleton, XnUserID user, XnBool bSuccess, void* cxt)
{
	printf("Calibration done [%d] %ssuccessfully\n", user, bSuccess?"":"un");
	if (bSuccess)
	{
		if (!g_bCalibrated)
		{
			g_UserGenerator.GetSkeletonCap().SaveCalibrationData(user, 0);
			g_nPlayer = user;
			g_UserGenerator.GetSkeletonCap().StartTracking(user);
			g_bCalibrated = TRUE;
			g_iCalibrationState = TN_CALIBRATED;
		}

		XnUserID aUsers[10];
		XnUInt16 nUsers = 10;
		g_UserGenerator.GetUsers(aUsers, nUsers);
		for (int i = 0; i < nUsers; ++i)
			g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(aUsers[i]);
	}

	else
		if (!g_bCalibrated) // check on player0 is enough
		{
			printf("Look for pose\n");
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection("Psi", user);
			g_iCalibrationState = TN_NOT_CALIBRATED;
			return;
		}
}

#define CHECK_RC(rc, what)											\
	if (rc != XN_STATUS_OK)											\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(rc));		\
		return rc;													\
	}

int TnInitialization(int mode)
{

	g_iMode = mode;

	XnStatus rc = XN_STATUS_OK;

	rc = g_Context.InitFromXmlFile(XmlPath.c_str());
	CHECK_RC(rc, "InitFromXml");

	rc = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(rc, "Find depth generator");
	rc = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	CHECK_RC(rc, "Find user generator");

	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON) ||
		!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
	{
		printf("User generator doesn't support either skeleton or pose detection.\n");
		return XN_STATUS_ERROR;
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	rc = g_Context.StartGeneratingAll();
	CHECK_RC(rc, "StartGenerating");

	XnCallbackHandle hPoseCBs, hUserCBs, hCalibrationCBs;

	g_UserGenerator.RegisterUserCallbacks(NewUser, LostUser, NULL, hUserCBs);
	g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(CalibrationStarted, CalibrationEnded, NULL, hCalibrationCBs);
	g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(PoseDetected, NULL, NULL, hPoseCBs);

	return 0;
}

void TnStopTracking()
{
	g_bSkel = false;
	g_nPlayer = 0;
}

void TnStartTracking()
{
	g_bSkel = true;
	FindPlayer();

}

void TnProcess()
{
	if (!g_bPause)
	{
		// Read next available data
		g_Context.WaitAndUpdateAll();
	}


	if (g_bCalibrated && g_nPlayer == 0)
	{

		LostPlayer();
	}

	TnUpdateTrackingState();
	XnPoint3D point = TnGetTrackedObjectCoordonates();

	iTn_X = point.X;
	iTn_Y = point.Y;
	iTn_Z = point.Z;
}

void TnDisplay()
{
	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	g_DepthGenerator.GetMetaData(depthMD);




	glOrtho(0, depthMD.XRes(), depthMD.YRes(), 0, -1.0, 1.0);

	//DRAW
	g_DepthGenerator.GetMetaData(depthMD);
	g_UserGenerator.GetUserPixels(0, sceneMD);
	DrawDepthMap(depthMD, sceneMD, g_nPlayer);

	unsigned int sd = (unsigned int)depthMD.DataSize();
	unsigned int ss = (unsigned int)sceneMD.DataSize();
	int i=0;
}

void TnPause()
{
	g_bPause = !g_bPause;
}

XnPoint3D TnGetTrackedObjectCoordonates()
{
	XnPoint3D point;

	XnSkeletonJointPosition joint1, joint2;

	if (g_iMode == TN_MODE_HEAD_TRACKING)
	{
		XnSkeletonJointPosition joint1;
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_HEAD, joint1);

		point = joint1.position;
	}

	if (g_iMode == TN_MODE_HAND_TRACKING)
	{
		if (g_iTrackingState == TN_STATE_RIGHT_HAND_TRACKED) // Main droite uniquement
		{
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_SHOULDER, joint1);
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_HAND, joint2);

			if (joint1.fConfidence > 0.5) // MAJ du joint shoulder si celui ci est sûr, sinon il ne sera pas mis à jour,
				jointRightShoulder = joint1;  // ce qui entrainera une simulation de sa position avec sa position précédente.

			joint1 = jointRightShoulder;

		}
		else if (g_iTrackingState == TN_STATE_LEFT_HAND_TRACKED) // Main gauche
		{
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_SHOULDER, joint1);
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_HAND, joint2);

			if (joint1.fConfidence > 0.5) // MAJ du joint shoulder si celui ci est sûr, sinon il ne sera pas mis à jour,
				jointLeftShoulder = joint1;  // ce qui entrainera une simulation de sa position avec sa position précédente.

			joint1 = jointLeftShoulder;
		}
		else if (g_iTrackingState == TN_STATE_BOTH_HANDS_TRACKED) // Droite && Gauche
		{
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_HAND, joint1);
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_HAND, joint2);
		}
		else
		{
			point.X = 0;
			point.Y = 0;
			point.Z = 0;
			return point;
		}

		XnPoint3D pt[2];
		pt[0] = joint1.position;
		pt[1] = joint2.position;

		point.X = pt[1].X - pt[0].X;
		point.Y = pt[1].Y - pt[0].Y;
		point.Z = pt[1].Z - pt[0].Z;



		if (joint2.fConfidence <0.5) // Si la confidence est trop basse, ne pas tracker le point.
		{
			point.X = 0;
			point.Y = 0;
			point.Z = 0;
		}

	}

	if (g_iMode == TN_MODE_RUNNING_GAME)
	{
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_FOOT, joint1);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_FOOT, joint2);


		XnPoint3D pt[2];
		pt[0] = joint1.position;
		pt[1] = joint2.position;

		point.Y = pt[1].Y - pt[0].Y;

		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_TORSO, joint1);
		pt[0] = joint1.position;
		point.X = pt[0].X;


		point.Z=0;

	}
	return point;
}

void TnUpdateTrackingState()
{

	g_iTrackingState = TN_STATE_NOTHING_TRACKED;

	if (g_nPlayer == 0)
	{
		g_iTrackingState = TN_STATE_NOTHING_TRACKED;
		return;
	}


	if (g_iMode == TN_MODE_HEAD_TRACKING)
		g_iTrackingState = TN_STATE_HEAD_TRACKED; // Tracking de la tête

	else if (g_iMode == TN_MODE_HAND_TRACKING)
	{
		XnPoint3D vector;
		XnPoint3D pt[2];
		XnSkeletonJointPosition joint1,joint2;

		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_HAND, joint1);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_HAND, joint2);

		if (joint1.fConfidence<0.5 && joint2.fConfidence<0.5)
		{
			g_iTrackingState = TN_STATE_HANDS_LOST;
			return;
		}


		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_HIP, joint1);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_HIP, joint2);

		if (joint1.fConfidence>0.5)
			jointHip = joint1;
		else if (joint2.fConfidence>0.5)
			jointHip = joint2;


		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_SHOULDER, joint1);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_HAND, joint2);

		if (joint1.fConfidence > 0.5) // MAJ du joint soulder si celui ci est sûr, sinon il ne sera pas mis à jour,
			jointRightShoulder = joint1;  // ce qui entrainera une simulation de sa position avec sa position précédente.

		joint1 = jointRightShoulder;

		pt[0] = joint1.position;
		pt[1] = joint2.position;

		vector.X = pt[1].X - pt[0].X;
		vector.Y = pt[1].Y - pt[0].Y;
		vector.Z = pt[1].Z - pt[0].Z;

		float normeVector = sqrt(vector.X*vector.X + vector.Y*vector.Y + vector.Z*vector.Z);

		if (normeVector > g_fStrechDistance && vector.Y > jointHip.position.Y) // Test main droite
			g_iTrackingState = TN_STATE_RIGHT_HAND_TRACKED; // La main droite est en avant

		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_SHOULDER, joint1);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_HAND, joint2);

		if (joint1.fConfidence > 0.5) // MAJ du joint soulder si celui ci est sûr, sinon il ne sera pas mis à jour,
			jointLeftShoulder = joint1;  // ce qui entrainera une simulation de sa position avec sa position précédente.

		joint1 = jointLeftShoulder;

		pt[0] = joint1.position;
		pt[1] = joint2.position;

		vector.X = pt[1].X - pt[0].X;
		vector.Y = pt[1].Y - pt[0].Y;
		vector.Z = pt[1].Z - pt[0].Z;

		normeVector = sqrt(vector.X*vector.X + vector.Y*vector.Y + vector.Z*vector.Z);

		if (normeVector > g_fStrechDistance && vector.Y > jointHip.position.Y) // Test main gauche
			if	(g_iTrackingState == TN_STATE_RIGHT_HAND_TRACKED) // Test main droite précédent
				g_iTrackingState = TN_STATE_BOTH_HANDS_TRACKED; // Les deux mains sont en avant
			else
				g_iTrackingState = TN_STATE_LEFT_HAND_TRACKED; // Seulement la main gauche est en avant
		else
			if	(g_iTrackingState == TN_STATE_RIGHT_HAND_TRACKED) // Test main droite précédent
				g_iTrackingState = TN_STATE_RIGHT_HAND_TRACKED; // Seulement la main droite est en avant
			else
				g_iTrackingState = TN_STATE_NO_HAND_TRACKED; // Hand Tracking mais pas de main en avant

	}

	else if (g_iMode == TN_MODE_RUNNING_GAME)
	{
		XnSkeletonJointPosition joint1, joint2;

		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_FOOT, joint1);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_FOOT, joint2);

		if (joint1.position.Y > -800 && joint2.position.Y > -800)
			g_iTrackingState = TN_STATE_JUMPING;
	}
}

bool TnIsProtecting()
{
	if (g_iMode != TN_MODE_RUNNING_GAME)
		return false;

	XnSkeletonJointPosition jointLeft, jointRight, jointN;

	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_ELBOW, jointLeft);
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_ELBOW, jointRight);
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_NECK, jointN);

	// Returns true only if the user is doing a X sign with his arms above his shoulders

	// if the elbows are above the neck (the neck joint is aligned with the shoulders)
	if (jointLeft.position.Y > jointN.position.Y && jointRight.position.Y > jointN.position.Y)
	{
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_LEFT_HAND, jointLeft);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(g_nPlayer, XN_SKEL_RIGHT_HAND, jointRight);

		// if the left hand is at the right of the right hand
		if (jointLeft.position.X>jointRight.position.X)
			return true;
	}

	// otherwise, return false
	return false;

}


// Getters :

int Tn_X(){return iTn_X;}
int Tn_Y(){return iTn_Y;}
int Tn_Z(){return iTn_Z;}

xn::Context TnGetContext(){return g_Context;}
xn::DepthGenerator TnGetDepthGenerator(){return g_DepthGenerator;}
xn::UserGenerator TnGetUserGenerator(){return g_UserGenerator;}
XnBool TnGetPauseState(){return g_bPause;}
int TnGetTrackingState(){return g_iTrackingState;}
XnBool TnIsTrackingEnable(){return g_bSkel;}
int TnGetTrackingMode(){return g_iMode;}
XnUserID TnGetTrackedPlayerID(){return g_nPlayer;}
int TnGetCalibrationState(){return g_iCalibrationState;}
std::string TnGetDataFilesLocation(){return XmlPath;}
float TnGetStrechDistance(){return g_fStrechDistance;}
XnBool TnGetDrawDepthWhileTrackingSkeleton(){return g_bDrawDepthWhileTracking;}
int TnGetNbUsers(){return g_UserGenerator.GetNumberOfUsers();}

// Setters :

void TnSetDataFilesLocation(char* location){XmlPath = location;}
void TnSetStrechDistance(float distance){g_fStrechDistance = distance;}
void TnSetDrawDepthWhileTrackingSkeleton(XnBool draw){g_bDrawDepthWhileTracking = draw;};
