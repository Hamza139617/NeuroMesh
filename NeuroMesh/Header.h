#pragma once
#include <iostream>
#include <fstream>
#include <cstdint>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "raylib.h"
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

		orderColumns(head_layer);

		checkStatus();
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

	void deleteNeuron(int id) {
		Neuron* ned = silentNeuronReturnById(id);
		if (!ned) return;

		Layer* owner = ownerLayer(ned);
		if (!owner) return;

		// Find the position before changing any of the grid links.
		int row = 0;
		int col = 0;
		findNeuronPos(owner, row, col, ned);


		clearAxons(ned); // clearing the axons of the neuron


		Neuron* current = ned;
		Neuron* below = current->down;

		while (below != nullptr) { // basically for closing the gap 
			current->id = below->id;
			current->weight = below->weight;
			current = below;
			below = below->down;
			// cout << "hi";
		}

		Neuron* last = current;

		// neuron conection rewirrring


		if (last->left) last->left->right = last->right;
		if (last->right) last->right->left = last->left;
		if (last->up) last->up->down = last->down;
		if (last->down) last->down->up = last->up;

		if (last == owner->top_left) {
			if (last->right) {
				owner->top_left = last->right;
			}
			else if (last->down) {
				Neuron* newTop = last->down;
				while (newTop->left) newTop = newTop->left;
				owner->top_left = newTop;
			}
			else {
				owner->top_left = nullptr;
			}
		}

		delete last;
		owner->current_count--;


		rebuildLayerForwardSynapses(owner);
		if (owner->prev) rebuildLayerForwardSynapses(owner->prev);


		orderColumns(head_layer);
		checkStatus();

	}

	void deleteLayer(int layerId) {
		for (Layer* l = head_layer; l; l = l->next) {
			if (l->layerId == layerId) {
				if (l->current_count > 0) return;
				delete l;
				l = nullptr;
				return;
			}
		}
		checkStatus();
		return;
	}

	void pruneNeuron(Neuron* n) {
		// if no n then return
		if (!n) return;

		// if condition not satisfied return
		if (n->weight <= pthreshold) return;


		int row = 0;
		int col = 0;

		Layer* owner = ownerLayer(n);
		if (!owner) return;// catching owner

		Neuron* left = n->left;
		Neuron* right = n->right;
		Neuron* up = n->up;
		Neuron* down = n->down;

		if (left) clearAxons(left);
		if (right) clearAxons(right);
		if (up) clearAxons(up);
		if (down) clearAxons(down);


		if (left) {
			Neuron* current = left;
			Neuron* inward = left->left;

			while (inward != nullptr) {
				current->id = inward->id;
				current->weight = inward->weight;
				current = inward;
				inward = inward->left;
			}

			Neuron* last = current;

			if (last->left) last->left->right = last->right;
			if (last->right) last->right->left = last->left;
			if (last->up) last->up->down = last->down;
			if (last->down) last->down->up = last->up;

			if (last == owner->top_left) {
				if (last->down) {
					Neuron* promoted = last->down;
					promoted->up = nullptr;
					promoted->left = last->left;
					promoted->right = last->right;
					if (last->right) last->right->left = promoted;
					owner->top_left = promoted;
				}
				else if (last->right) {
					owner->top_left = last->right;
				}
				else {
					owner->top_left = nullptr;
				}
			}

			delete last;
			owner->current_count--;
		}


		if (right) {
			Neuron* current = right;
			Neuron* inward = right->right;

			while (inward != nullptr) {
				current->id = inward->id;
				current->weight = inward->weight;
				current = inward;
				inward = inward->right;
			}

			Neuron* last2 = current;

			if (last2->left) last2->left->right = last2->right;
			if (last2->right) last2->right->left = last2->left;
			if (last2->up) last2->up->down = last2->down;
			if (last2->down) last2->down->up = last2->up;

			if (last2 == owner->top_left) {
				if (last2->down) {
					Neuron* promoted = last2->down;
					promoted->up = nullptr;
					promoted->left = last2->left;
					promoted->right = last2->right;
					if (last2->right) last2->right->left = promoted;
					owner->top_left = promoted;
				}
				else if (last2->right) {
					owner->top_left = last2->right;
				}
				else {
					owner->top_left = nullptr;
				}
			}

			delete last2;
			owner->current_count--;
		}


		if (up) {
			Neuron* current = up;
			Neuron* inward = up->up;

			while (inward != nullptr) {
				current->id = inward->id;
				current->weight = inward->weight;
				current = inward;
				inward = inward->up;
			}

			Neuron* last3 = current;

			if (last3->left) last3->left->right = last3->right;
			if (last3->right) last3->right->left = last3->left;
			if (last3->up) last3->up->down = last3->down;
			if (last3->down) last3->down->up = last3->up;

			if (last3 == owner->top_left) {
				if (last3->down) {
					Neuron* promoted = last3->down;
					promoted->up = nullptr;
					promoted->left = last3->left;
					promoted->right = last3->right;
					if (last3->right) last3->right->left = promoted;
					owner->top_left = promoted;
				}
				else if (last3->right) {
					owner->top_left = last3->right;
				}
				else {
					owner->top_left = nullptr;
				}
			}

			delete last3;
			owner->current_count--;
		}


		if (down) {
			Neuron* current = down;
			Neuron* inward = down->down;

			while (inward != nullptr) {
				current->id = inward->id;
				current->weight = inward->weight;
				current = inward;
				inward = inward->down;
			}

			Neuron* last4 = current;

			if (last4->left) last4->left->right = last4->right;
			if (last4->right) last4->right->left = last4->left;
			if (last4->up) last4->up->down = last4->down;
			if (last4->down) last4->down->up = last4->up;

			if (last4 == owner->top_left) {
				if (last4->down) {
					Neuron* promoted = last4->down;
					promoted->up = nullptr;
					promoted->left = last4->left;
					promoted->right = last4->right;
					if (last4->right) last4->right->left = promoted;
					owner->top_left = promoted;
				}
				else if (last4->right) {
					owner->top_left = last4->right;
				}
				else {
					owner->top_left = nullptr;
				}
			}

			delete last4;
			owner->current_count--;
		}

		rebuildLayerForwardSynapses(owner);
		if (owner->prev) rebuildLayerForwardSynapses(owner->prev);

		n->weight = n->weight / 2.0; // FIXED: was completely missing

		orderColumns(head_layer);
		checkStatus();
	}

	Neuron* mergeNeurons(Synapse* s) {
		// for merginging the neurons

		if (!s) return nullptr;


		// checking the owner of the seynapse
		Neuron* sourceNeuron = nullptr;
		Layer* sourceLayer = nullptr;

		for (Layer* l = head_layer; l != nullptr && !sourceNeuron; l = l->next) {

			for (Neuron* colt = l->top_left; colt != nullptr && !sourceNeuron; colt = colt->right) {
				for (Neuron* nn = colt; nn != nullptr; nn = nn->down) {
					bool found = false;
					for (Synapse* sy = nn->head_axon; sy != nullptr; sy = sy->next_synapse)
						if (sy == s) { found = true; break; }
					if (found) { sourceNeuron = nn; sourceLayer = l; break; }
				}
			}

		}

		if (!sourceNeuron || !sourceLayer) return nullptr;

		Neuron* targetNeuron = s->target_neuron;
		Layer* outerLayer = sourceLayer->next;

		sourceNeuron->weight = floor((sourceNeuron->weight + targetNeuron->weight) / 2.0);
		Synapse* travelingAxons = targetNeuron->head_axon;
		targetNeuron->head_axon = nullptr;

		Neuron* vacUp = targetNeuron->up;
		Neuron* vacDown = targetNeuron->down;
		Neuron* vacLeft = targetNeuron->left;
		Neuron* vacRight = targetNeuron->right;
		Layer* vacLayer = outerLayer;

		delete targetNeuron;
		outerLayer->current_count--;

		// Vacancy promoption part

		while (true) {
			Layer* nextOuterLayer = vacLayer->next;
			Synapse* best = nextOuterLayer ? findBestCandidate(travelingAxons) : nullptr;

			Synapse* freeMe = travelingAxons;
			while (freeMe) {
				Synapse* nx = freeMe->next_synapse;
				if (freeMe != best) delete freeMe;
				freeMe = nx;
			}

			if (!best) {
				bypassVacantSlot(vacLayer, vacUp, vacDown, vacLeft, vacRight);
				break;
			}

			Neuron* promoted = best->target_neuron;
			delete best;

			Neuron* promUp = promoted->up;
			Neuron* promDown = promoted->down;
			Neuron* promLeft = promoted->left;
			Neuron* promRight = promoted->right;
			Synapse* promotedAxons = promoted->head_axon;
			promoted->head_axon = nullptr;

			placeIntoVacancy(promoted, vacUp, vacDown, vacLeft, vacRight, vacLayer);
			vacLayer->current_count++;
			nextOuterLayer->current_count--;

			vacUp = promUp;
			vacDown = promDown;
			vacLeft = promLeft;
			vacRight = promRight;
			vacLayer = nextOuterLayer;
			travelingAxons = promotedAxons;
		}

		for (Layer* l = head_layer; l != nullptr; l = l->next) rebuildLayerForwardSynapses(l);

		checkStatus();

		return sourceNeuron;
	}



	Neuron* mergeColumns(Neuron* columnA_any, Neuron* columnB_any) {
		// for merging of the columns
		// both neurons are the head neurons of the respective column
		if (!columnA_any || !columnB_any) return nullptr;

		Layer* owner = ownerLayer(columnA_any);
		if (!owner) return nullptr;

		int colA = 0, dummyRow = 0;
		findNeuronPos(owner, dummyRow, colA, columnA_any);

		int colB = colA + 1;

		Neuron* firstN = columnA_any;
		Neuron* secondN = columnB_any;

		Neuron* leftB = (colA > 0) ? columnTop(owner, colA - 1) : nullptr;
		Neuron* rightB = columnTop(owner, colB + 1);

		double newWeight;
		int row = 0;
		Neuron* oldN = nullptr;
		Neuron* mergedTop = nullptr;
		Neuron* mergedBottom = nullptr;


		while (firstN || secondN) {
			Neuron* newNeuron;

			if (firstN && secondN) {

				newWeight = floor((firstN->weight + secondN->weight) / 2.0);

				newNeuron = new Neuron();
				newNeuron->weight = newWeight;
				newNeuron->id = firstN->id;
				newNeuron->left = newNeuron->right = newNeuron->up = newNeuron->down = nullptr;
				newNeuron->head_axon = nullptr;

				clearAxons(firstN);
				clearAxons(secondN);

				oldN = firstN;
				firstN = firstN->down;
				delete oldN;
				oldN = secondN;
				secondN = secondN->down;
				delete oldN;

			}
			else if (firstN) {
				newNeuron = firstN;
				clearAxons(newNeuron);
				firstN = firstN->down;
			}
			else {
				newNeuron = secondN;
				clearAxons(newNeuron);
				secondN = secondN->down;
			}

			newNeuron->left = leftB;
			newNeuron->right = rightB;
			if (leftB) leftB->right = newNeuron;
			else if (row == 0) owner->top_left = newNeuron;
			if (rightB) rightB->left = newNeuron;
			mergedBottom = newNeuron;

			if (leftB) leftB = leftB->down;
			if (rightB) rightB = rightB->down;

			row++;
		}

		rebuildLayerForwardSynapses(owner);
		rebuildLayerForwardSynapses(owner->prev);

		return mergedTop;

	}


	void rebuildLayerForwardSynapses(Layer* l) {
		if (!l || !l->next) return;
		int col = 0;

		for (Neuron* colt = l->top_left; colt; colt = colt->right, col++) {
			int row = 0;
			for (Neuron* n = colt; n; n = n->down, row++) {
				clearAxons(n);
				Neuron* aligned = silentNeuronReturnByPosition(l->next->layerId, row, col);
				if (aligned) {
					addSynapse(n, aligned->up, 'U');
					addSynapse(n, aligned->down, 'D');
					addSynapse(n, aligned->left, 'L');
					addSynapse(n, aligned->right, 'R');
				}
			}
		}
	}

	void rowLinkage(Layer* l) {
		int maxHeight = 0;
		for (Neuron* c = l->top_left; c; c = c->right) {
			int h = 0;
			for (Neuron* n = c; n; n = n->down)  h++;
			if (h > maxHeight) maxHeight = h;
		}

		for (int row = 0; row < maxHeight; row++) {
			Neuron* prev = nullptr;
			for (Neuron* c = l->top_left; c; c = c->right) {
				Neuron* n = c;
				for (int r = 0; r < row && n; r++) n = n->down;
				if (n) {
					n->left = prev;
					if (prev) prev->right = n;
					prev = n;
				}
			}
			if (prev) prev->right = nullptr;
		}

	}

	Layer* mergeLayers(Layer* a, Layer* b) {
		// merging the two layers 
		// a will always be first one and b is always going to be second one

		if (a == nullptr || b == nullptr)
			return nullptr;

		Neuron* temp;


		Neuron* colt1 = a->top_left;
		Neuron* colt2 = b->top_left;
		Neuron* prevColumnTopInA = nullptr;

		while (colt1 != nullptr || colt2 != nullptr) {
			Neuron* s1 = colt1;
			Neuron* s2 = colt2;
			Neuron* aboveIna = nullptr;
			Neuron* thisColumnTopInA = colt1;

			while (s1 != nullptr || s2 != nullptr) {

				if (s1 != nullptr && s2 != nullptr) {
					s1->weight = floor((s1->weight + s2->weight) / 2);

					clearAxons(s2);
					temp = s2;
					s2 = s2->down;
					delete temp;
					temp = nullptr;
					if (b->current_count > 0)
						b->current_count--;
					aboveIna = s1;
					s1 = s1->down;
				}
				else if (s1 != nullptr) {
					aboveIna = s1;
					s1 = s1->down;
				}
				else {
					Neuron* migrating = s2;
					s2 = s2->down;

					migrating->up = aboveIna;
					migrating->down = nullptr;
					if (aboveIna) aboveIna->down = migrating;
					else thisColumnTopInA = migrating;

					aboveIna = migrating;
					if (b->current_count > 0) b->current_count--;
					a->current_count;
				}

			}

			if (colt1 == nullptr && thisColumnTopInA != nullptr) {
				if (prevColumnTopInA) prevColumnTopInA->right = thisColumnTopInA;
				else a->top_left = thisColumnTopInA;
			}

			prevColumnTopInA = thisColumnTopInA;
			colt1 = colt1 ? colt1->right : nullptr;
			colt2 = colt2 ? colt2->right : nullptr;

		}

		rowLinkage(a);

		a->next = b->next;

		if (b->next)
			b->next->prev = a;

		if (b == tail_layer) tail_layer = a;

		delete b;
		b = nullptr;

		rebuildLayerForwardSynapses(a);

		if (a->prev) rebuildLayerForwardSynapses(a->prev);

		checkStatus();



		return a;
	}


	void removeEmptyLayer(Layer* l) {
		if (!l) return;

		if (l->current_count > 0) return;

		if (l->layerId == 0) {
			head_layer = l->next;
		}
		else {
			l->prev->next = l->next;
			if (l->next) l->next->prev = l->prev;
			rebuildLayerForwardSynapses(l->prev);
		}

		deleteLayer(l->layerId);

	}



	//===============Printng/Exportng============
	void printLayer(int layerid) {
		// for printing the layer 
		// this is als going to be helping us in the debugging part as well

		//for (Layer* l = head_layer; l != nullptr; l = l->next) {

		//	Neuron* n;

		//	n = l->top_left;

		//	cout << "Layer : " << l->layerId << endl;

		//	while (n != nullptr) {

		//		Neuron* st = n;

		//		for (; n->right != nullptr; n = n->right) {
		//			cout << n->id << " ";
		//		}

		//		cout << n->id << endl;

		//		n = st->down;
			//}


		//}


		Layer* l = getLayerById(layerid);

		if (l == nullptr) return;

		for (int row = 0; row < N; row++) {
			for (int col = 0; col < N; col++) {
				Neuron* n = silentNeuronReturnByPosition(layerid, row, col);

				if (n) {
					cout << "  ID: " << n->id << " W: " << n->weight;

				}
				else {
					cout << " empty ";
				}
			}
			cout << endl;
		}



	}
	void exportMesh(const std::string& fileName) {
		// the man functin for export the mesh in to a text file

		ofstream fout(fileName);

		if (!fout) {
			cout << " Coudln't be able to open the file" << endl;
			return;
		}

		fout << "=========================================NEUROMESH==================================" << endl;

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			fout << "Layer ID " << l->layerId << endl << endl;
			int col = 0;

			for (Neuron* colt = l->top_left; colt != nullptr; colt = colt->right, col++) {
				int row = 0;

				for (Neuron* n = colt; n != nullptr; n = n->down, row++) {
					fout << "Neuron ID:" << n->id << " Weight: " << n->weight << " Axon list : ";

					Synapse* s = n->head_axon;

					if (s == nullptr) {
						fout << "  NONE";
					}
					int temp = 1;
					while (s != nullptr) {
						fout << " " << temp << "  ==>Target ID: ";
						if (s->target_neuron) fout << s->target_neuron->id;
						else fout << " NONE ";
						s = s->next_synapse;
						temp++;
					}

					fout << endl << endl;

				}
			}

			fout << "=========================================================================================" << endl;

		}

		fout.close();

	}

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
	char forwardPropagate(char inputLetter) {
		// doing the forward propogation thing

		if (head_layer == nullptr || head_layer->top_left == nullptr) return '0';

		char ch = inputLetter;
		int chn = toupper(inputLetter) - 'A';
		Synapse* max = nullptr;
		double maxWeight = 0;
		char maxDir;
		max = head_layer->top_left->head_axon;
		maxWeight = head_layer->top_left->head_axon->weight;




		for (Neuron* nod = head_layer->top_left; nod; ) {

			max = nod->head_axon;
			if (nod->head_axon == nullptr) break;
			maxWeight = nod->head_axon->weight;
			maxDir = nod->head_axon->direction;

			for (Synapse* s = nod->head_axon; s; s = s->next_synapse) {
				if (s->weight > maxWeight) {
					maxWeight = s->weight;
					max = s;
					maxDir = s->direction;
				}
				else if (s->weight == maxWeight) {
					int score1 = 0; // score for the weight of the old
					int score2 = 0; // score for the wieght of the nex

					if (s->direction == 'L') score1 = 4;
					else if (s->direction == 'R') score1 = 3;
					else if (s->direction == 'U') score1 = 2;
					else score1 = 1;

					if (maxDir == 'L') score2 = 4;
					else if (maxDir == 'R') score2 = 3;
					else if (maxDir == 'U') score2 = 2;
					else score2 = 1;

					if (score1 > score2) {
						maxWeight = s->weight;
						max = s;
						maxDir = s->direction;
					}


				}
			}

			max->is_active = true;

			chn = ((chn + (int)fmod(max->weight, 26.0)) % 26 + 26) % 26;


			nod = max->target_neuron;

		}

		char result = chn + 'A';

		return result;


	}

	void backwardPropagate(char targetLetter, char gen) {

		// first finding the maxSynapse 



		if (head_layer == nullptr || head_layer->top_left == nullptr) return;

		char genL = gen;
		if (genL == targetLetter) return;
		int genl = toupper(genL) - 'A';
		int gent = toupper(targetLetter) - 'A';

		double error = gent - genl;



		Synapse* maxSynapse = head_layer->top_left->head_axon;

		for (Neuron* nod = head_layer->top_left; nod; ) {
			for (Synapse* s = nod->head_axon; s; s = s->next_synapse) {
				if (s->is_active == true) {
					nod = s->target_neuron;
					s->weight += error;

					break;
				}
			}
			break;
		}








	}

