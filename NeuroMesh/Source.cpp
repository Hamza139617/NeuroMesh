#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");

		

	m.findNeuronByPosition(0, 1, 1);

	return 0;
}