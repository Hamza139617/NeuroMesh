#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");


	m.printNeuron(1);

	return 0;
}