private:

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!! Helper functions sections !!!!!!!!!!!!!!!!

	int dirPriority(char d) {
		if (d == 'L') return 0;
		if (d == 'R') return 1;
		if (d == 'U') return 2;
		return 3;
	}


	Synapse* findBestCandidate(Synapse* axonList) {
		Synapse* best = nullptr;

		for (Synapse* s = axonList; s != nullptr; s = s->next_synapse) {
			if (best == nullptr) { best = s; continue; }
			if (s->weight > best->weight) { best = s; continue; }
			if (s->weight == best->weight && dirPriority(s->direction) < dirPriority(best->direction)) best = s;
		}
		return best;
	}


	void placeIntoVacancy(Neuron* moving, Neuron* vacUp, Neuron* vacDown, Neuron* vacLeft, Neuron* vacRight, Layer* vacLayer) {
		moving->up = vacUp;
		moving->down = vacDown;
		moving->left = vacLeft;
		moving->right = vacRight;
		if (vacUp) vacUp->down = moving;
		if (vacDown) vacDown->up = moving;
		if (vacLeft) vacLeft->right = moving;
		if (vacRight) vacRight->left = moving;
		if (vacUp == nullptr && vacLeft == nullptr) vacLayer->top_left = moving;
	}

	void bypassVacantSlot(Layer* layer, Neuron* up, Neuron* down, Neuron* left, Neuron* right) {
		if (up != nullptr) {
			up->down = down;
			if (down) down->up = up;
			if (left) left->right = right;
			if (right) right->left = left;

		}
		else {
			if (down != nullptr) {
				down->up = nullptr;
				down->left = left;
				down->right = right;
				if (left) left->right = down;
				else layer->top_left = down;

			}
			else {
				if (left) left->right = right;
				else layer->top_left = right;
				if (right) right->left = left;
			}
		}
	}


	void findNeuronPos(Layer* l, int& row, int& col, Neuron* n) {
		if (!l) return;

		row = 0;
		col = 0;

		for (Neuron* coln = l->top_left; coln; coln = coln->right, col++) {

			row = 0;
			for (Neuron* rown = coln; rown; rown = rown->down, row++) {

				if (rown->id == n->id) return;

			}

		}

	}

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

			if (col > 0) {
				Neuron* leftNeighbor = silentNeuronReturnByPosition(l->layerId, row, col - 1);
				if (leftNeighbor) {
					n->left = leftNeighbor;
					leftNeighbor->right = n;
				}

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

	//! column ordering and management functions including the column mergin functions


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

	double layerWeight(Layer* l) {
		if (!l) return 0.0;

		double total = 0.0;

		for (Neuron* col = l->top_left; col != nullptr; col = col->right) {
			for (Neuron* n = col; n; n = n->down) {
				total += n->weight;
			}
		}

		return total;
	}

	void checkStatus() {
		if (!head_layer || head_layer->top_left) return;

		while (true) {
			bool changed = false;

			for (Layer* l = head_layer; l && !changed; l = l->next) {

				for (Neuron* col = l->top_left; col != nullptr && !changed; col = col->right) {
					for (Neuron* n = col; n != nullptr; n = n->down) {

						if (n->weight > pthreshold) {

							pruneNeuron(n);
							changed = true;
							break;

						}

					}


				}

			}

			if (changed) continue;


			for (Layer* l = head_layer; l && !changed; l = l->next) {

				for (Neuron* col = l->top_left; col != nullptr && !changed; col = col->right) {
					for (Neuron* n = col; n != nullptr; n = n->down) {

						for (Synapse* s = n->head_axon; s; s = s->next_synapse) {
							if (s->weight > fthreshold) {
								mergeNeurons(s);
								changed = true;
								break;
							}
						}

					}


				}

			}

			if (changed) continue;

			for (Layer* l = head_layer; l && l->next != nullptr; l = l->next) {
				Layer* nex = l->next;

				if (layerWeight(l) == layerWeight(nex)) {
					mergeLayers(l, nex);
					changed = true;
					break;
				}
			}

			if (changed) continue;


			for (Layer* l = head_layer; l && l->next != nullptr; l = l->next) {
				Layer* nex = l->next;


				if (l->current_count == 0) {
					removeEmptyLayer(l);
					changed = true;
					break;
				}


			}

			if (!changed) break;

		}



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


	void orderColumns(Layer* head_layer) {
		// basically for ordering the columns

		double weight1;
		double weight2;

		int col = 0;
		int row = 0;

		Neuron* secondN = nullptr;
		Neuron* leftNeighbor = nullptr;
		Neuron* rightNeighbor = nullptr;


		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			bool changed = true;

			while (changed) {
				changed = false;
				col = 0;

				for (Neuron* n = l->top_left; n && n->right != nullptr; n = n->right, col++) {
					weight1 = columnWeight(l, col);
					weight2 = columnWeight(l, col + 1);

					if (weight1 > weight2) {
						// checking fi the total weight of one column is more than the total weight of the other column

						Neuron* firstN = n;
						secondN = n->right;
						row = 0;

						while (firstN != nullptr || secondN != nullptr) {
							Neuron* nowAtCol = secondN;
							Neuron* nowAtNextCol = firstN;


							leftNeighbor = (col > 0) ? silentNeuronReturnByPosition(l->layerId, row, col - 1) : nullptr;
							rightNeighbor = silentNeuronReturnByPosition(l->layerId, row, col + 2);

							if (nowAtCol) {
								nowAtCol->left = leftNeighbor;
								nowAtCol->right = nowAtNextCol;
							}
							if (nowAtNextCol) {
								nowAtNextCol->left = nowAtCol;
								nowAtNextCol->right = rightNeighbor;
							}

							if (leftNeighbor) leftNeighbor->right = nowAtCol;
							else if (row == 0) l->top_left = nowAtCol;

							if (rightNeighbor) rightNeighbor->left = nowAtNextCol;

							if (nowAtCol) clearAxons(nowAtCol);
							if (nowAtNextCol) clearAxons(nowAtNextCol);

							Neuron* aligned1 = nullptr;

							if (nowAtCol) {
								if (l->next) aligned1 = silentNeuronReturnByPosition(l->next->layerId, row, col);
								if (aligned1) {

									addSynapse(nowAtCol, aligned1->up, 'U');
									addSynapse(nowAtCol, aligned1->down, 'D');
									addSynapse(nowAtCol, aligned1->left, 'L');
									addSynapse(nowAtCol, aligned1->right, 'R');


								}
							}

							aligned1 = nullptr;


							if (nowAtNextCol) {
								if (l->next) aligned1 = silentNeuronReturnByPosition(l->next->layerId, row, col + 1);

								if (aligned1) {
									addSynapse(nowAtNextCol, aligned1->up, 'U');
									addSynapse(nowAtNextCol, aligned1->down, 'D');
									addSynapse(nowAtNextCol, aligned1->left, 'L');
									addSynapse(nowAtNextCol, aligned1->right, 'R');

								}
							}

							if (firstN) firstN = firstN->down;
							if (secondN) secondN = secondN->down;
							row++;



						}

						changed = true;
						break;
					}
					else if (weight1 == weight2) {
						mergeColumns(n, n->right);
						changed = true;
						break;
					}
				}
			}
		}


	}


public:

	Layer* getLayerById(int layerId) {
		for (Layer* l = head_layer; l != nullptr; l = l->next) {
			if (l->layerId == layerId) {
				return l;
			}


		}

		return nullptr;
	}

	Layer* getHeadLayer() {
		return head_layer;
	}

	int getN() {
		return N;
	}

	Layer* getOwnerLayer(Neuron* n) {
		return ownerLayer(n);
	}

	Neuron* navigateFrom(int& layerId, int& row, int& col, int choice) {

	}


	double columnWeight(Layer* l, int col) {
		double sum = 0.0;
		for (Neuron* n = columnTop(l, col); n != nullptr; n = n->down) {
			sum += n->weight;
		}

		return sum;
	}





};


