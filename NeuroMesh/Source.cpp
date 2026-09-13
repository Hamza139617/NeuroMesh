#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");

	m.printLayer(2);




	
	Neuron* n = m.findNeuronById(7);
	cout << endl;
	m.pruneNeuron(n);
	m.printLayer(2);

	

	cout << endl << endl << endl << endl; // basically for seperating the output of the raylib from the output
	// of our operation.
	return 0;
}