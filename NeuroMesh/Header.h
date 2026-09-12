#pragma once
#include <iostream>
#include <fstream>
#include <cstdint>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
using namespace std;

struct Synapse; // forwarding declare

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


struct Layer {
	int N;
	int current_count;

	int layerId;
	Neuron* top_left;
	Layer* next;
	Layer* prev;
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

	void insertNeuron(int id, double weight) {



	}


	void loadFromFile(const std::string& fileName) {
		ifstream fin(fileName);

		if (!fin) {
			cout << "Could not open the file: " << fileName << endl;
			return;
		}

		fin >> N >> pthreshold >> fthreshold;

		head_layer = tail_layer = createLayer(N, 0, nullptr, nullptr);

		int row = 0, col = 0;
		Neuron* leftNeighbor = nullptr;
		Neuron* rowStart = nullptr;
		Neuron* prevRowStart = nullptr;
		Neuron* aboveWalker = nullptr;

		int id;
		double weight;

		while (fin >> id >> weight) {

			if (tail_layer->current_count == N * N) {
				Layer* newLayer = createLayer(N, tail_layer->layerId + 1, nullptr, tail_layer);
				tail_layer->next = newLayer;
				tail_layer = newLayer;

				row = 0;
				col = 0;

				leftNeighbor = nullptr;
				rowStart = nullptr;
				prevRowStart = nullptr;
				aboveWalker = nullptr;

			}

			Neuron* aboveNeighbor = (row == 0) ? nullptr : aboveWalker;

			Neuron* n = new Neuron();
			n->id = id;
			n->weight = weight;
			n->up = aboveNeighbor;
			n->down = nullptr;
			n->left = leftNeighbor;
			n->right = nullptr;
			n->head_axon = nullptr;

			if (leftNeighbor) leftNeighbor->right = n;
			if (aboveNeighbor) aboveNeighbor->down = n;
			if (row == 0 && col == 0) tail_layer->top_left = n;

			tail_layer->current_count++;

			if (col == 0) rowStart = n;
			leftNeighbor = n;
			if (row > 0) aboveWalker = aboveWalker->right;

			col++;
			if (col == N) {
				col = 0;
				row++;
				prevRowStart = rowStart;
				aboveWalker = prevRowStart;
				leftNeighbor = nullptr;
			}
		}

		fin.close();

		buildAllSynapses();
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
	void printLayer(int layerid) {
		// for printing the layer 
		// this is als going to be helping us in the debugging part as well

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			Neuron* n;

			n = l->top_left;

			cout << "Layer : " << l->layerId << endl;

			while (n != nullptr) {

				Neuron* st = n;

				for (; n->right != nullptr; n = n->right) {
					cout << n->weight << " ";
				}

				cout << n->weight << endl;

				n = st->down;
			}
			

		}

	}
	void exportMesh(const std::string& fileName);

	void navigateMesh();

	Neuron* findNeuronById(int id) {
		// for finding the neuron by id and rturning

		int row = 0;
		int col = 0;

		Neuron* n = nullptr; // for traversing
		Neuron* st = nullptr; // for storing the current ptr

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			n = l->top_left;

			row = 0;
			col = 0;

			while (n != nullptr) {

				st = n;


				for (; n->right != nullptr; n = n->right) {

					if (n->id == id) {
						cout << "Layer Id : " << l->layerId << endl << "Row: " << row << " Col: " << col;
						return n;
					}

					col++;
				}

				if (n->id == id) {
					cout << "Layer Id : " << l->layerId << endl << "Row: " << row << " Col: " << col;
					return n;
				}

				row++;
				col = 0;

				n = st->down;

			}

		}

		return nullptr;
	}

	void printNeuron(int id) {
		// sharing the neuron information 
		Neuron* n = findNeuronById(id);

		if (n) {
			cout << " Neuron id : " << n->id << endl << "Weight : " << n->weight;
		}

		return;
	}

	//!!!!!!!!!!!!!!!!!searching!!!!!!!!!!!!!!!!!!!

	Neuron* findNeuronByPosition(int layerId, int row, int col) {
		// findng the neuron by layerid row and col then returning it

		int row = 0;
		int col = 0;

		Neuron* n = nullptr; // for traversing
		Neuron* st = nullptr; // for storing the current ptr

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			n = l->top_left;

			row = 0;
			col = 0;

			while (n != nullptr) {

				st = n;


				for (; n->right != nullptr; n = n->right) {

					if (n->id == id) {
						cout << "Layer Id : " << l->layerId << endl << "Row: " << row << " Col: " << col;
						return n;
					}

					col++;
				}

				if (n->id == id) {
					cout << "Layer Id : " << l->layerId << endl << "Row: " << row << " Col: " << col;
					return n;
				}

				row++;
				col = 0;

				n = st->down;

			}

		}

		return nullptr;


	}


	//!!!!!!!!!!!!!!!!!prapogation!!!!!!!!!!!!!!!!!
	char forwardPropagate(char inputLetter);

	void backwardPropagate(char targetLetter);

private:
	// private helper functions for accomplshing a specific task
	Layer* createLayer(int N, int layerId, Layer* next, Layer* prev) {
		Layer* newLayer = new Layer();
		newLayer->N = N;
		newLayer->current_count = 0;
		newLayer->layerId = layerId;
		newLayer->next = next;
		newLayer->prev = prev;
		newLayer->top_left = nullptr;

		return newLayer;
	}

	void addSynapse(Neuron* from, Neuron* to) {
		if (from == nullptr || to == nullptr) return;
		Synapse* s = new Synapse();
		
		s->weight = 0.0;
		s->is_active = false;
		s->target_neuron = to;
		s->next_synapse = from->head_axon;
		from->head_axon = s;

	}

	void buildAllSynapses() {
		for (Layer* l = head_layer; l != nullptr && l->next != nullptr; l = l->next) {
			Layer* lnext = l->next;

			Neuron* srcRow = l->top_left;
			Neuron* tgtRow = lnext->top_left;

			while (srcRow != nullptr) {
				Neuron* src = srcRow;
				Neuron* aligned = tgtRow;

				while (src != nullptr) {
					if (aligned != nullptr) {
						addSynapse(src, aligned->up);
						addSynapse(src, aligned->down);
						addSynapse(src, aligned->left);
						addSynapse(src, aligned->right);
					}

					src = src->right;
					aligned = (aligned != nullptr) ? aligned->right : nullptr;
				}

				srcRow = srcRow->down;
				tgtRow = (tgtRow != nullptr) ? tgtRow->down : nullptr;
			}
		}
	}
};