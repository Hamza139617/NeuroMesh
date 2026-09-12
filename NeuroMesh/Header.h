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

struct Neuron { // basic building block
	int id;
	double weight;
	Neuron* up;
	Neuron* down;
	Neuron* left;
	Neuron* right;
	Synapse* head_axon;
};

struct Synapse { // the connection for the conecting neurons
	double weight;
	bool is_active;
	char direction;// U up D down L left R right
	Neuron* target_neuron;
	Synapse* next_synapse;
};


struct Layer { // collec tion of neurons
	int N;
	int current_count;

	int layerId;
	Neuron* top_left;
	Layer* next;
	Layer* prev;
};





class NeuroMesh { // the neural substrate
private:
	Layer* head_layer;
	Layer* tail_layer;
	int N;
	double pthreshold;
	double fthreshold;

	struct PlacementSpot {
		Layer* layer;
		int row;
		int col;
	};

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
		// for inserting the neuron according to the structuring constraints and ordering constraints

		if (silentNeuronReturnById(id) != nullptr) {
			cout << endl << "Insert failed: id" << id << " already exists." << endl;
			// if there is already a neuron with the same id then return because to neuron can't have same id
			return;
		}

		PlacementSpot  spot = findPlacementSpot();
		Neuron* n = placeNeuronAt(spot.layer, spot.row, spot.col, id, weight);
		wireNeuronConnections(spot.layer, n, spot.row, spot.col);

		cout << "Inserted neuron id " << id << " Layer : " << spot.layer->layerId << endl;
		cout << " row " << spot.row << " col " << spot.col << endl;

	}


	void loadFromFile(const std::string& fileName) {
		// loading the data from the file for initializing the mesh
		ifstream fin(fileName);

		if (!fin) {
			cout << "Could not open the file: " << fileName << endl;
			return;
		}

		fin >> N >> pthreshold >> fthreshold;

		int id;
		double weight;
		while (fin >> id >> weight) {
			PlacementSpot spot = findPlacementSpot();
			placeNeuronAt(spot.layer, spot.row, spot.col, id, weight);
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

			col = 0;
			for (Neuron* colt = l->top_left; colt; colt = colt->right) {
				col++;
				for (Neuron* n = colt; n; n = n->down) {
					if (n->id == id) {
						cout << "Layer Id : " << l->layerId << endl << "Row: " << row << " Col: " << col;
						return n;
					}

					row++;

				}
				row = 0;
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

		int row1 = 0;
		int col1 = 0;

		Neuron* n = nullptr; // for traversing

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			if (l->layerId == layerId) {
				// cout << "h1";

				Neuron* colt = l->top_left;
				for (int c = 0; c < col && colt; c++)
					colt = colt->right;
			
				if (!colt) return nullptr;

				Neuron* n = colt;
				for (int r = 0; r < row && n; r++)
					n = n->down;

				if (!n) 
				return nullptr;

				cout << "Neuron id : " << n->id << " Weight: " << n->weight;
				return n;

			}

		}

		return nullptr;


	}

	Neuron* silentNeuronReturnByPosition(int layerId, int row, int col) {
		// findng the neuron by layerid row and col then returning it

		int row1 = 0;
		int col1 = 0;

		Neuron* n = nullptr; // for traversing

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			if (l->layerId == layerId) {
				// cout << "h1";

				Neuron* colt = l->top_left;
				for (int c = 0; c < col && colt; c++)
					colt = colt->right;

				if (!colt) return nullptr;

				Neuron* n = colt;
				for (int r = 0; r < row && n; r++)
					n = n->down;

				if (!n)
					return nullptr;


				return n;

			}

		}

		return nullptr;


	}

	Neuron* silentNeuronReturnById(int id) {
		// for finding the neuron by id and rturning

		int row = 0;
		int col = 0;

		Neuron* n = nullptr; // for traversing
		Neuron* st = nullptr; // for storing the current ptr

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			col = 0;
			for (Neuron* colt = l->top_left; colt; colt = colt->right) {
				col++;
				for (Neuron* n = colt; n; n = n->down) {
					if (n->id == id) {
						
						return n;
					}

					row++;

				}
				row = 0;
			}

		}

		return nullptr;
	}


	//!!!!!!!!!!!!!!!!!prapogation!!!!!!!!!!!!!!!!!
	char forwardPropagate(char inputLetter);

	void backwardPropagate(char targetLetter);

