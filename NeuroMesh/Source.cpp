#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");

	RaylibGUI gui(m);
	gui.run();


	

	cout << endl << endl << endl << endl; // basically for seperating the output of the raylib from the output
	// of our operation.
	return 0;
}