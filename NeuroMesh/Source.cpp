#include "Header.h"


int main() {

	NeuroMesh m;

	m.loadFromFile("mesh.txt");
	m.printLayer(0);



	return 0;
}