enum class AppState { Menu, Simulation, Controls, About };

class RaylibGUI {
private:

	NeuroMesh& mesh;
	int currentLayer;
	int currentRow;
	int currentCol;

	bool running;

	int screenWidth;
	int screenHeight;

	float gridStartX;
	float gridStartY;
	float spacing;
	float nodeRadius;

	// ---- app state / navigation ----
	AppState state;
	int menuSelection; // 0 = Simulation, 1 = Controls, 2 = About

	// ---- smooth layer transition ----
	float layerTransition;   // 0 = just switched, 1 = settled
	int prevLayerId;         // layer we are fading away from
	int transitionDir;       // +1 = moving to a deeper layer, -1 = back
	static constexpr float TRANSITION_DURATION = 0.32f;

	// ---- dark blue theme ----
	const Color BG_DEEP = Color{ 8,   11,  22,  255 };
	const Color BG_PANEL = Color{ 15,  20,  38,  255 };
	const Color BG_PANEL_LIGHT = Color{ 22,  29,  52,  255 };
	const Color BORDER_COL = Color{ 35,  46,  74,  255 };
	const Color BLUE_DEEP = Color{ 38,  92,  170, 255 };
	const Color BLUE_PRIMARY = Color{ 64,  156, 255, 255 };
	const Color BLUE_LIGHT = Color{ 130, 205, 255, 255 };
	const Color BLUE_DIM = Color{ 60,  95,  150, 120 };
	const Color BLUE_BRIGHT = Color{ 120, 195, 255, 240 };
	const Color ACCENT_GOLD = Color{ 255, 196, 92,  255 };
	const Color TEXT_PRIMARY = Color{ 225, 232, 245, 255 };
	const Color TEXT_MUTED = Color{ 110, 128, 158, 255 };

public:
	RaylibGUI(NeuroMesh& mesh) : mesh(mesh) {
		currentLayer = 0;
		currentRow = 0;
		currentCol = 0;

		running = true;

		screenWidth = 1280;
		screenHeight = 760;

		state = AppState::Menu;
		menuSelection = 0;

		layerTransition = 1.0f;
		prevLayerId = 0;
		transitionDir = 1;

		computeLayout();
	}

