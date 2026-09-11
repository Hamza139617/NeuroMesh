#pragma once
#include <iostream>
#include <fstream>
#include <cstdint>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
using namespace std;

struct Layer {
	int N;
	int current_count;

	int layerId;
	Neuron* top_left;
	Layer* next;
	Layer* prev;
};

struct Neuron {
	int id;
	double weight;
	Neuron* up;
	Neuron* down;
	Neuron* left;
	Neuron* right;
	Synapse* head_axon;
};

struct Synapse {
	double weight;
	bool is_active;
	Neuron* target_neuron;
	Synapse* next_synapse;
};



class NeuroMesh {
private:
	Layer* head_layer;
	Layer* tail_layer;
	int N;
	double pthreshold;
	double fthreshold;

public:
	NeuroMesh() {
		head_layer = nullptr;
		tail_layer = nullptr;
		N = 0;
		pthreshold = 0.0;
		fthreshold = 0.0;
	}

	//==============creation================

	void loadFromFile(const std::string& fileName);
	void insertNeuron(int id, double weight) {

	}


	//==============deleting=================

	void deleteNeuron(int id);
	void deleteLayer(int layerId);

	//=============triggering================
	void pruneNeuron(Neuron* n);
	Neuron* mergeNeurons(Synapse* s);
	Neuron* mergeColumns(Neuron* columnA_any, Neuron* columnB_any);
	Layer* mergeLayers(Layer* a, Layer* b);
	void removeEmptyLayer(Layer* l);

	

	//===============Printng/Exportng============
	void printLayer(int layerid);
	void exportMesh(const std::string& fileName);

	void navigateMesh();

	void printNeuron(int id);

	//!!!!!!!!!!!!!!!!!searching!!!!!!!!!!!!!!!!!!!
	Neuron* findNeuronById(int id);
	Neuron* findNeuronByPosition(int layerId, int row, int col);


	//!!!!!!!!!!!!!!!!!prapogation!!!!!!!!!!!!!!!!!
	char forwardPropagate(char inputLetter);

	void backwardPropagate(char targetLetter);

private:
	// private helper functions for accomplshing a specific task
	Layer* createLayer() {

	}
};