private:

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!! Helper functions sections !!!!!!!!!!!!!!!!

	Layer* ownerLayer(Neuron* target) {
		for (Layer* l = head_layer; l != nullptr; l = l->next) {
			for (Neuron* colt = l->top_left; colt != nullptr; colt = colt->right) {
				for (Neuron* n = colt; n != nullptr; n = n->down) {
					if (n == target) return l;
				}
			}
		}
	}

	
	PlacementSpot findPlacementSpot() {
		Layer* target = nullptr;

		for (Layer* l = head_layer; l != nullptr; l = l->next) {
			if (l->current_count < l->N * l->N) {
				target = l;
				break;
			}
		}

		if (target == nullptr) {
			int newId = tail_layer ? tail_layer->layerId + 1 : 0;
			target = createLayer(N, newId, nullptr, tail_layer);
			if (tail_layer)
				tail_layer->next = target;
			tail_layer = target;

			if (head_layer == nullptr) head_layer = target;
		}

		int bestCol = -1, bestRow = -1;
		double bestWeight = -1.0;

		for (int c = 0; c < target->N; c++) {
			int h = columnHeight(target, c);
			if (h >= target->N) continue;
			double w = columnWeight(target, c);
			if (bestCol == -1 || w < bestWeight) {
				bestCol = c;
				bestRow = h;
				bestWeight = w;
			}
		}

		return { target, bestRow, bestCol };
	}


	Neuron* placeNeuronAt(Layer* l, int row, int col, int id, double weight) {
		Neuron* n = new Neuron();
		n->id = id;
		n->weight = weight;
		n->up = n->down = n->left = n->right = nullptr;
		n->head_axon = nullptr;

		if (row == 0) {
			if (col == 0) {
				l->top_left = n;
			}
			else {
				Neuron* leftCol = columnTop(l, col - 1);
				n->left = leftCol;
				if (leftCol) leftCol->right = n;
			}
		}
		else {
			Neuron* bottom = columnTop(l, col);
			while (bottom->down != nullptr) bottom = bottom->down;
			bottom->down = n;
			n->up = bottom;

			Neuron* leftNeighbor = silentNeuronReturnByPosition(l->layerId, row, col - 1);
			if (leftNeighbor) {
				n->left = leftNeighbor;
				leftNeighbor->right = n;
			}
			Neuron* rightNeighbor = silentNeuronReturnByPosition(l->layerId, row, col + 1);
			if (rightNeighbor) {
				n->right = rightNeighbor; 
				rightNeighbor->left = n;
			}

			
		} 

		l->current_count++;
		return n;
	}


	void wireNeuronConnections(Layer* l, Neuron* n, int row, int col) {
		if (l->next != nullptr) {
			Neuron* aligned = silentNeuronReturnByPosition(l->next->layerId, row, col);
			if (aligned != nullptr) {
				addSynapse(n, aligned->up, 'U');
				addSynapse(n, aligned->down, 'D');
				addSynapse(n, aligned->left, 'L');
				addSynapse(n, aligned->right, 'R');
			}
		}

		if (l->prev != nullptr) {
			struct Mirror { int dr, dc; char dir; };

			Mirror mirrors[4] = {
				{1, 0, 'U'},
				{-1, 0, 'D'},
				{0, 1, 'L'},
				{0, -1, 'R'},
			};

			for (int i = 0; i < 4; i++) {
				int pr = row + mirrors[i].dr;
				int pc = col + mirrors[i].dc;
				if (pr < 0 || pc < 0) continue;
				Neuron* p = silentNeuronReturnByPosition(l->prev->layerId, pr, pc);
				if (p != nullptr) addSynapse(p, n, mirrors[i].dir);
			}

		}


	}



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

	void addSynapse(Neuron* from, Neuron* to, char dir) {
		// for adding the synapse between different neurons alongside the direction represented dir

		if (from == nullptr || to == nullptr) return;
		Synapse* s = new Synapse();

		s->weight = (from->weight + to->weight) / 4.0;
		s->weight = (from->weight + to->weight) / 4.0;
		s->is_active = false;
		s->direction = dir;
		s->target_neuron = to;
		s->next_synapse = from->head_axon;
		from->head_axon = s;

	}

	void setSynapseWeight(int id, char dir, double newWeight) {
		Neuron* source = silentNeuronReturnById(id);

		if (source == nullptr) {
			cout << "No neuron with id " << id << endl;
			return;
		}

		for (Synapse* s = source->head_axon; s != nullptr; s = s->next_synapse) {

			if (s->direction == dir) {
				s->weight = newWeight;
				return;
			}
		}

		cout << "Neuron " << id << " has no synapse in direction " << dir << endl;
	}

	void buildAllSynapses() {
		for (Layer* l = head_layer; l != nullptr && l->next != nullptr; l = l->next) {
			Layer* lnext = l->next;

			int col = 0;

			for (Neuron* colt = l->top_left; colt; colt = colt->right, col++) {
				int row = 0;
				for (Neuron* src = colt; src; src = src->down, row++) {
					Neuron* aligned = silentNeuronReturnByPosition(lnext->layerId, row, col);
					if (aligned) {
						addSynapse(src, aligned->up, 'U');
						addSynapse(src, aligned->down, 'D');
						addSynapse(src, aligned->left, 'L');
						addSynapse(src, aligned->right, 'R');
					}
				}
			}
		}
	}

	//! column ordering and management functions 

	int columnHeight(Layer* l, int col) {
		int h = 0;
		for (Neuron* n = columnTop(l, col); n != nullptr; n = n->down) h++;
		
		return h;
	}

	Neuron* columnTop(Layer* l, int col) {
		Neuron* c = l->top_left;
		for (int i = 0; i < col && c != nullptr; i++)
			c = c->right;
		return c;
	}


	void clearAxons(Neuron* n) {
		if (!n) return;
		Synapse* s = n->head_axon;
		Synapse* next;

		while (s != nullptr) {
			next = s->next_synapse;
			delete s;
			s = next;
		}
		n->head_axon = nullptr;
	}

	
	void orderColumns() {
		// basically for ordering the columns

		double weight1;
		double weight2;

		int col = 0;
		int row = 0;

		Neuron* secondN = nullptr;
		Neuron* temp = nullptr;

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			col = 0;

			for (Neuron* n = l->top_left; n->right != nullptr; n = n->right, col++) {

				weight1 = columnWeight(l, col);
				weight2 = columnWeight(l, col + 1);

				if (weight1 > weight2) {
					// checking if the total weight of one column is more then the total weight of the other column
					// in this case swap

					for (Neuron* firstN = n; firstN; firstN = firstN->down, row++) {
						secondN = firstN->right;

						firstN->right = secondN->right;
						secondN->right = firstN;
						
						if (firstN->right) {
							firstN->right->left = firstN;
						}

						secondN->left = firstN->left;
						firstN->left = secondN;




					}
					row = 0;
				}

			}

			

		}

	}


	public:


		
		double columnWeight(Layer* l, int col) {
			double sum = 0.0;
			for (Neuron* n = columnTop(l, col); n != nullptr; n = n->down) {
				sum += n->weight;
			}

			return sum;
		}


};