	void run() {
		if (mesh.getHeadLayer() == nullptr)
			return;

		initialize();

		while (!WindowShouldClose() && running) {
			handleInput();

			if (state == AppState::Simulation) validateSelection();

			if (layerTransition < 1.0f) {
				layerTransition += GetFrameTime() / TRANSITION_DURATION;
				if (layerTransition > 1.0f) layerTransition = 1.0f;
			}

			BeginDrawing();

			ClearBackground(BG_DEEP);

			draw();

			EndDrawing();
		}

		CloseWindow();
	}

private:

	void initialize() {
		InitWindow(screenWidth, screenHeight, "NeuroMesh");
		SetExitKey(KEY_NULL); // otherwise Esc would close the whole window

		// Fit the window to the ACTUAL monitor, with room left over for the
		// title bar and taskbar - a fixed 1280x760 can end up with its
		// title bar pushed off-screen on a 1366x768 display (8px of slack
		// isn't enough), which looks exactly like "the window never opened"
		// even though the process is running fine.
		int monitor = GetCurrentMonitor();
		int monitorW = GetMonitorWidth(monitor);
		int monitorH = GetMonitorHeight(monitor);

		int margin = 110; // title bar + taskbar + a little breathing room
		int maxW = monitorW - 60;
		int maxH = monitorH - margin;

		if (screenWidth > maxW) screenWidth = maxW;
		if (screenHeight > maxH) screenHeight = maxH;

		SetWindowSize(screenWidth, screenHeight);
		SetWindowPosition((monitorW - screenWidth) / 2, (monitorH - screenHeight) / 2 - 20);

		computeLayout(); // redo the grid math against the final window size

		SetTargetFPS(60);
	}

