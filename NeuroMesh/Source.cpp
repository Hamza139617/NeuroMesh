#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");

	m.printLayer(2);

	m.deleteNeuron(1);
	m.deleteNeuron(2);
	m.deleteNeuron(3);
	m.deleteNeuron(4);
	m.deleteNeuron(5);

	m.printLayer(1);
	


	cout << endl << endl << endl << endl; // basically for seperating the output of the raylib from the output
	// of our operation.
	return 0;
}