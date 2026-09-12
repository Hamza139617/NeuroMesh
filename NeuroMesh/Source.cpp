#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");

		

	m.findNeuronByPosition(0, 1, 1);


	cout << endl << endl << endl << endl; // basically for seperating the output of the raylib from the output
	// of our operation.
	return 0;
}