	// Sizes the grid so the main layer and the next-layer preview both fit
	// on screen, whatever N is and whatever the window ended up sized to.
	void computeLayout() {
		int n = mesh.getN();
		if (n < 2) n = 2;

		float available = (float)screenWidth - 600.0f; // room right of the info panel
		if (available < 200.0f) available = 200.0f;
		float gap = 130.0f;       // space between main grid and preview
		float maxSpacing = (available - gap) / ((n - 1) * 1.6f);

		spacing = maxSpacing < 90.0f ? maxSpacing : 90.0f;
		if (spacing < 40.0f) spacing = 40.0f;
		nodeRadius = spacing * 0.18f;

		gridStartX = 500.0f;
		gridStartY = 210.0f;
	}

	// ---------------------------------------------------------------- input --
	void handleInput() {
		if (state == AppState::Menu) { handleMenuInput(); return; }

		if (state == AppState::Controls || state == AppState::About) {
			if (IsKeyPressed(KEY_ESCAPE)) state = AppState::Menu;
			return;
		}

		// ---- Simulation ----
		if (IsKeyPressed(KEY_ESCAPE)) { state = AppState::Menu; return; }

		if (IsKeyPressed(KEY_ONE)) {
			moveUp();
		}
		else if (IsKeyPressed(KEY_TWO)) {
			moveDown();
		}
		else if (IsKeyPressed(KEY_THREE)) {
			moveLeft();
		}
		else if (IsKeyPressed(KEY_FOUR)) {
			moveRight();
		}
		else if (IsKeyPressed(KEY_FIVE)) {
			previousLayer();
		}
		else if (IsKeyPressed(KEY_SIX)) {
			nextLayer();
		}
		else if (IsKeyPressed(KEY_A)) {
			int id;
			double weight;

			cin >> id;
			cin >> weight;

			mesh.insertNeuron(id, weight);
		}
		else if (IsKeyPressed(KEY_B)) {
			int id;
			cin >> id;
			mesh.deleteNeuron(id);
		}
		else if (IsKeyPressed(KEY_C)) {
			int id;
			cin >> id;
			mesh.deleteLayer(id);
		}
		else if (IsKeyPressed(KEY_D)) {
			int id;
			cin >> id;
			Neuron* n = mesh.silentNeuronReturnById(id);
			mesh.pruneNeuron(n);
		}
		else if (IsKeyPressed(KEY_T)) {
			int id;
			cin >> id;
			Neuron* n = mesh.silentNeuronReturnById(id);
			if (n != nullptr) mesh.mergeNeurons(n->head_axon);
		}
		else if (IsKeyPressed(KEY_H)) {
			char input, expected;
			char result;
			cin >> input;
			cin >> expected;
			result = mesh.forwardPropagate(input);
			cout << result;
			cout << endl;
			mesh.backwardPropagate(expected, result);
		}
		else if (IsKeyPressed(KEY_SEVEN)) {
			running = false;
		}
	}

	void handleMenuInput() {
		if (IsKeyPressed(KEY_DOWN)) {
			menuSelection = (menuSelection + 1) % 3;
		}
		else if (IsKeyPressed(KEY_UP)) {
			menuSelection = (menuSelection + 2) % 3;
		}
		else if (IsKeyPressed(KEY_ENTER)) {
			if (menuSelection == 0) {
				state = AppState::Simulation;
				layerTransition = 1.0f; // entering fresh - no animation
			}
			else if (menuSelection == 1) state = AppState::Controls;
			else state = AppState::About;
		}
		else if (IsKeyPressed(KEY_ESCAPE)) {
			running = false; // Esc on the main menu quits
		}
	}

	// -------------------------------------------------------------- drawing --
	void draw() {
		if (state == AppState::Menu) { drawMenu(); return; }
		if (state == AppState::Controls) { drawControlsScreen(); return; }
		if (state == AppState::About) { drawAboutScreen(); return; }

		drawSimulation();
	}

	// ---- small helpers ----
	float easeOutCubic(float t) {
		float u = 1.0f - t;
		return 1.0f - u * u * u;
	}

	Color lerpColor(Color a, Color b, float t) {
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		return Color{
			(unsigned char)(a.r + (b.r - a.r) * t),
			(unsigned char)(a.g + (b.g - a.g) * t),
			(unsigned char)(a.b + (b.b - a.b) * t),
			(unsigned char)(a.a + (b.a - a.a) * t)
		};
	}

