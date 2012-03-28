#include <iostream>
using namespace std;

#include "Tracknect.h"
#include <GL/glut.h>

#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480

void glutDisplay (void)
{

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_TEXTURE_2D);



	TnDisplay();

	glutSwapBuffers();
}


void glutIdle (void)
{
	// Display the frame
	TnProcess();
	glutPostRedisplay();
}

void glutKeyboard (unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		TnCleanupExit();
	case'p':
		TnPause();
		break;

	case's':
		TnStopTracking();
		break;
	case'd':
		TnStartTracking();
		break;
	case'x':
		TnSetDrawDepthWhileTrackingSkeleton(!TnGetDrawDepthWhileTrackingSkeleton());
		break;
	}
}

void glInit (int * pargc, char ** argv)
{
	glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow ("Loic Teyssier's Tracknect Library");
	//glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

int main(int argc, char **argv)
{
	int err = TnInitialization(TN_MODE_RUNNING_GAME);

	glInit(&argc, argv);
	glutMainLoop();

	return 0;
}