	// One S-shaped curve between two points (also used to place the
	// travelling pulses, so they follow the line exactly).
	Vector2 curvePoint(Vector2 a, Vector2 b, float t) {
		float s = t * t * (3.0f - 2.0f * t);
		return Vector2{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * s };
	}

	void drawCurve(Vector2 a, Vector2 b, float thick, Color c) {
		const int segments = 20;
		Vector2 prev = a;
		for (int i = 1; i <= segments; i++) {
			Vector2 p = curvePoint(a, b, (float)i / segments);
			DrawLineEx(prev, p, thick, c);
			prev = p;
		}
	}

	void drawTopBar(const char* right) {
		DrawRectangle(0, 0, screenWidth, 80, BG_PANEL);
		DrawRectangle(0, 79, screenWidth, 1, BORDER_COL);
		DrawText("NeuroMesh", 40, 26, 28, BLUE_PRIMARY);

		if (right != nullptr) {
			int w = MeasureText(right, 15);
			DrawText(right, screenWidth - w - 40, 32, 15, TEXT_MUTED);
		}
	}

	// slowly drifting constellation behind the main menu
	void drawBackdropNetwork() {
		float t = (float)GetTime();
		const int count = 18;
		Vector2 pts[count];

		for (int i = 0; i < count; i++) {
			pts[i].x = (0.5f + 0.48f * sinf(i * 1.7f + t * 0.12f)) * screenWidth;
			pts[i].y = (0.5f + 0.48f * cosf(i * 2.3f + t * 0.10f)) * screenHeight;
		}

		for (int i = 0; i < count; i++) {
			for (int j = i + 1; j < count; j++) {
				float dx = pts[i].x - pts[j].x;
				float dy = pts[i].y - pts[j].y;
				float dist = sqrtf(dx * dx + dy * dy);
				if (dist < 240.0f) {
					float a = (1.0f - dist / 240.0f) * 0.35f;
					DrawLineEx(pts[i], pts[j], 1.0f, Fade(BLUE_PRIMARY, a));
				}
			}
		}
		for (int i = 0; i < count; i++) {
			DrawCircleV(pts[i], 3.0f, Fade(BLUE_LIGHT, 0.55f));
		}
	}

	// ---- main menu ----
	void drawMenu() {
		drawBackdropNetwork();

		const char* title = "NeuroMesh";
		int titleSize = 64;
		int tw = MeasureText(title, titleSize);
		DrawText(title, screenWidth / 2 - tw / 2, 120, titleSize, BLUE_PRIMARY);

		const char* subtitle = "A pointer-based dynamic neural substrate";
		int subSize = 20;
		int stw = MeasureText(subtitle, subSize);
		DrawText(subtitle, screenWidth / 2 - stw / 2, 198, subSize, TEXT_MUTED);

		const char* items[3] = { "Simulation", "Controls", "About" };
		const char* notes[3] = {
			"Explore the live mesh, layer by layer",
			"Every key and what it does",
			"What this project is and how it works"
		};

		float cardW = 440.0f, cardH = 84.0f, gap = 18.0f;
		float startY = 280.0f;

		for (int i = 0; i < 3; i++) {
			float x = screenWidth / 2 - cardW / 2;
			float y = startY + i * (cardH + gap);
			bool sel = (i == menuSelection);

			Rectangle rec = { x, y, cardW, cardH };
			DrawRectangleRounded(rec, 0.16f, 8, sel ? BG_PANEL_LIGHT : BG_PANEL);
			DrawRectangleLinesEx(rec, sel ? 2.0f : 1.0f, sel ? ACCENT_GOLD : BORDER_COL);

			if (sel) DrawRectangle((int)x + 2, (int)y + 14, 4, (int)cardH - 28, ACCENT_GOLD);

			DrawText(items[i], (int)x + 32, (int)y + 16, 26, sel ? Color{ 255, 255, 255, 255 } : TEXT_PRIMARY);
			DrawText(notes[i], (int)x + 32, (int)y + 50, 15, sel ? BLUE_LIGHT : TEXT_MUTED);
		}

		const char* hint = "Up / Down  navigate      Enter  select      Esc  quit";
		int hs = 15;
		int hw = MeasureText(hint, hs);
		DrawText(hint, screenWidth / 2 - hw / 2, screenHeight - 50, hs, TEXT_MUTED);
	}

	// ---- controls screen ----
	void drawControlsScreen() {
		drawTopBar("Esc  back to menu");
		DrawText("Controls", 80, 120, 36, TEXT_PRIMARY);
		DrawRectangle(80, 168, 120, 3, BLUE_PRIMARY);

		struct KeyRow { const char* key; const char* desc; };
		KeyRow rows[] = {
			{ "1 / 2 / 3 / 4", "Move selection up / down / left / right" },
			{ "5 / 6",         "Go to the previous / next layer" },
			{ "A",             "Insert a neuron  (type id and weight in the console)" },
			{ "B",             "Delete a neuron  (type its id in the console)" },
			{ "C",             "Delete a layer  (type its id in the console)" },
			{ "D",             "Prune a neuron  (type its id in the console)" },
			{ "T",             "Fuse a neuron's first synapse  (type its id in the console)" },
			{ "H",             "Forward + backward propagate  (input, expected in the console)" },
			{ "7",             "Quit" },
			{ "Esc",           "Back to the menu" },
		};

		float y = 196.0f;
		for (auto& row : rows) {
			Rectangle keyBox = { 80.0f, y, 170.0f, 36.0f };
			DrawRectangleRounded(keyBox, 0.3f, 6, BG_PANEL);
			DrawRectangleLinesEx(keyBox, 1.0f, BORDER_COL);
			int kw = MeasureText(row.key, 17);
			DrawText(row.key, (int)(80 + 85 - kw / 2), (int)y + 9, 17, BLUE_PRIMARY);

			DrawText(row.desc, 275, (int)y + 9, 18, TEXT_PRIMARY);
			y += 48.0f;
		}
	}

	// ---- about screen ----
	void drawAboutScreen() {
		drawTopBar("Esc  back to menu");
		DrawText("About", 80, 120, 36, TEXT_PRIMARY);
		DrawRectangle(80, 168, 120, 3, BLUE_PRIMARY);

		const char* lines[] = {
			"NeuroMesh is a pointer-based data structure project: a dynamically",
			"growing mesh of layers, each holding an N x N grid of neurons that",
			"are linked only through up / down / left / right pointers.",
			"No arrays and no STL containers are used anywhere in the mesh.",
			"",
			"Neurons connect forward to the next layer through synapses, following",
			"a fixed orthogonal rule. The mesh restructures itself at runtime:",
			"overloaded neurons prune their neighbours, strong synapses fuse",
			"neurons across layers, and columns or whole layers merge or vanish",
			"as their weights change - all through direct pointer relinking.",
			"",
			"This viewer reads the live structure directly, so what you see is",
			"the real mesh, not a separate model of it.",
		};

		float y = 200.0f;
		for (auto& line : lines) {
			DrawText(line, 80, (int)y, 19, TEXT_PRIMARY);
			y += 31.0f;
		}
	}

	// ---- simulation screen ----
	void drawSimulation() {
		drawTopBar("Esc  menu");

		const char* crumb = TextFormat("Layer  %d / %d", getLayerIndex() + 1, getLayerCount());
		int cw = MeasureText(crumb, 22);
		DrawText(crumb, screenWidth / 2 - cw / 2, 28, 22, TEXT_PRIMARY);

		drawSynapses();
		drawNextLayerPreview();
		drawLayer();
		drawNodeInfo();
		drawSynapsePanel();
		drawFooterHints();
	}

	void drawFooterHints() {
		DrawRectangle(0, screenHeight - 50, screenWidth, 50, BG_PANEL);
		DrawRectangle(0, screenHeight - 50, screenWidth, 1, BORDER_COL);

		const char* hint = "1-4 move     5 / 6 layer     A B C D T H actions     Esc menu";
		int fs = 15;
		int tw = MeasureText(hint, fs);
		DrawText(hint, screenWidth / 2 - tw / 2, screenHeight - 32, fs, TEXT_MUTED);
	}

	// ---- movement ----
	void moveUp() {
		if (currentRow <= 0) return;
		int newRow = currentRow - 1;
		if (positionExists(newRow, currentCol)) currentRow = newRow;
	}
	void moveDown() {
		int newRow = currentRow + 1;
		if (positionExists(newRow, currentCol)) currentRow = newRow;
	}
	void moveLeft() {
		if (currentCol <= 0) return;
		int newCol = currentCol - 1;
		if (positionExists(currentRow, newCol)) currentCol = newCol;
	}
	void moveRight() {
		int newCol = currentCol + 1;
		if (positionExists(currentRow, newCol)) currentCol = newCol;
	}

	void snapToFirstAvailable() {
		currentRow = 0;
		currentCol = 0;
		for (int row = 0; row < mesh.getN(); row++) {
			for (int col = 0; col < mesh.getN(); col++) {
				if (positionExists(row, col)) {
					currentRow = row;
					currentCol = col;
					return;
				}
			}
		}
	}

	// keeps the selection valid after the mesh changes underneath us
	// (delete / prune / fuse / merge can remove the layer or neuron we were on)
	void validateSelection() {
		if (getCurrentLayer() == nullptr) {
			Layer* head = mesh.getHeadLayer();
			if (head == nullptr) return;
			currentLayer = head->layerId;
		}
		if (getCurrentNeuron() == nullptr) snapToFirstAvailable();
	}

	void startTransition(int direction) {
		prevLayerId = currentLayer;
		transitionDir = direction;
		layerTransition = 0.0f;
	}

	// uses the layer list's own prev/next links, so it still works when
	// layer ids are no longer contiguous (after a delete or merge)
	void previousLayer() {
		Layer* current = getCurrentLayer();
		if (current == nullptr || current->prev == nullptr) return;

		startTransition(-1);
		currentLayer = current->prev->layerId;
		currentRow = 0;
		currentCol = 0;

		if (getCurrentNeuron() == nullptr) snapToFirstAvailable();
	}

	void nextLayer() {
		Layer* current = getCurrentLayer();
		if (current == nullptr || current->next == nullptr) return;

		startTransition(+1);
		currentLayer = current->next->layerId;
		currentRow = 0;
		currentCol = 0;

		if (getCurrentNeuron() == nullptr) snapToFirstAvailable();
	}

	// ---- lookups ----
	Layer* getCurrentLayer() {
		return mesh.getLayerById(currentLayer);
	}
	Neuron* getCurrentNeuron() {
		return mesh.silentNeuronReturnByPosition(currentLayer, currentRow, currentCol);
	}
	Layer* getNextLayerPtr() {
		Layer* l = getCurrentLayer();
		return l ? l->next : nullptr;
	}

	int getLayerCount() {
		int count = 0;
		for (Layer* l = mesh.getHeadLayer(); l != nullptr; l = l->next) count++;
		return count;
	}
	int getLayerIndex() {
		int index = 0;
		for (Layer* l = mesh.getHeadLayer(); l != nullptr; l = l->next, index++) {
			if (l->layerId == currentLayer) return index;
		}
		return 0;
	}

	bool positionExists(int row, int col) {
		if (row < 0 || col < 0) return false;
		if (row >= mesh.getN() || col >= mesh.getN()) return false;

		Neuron* n = mesh.silentNeuronReturnByPosition(currentLayer, row, col);
		return n != nullptr;
	}

	// ---- geometry ----
	float slideOffset() {
		return transitionDir * (1.0f - easeOutCubic(layerTransition)) * 24.0f;
	}

	Vector2 mainNeuronPos(int row, int col) {
		return Vector2{ gridStartX + col * spacing, gridStartY + row * spacing + slideOffset() };
	}

	float previewX() {
		return gridStartX + (mesh.getN() - 1) * spacing + 130.0f;
	}
	float previewSpacing() { return spacing * 0.6f; }
	float previewRadius() { return nodeRadius * 0.55f; }

	Vector2 previewNeuronPos(int row, int col) {
		return Vector2{ previewX() + col * previewSpacing(), gridStartY + row * previewSpacing() };
	}

	// ---- main grid (cross-fades the old layer out while the new one slides in) ----
	double maxWeightIn(Layer* l) {
		double best = 0.0;
		for (Neuron* col = l->top_left; col; col = col->right)
			for (Neuron* n = col; n; n = n->down)
				if (n->weight > best) best = n->weight;
		return best;
	}

	void drawLayerGrid(Layer* l, float alpha, float yOffset, bool interactive) {
		if (l == nullptr || alpha <= 0.0f) return;

		double maxW = maxWeightIn(l);

		int c = 0;
		for (Neuron* col = l->top_left; col; col = col->right, c++) {
			int row = 0;
			for (Neuron* n = col; n; n = n->down, row++) {
				float x = gridStartX + c * spacing;
				float y = gridStartY + row * spacing + yOffset;

				bool selected = interactive && (row == currentRow && c == currentCol);

				float heat = (maxW > 0.0) ? (float)(n->weight / maxW) : 0.0f;
				Color fill = selected ? ACCENT_GOLD : lerpColor(BLUE_DEEP, BLUE_LIGHT, heat);

				if (selected) {
					DrawCircle((int)x, (int)y, nodeRadius + 15, Fade(ACCENT_GOLD, 0.07f * alpha));
					DrawCircle((int)x, (int)y, nodeRadius + 9, Fade(ACCENT_GOLD, 0.12f * alpha));
					DrawCircleLines((int)x, (int)y, nodeRadius + 6, Fade(ACCENT_GOLD, 0.7f * alpha));
				}

				DrawCircle((int)x, (int)y, nodeRadius, Fade(fill, alpha));

				const char* idText = TextFormat("%d", n->id);
				int fs = nodeRadius >= 14.0f ? 14 : 12;
				int textWidth = MeasureText(idText, fs);
				DrawText(idText, (int)x - textWidth / 2, (int)y - fs / 2, fs, Fade(Color{ 8, 12, 24, 255 }, alpha));
			}
		}
	}

	void drawLayer() {
		float eased = easeOutCubic(layerTransition);

		if (layerTransition < 1.0f && prevLayerId != currentLayer) {
			Layer* old = mesh.getLayerById(prevLayerId);
			drawLayerGrid(old, 1.0f - eased, -transitionDir * eased * 24.0f, false);
		}

		drawLayerGrid(getCurrentLayer(), eased, slideOffset(), true);
	}

	// ---- next-layer preview ----
	bool isTargetOfSelected(Neuron* candidate) {
		Neuron* sel = getCurrentNeuron();
		if (sel == nullptr) return false;
		for (Synapse* s = sel->head_axon; s != nullptr; s = s->next_synapse)
			if (s->target_neuron == candidate) return true;
		return false;
	}

	void drawNextLayerPreview() {
		Layer* nl = getNextLayerPtr();
		if (nl == nullptr) return;

		float eased = easeOutCubic(layerTransition);

		int c = 0;
		for (Neuron* col = nl->top_left; col; col = col->right, c++) {
			int row = 0;
			for (Neuron* n = col; n; n = n->down, row++) {
				Vector2 p = previewNeuronPos(row, c);
				bool hit = isTargetOfSelected(n);

				DrawCircleV(p, previewRadius() + (hit ? 2.0f : 0.0f),
					Fade(hit ? BLUE_PRIMARY : BG_PANEL_LIGHT, eased));
				DrawCircleLines((int)p.x, (int)p.y, previewRadius() + (hit ? 2.0f : 0.0f),
					Fade(hit ? BLUE_LIGHT : BORDER_COL, eased));

				if (hit) {
					const char* idText = TextFormat("%d", n->id);
					int tw = MeasureText(idText, 12);
					DrawText(idText, (int)p.x - tw / 2, (int)(p.y + previewRadius() + 6), 12, Fade(BLUE_LIGHT, eased));
				}
			}
		}

		DrawText("NEXT LAYER", (int)previewX() - 10, (int)gridStartY - 44, 13, Fade(TEXT_MUTED, eased));
	}

	// ---- synapses ----
	bool findPositionInLayer(Layer* l, Neuron* target, int& outRow, int& outCol) {
		if (l == nullptr || target == nullptr) return false;
		int c = 0;
		for (Neuron* col = l->top_left; col; col = col->right, c++) {
			int row = 0;
			for (Neuron* n = col; n; n = n->down, row++) {
				if (n == target) { outRow = row; outCol = c; return true; }
			}
		}
		return false;
	}

	// pass 0 draws every neuron's synapses faintly, pass 1 redraws the
	// selected neuron's on top - bright, thicker, labelled, with a pulse
	void drawSynapses() {
		Layer* l = getCurrentLayer();
		Layer* nl = getNextLayerPtr();
		if (l == nullptr || nl == nullptr) return;

		float eased = easeOutCubic(layerTransition);
		float t = (float)GetTime();
		Neuron* selectedNeuron = getCurrentNeuron();

		for (int pass = 0; pass < 2; pass++) {
			int c = 0;
			for (Neuron* col = l->top_left; col; col = col->right, c++) {
				int row = 0;
				for (Neuron* n = col; n; n = n->down, row++) {
					bool isSelected = (n == selectedNeuron);
					if ((pass == 0 && isSelected) || (pass == 1 && !isSelected)) continue;

					int index = 0;
					for (Synapse* s = n->head_axon; s != nullptr; s = s->next_synapse, index++) {
						int tr, tc;
						if (!findPositionInLayer(nl, s->target_neuron, tr, tc)) continue;

						Vector2 from = mainNeuronPos(row, c);
						Vector2 to = previewNeuronPos(tr, tc);

						if (!isSelected) {
							drawCurve(from, to, 1.0f, Fade(BLUE_DIM, eased));
							continue;
						}

						drawCurve(from, to, 2.4f, Fade(BLUE_BRIGHT, eased));

						float phase = t * 0.7f + index * 0.22f;
						float pulseT = phase - floorf(phase);
						Vector2 p = curvePoint(from, to, pulseT);
						DrawCircleV(p, 5.0f, Fade(BLUE_LIGHT, 0.25f * eased));
						DrawCircleV(p, 2.8f, Fade(Color{ 255, 255, 255, 255 }, eased));

						Vector2 mid = curvePoint(from, to, 0.5f);
						DrawText(TextFormat("%c %.1f", s->direction, s->weight),
							(int)mid.x - 16, (int)mid.y - 18, 13, Fade(BLUE_LIGHT, eased));
					}
				}
			}
		}
	}

	// ---- side panels ----
	void drawNodeInfo() {
		Rectangle panel = { 40.0f, 110.0f, 400.0f, 250.0f };
		DrawRectangleRounded(panel, 0.05f, 8, BG_PANEL);
		DrawRectangleLinesEx(panel, 1.0f, BORDER_COL);

		DrawText("CURRENT NEURON", 65, 132, 15, TEXT_MUTED);

		Neuron* current = getCurrentNeuron();
		if (current == nullptr) {
			DrawText("No neuron here", 65, 180, 22, TEXT_MUTED);
			return;
		}

		DrawText("Id", 65, 172, 20, TEXT_MUTED);
		DrawText(TextFormat("%d", current->id), 190, 170, 24, TEXT_PRIMARY);
		DrawText("Weight", 65, 208, 20, TEXT_MUTED);
		DrawText(TextFormat("%.2f", current->weight), 190, 206, 24, TEXT_PRIMARY);
		DrawText("Layer", 65, 244, 20, TEXT_MUTED);
		DrawText(TextFormat("%d", currentLayer), 190, 242, 24, TEXT_PRIMARY);
		DrawText("Row / Col", 65, 280, 20, TEXT_MUTED);
		DrawText(TextFormat("%d / %d", currentRow, currentCol), 190, 278, 24, TEXT_PRIMARY);
		DrawText("Synapses", 65, 316, 20, TEXT_MUTED);

		int count = 0;
		for (Synapse* s = current->head_axon; s != nullptr; s = s->next_synapse) count++;
		DrawText(TextFormat("%d", count), 190, 314, 24, TEXT_PRIMARY);
	}

	void drawSynapsePanel() {
		Rectangle panel = { 40.0f, 380.0f, 400.0f, 210.0f };
		DrawRectangleRounded(panel, 0.05f, 8, BG_PANEL);
		DrawRectangleLinesEx(panel, 1.0f, BORDER_COL);

		DrawText("OUTGOING SYNAPSES", 65, 402, 15, TEXT_MUTED);

		Neuron* current = getCurrentNeuron();
		if (current == nullptr || current->head_axon == nullptr) {
			DrawText(getNextLayerPtr() == nullptr ? "Last layer - no outgoing synapses" : "None", 65, 440, 18, TEXT_MUTED);
			return;
		}

		float y = 436.0f;
		int shown = 0;
		for (Synapse* s = current->head_axon; s != nullptr && shown < 4; s = s->next_synapse, shown++) {
			DrawText(TextFormat("%c", s->direction), 65, (int)y, 20, ACCENT_GOLD);
			DrawText(TextFormat("to neuron %d", s->target_neuron->id), 100, (int)y, 20, TEXT_PRIMARY);
			DrawText(TextFormat("w %.2f", s->weight), 320, (int)y, 20, BLUE_LIGHT);
			y += 36.0f;
		}
	}

};