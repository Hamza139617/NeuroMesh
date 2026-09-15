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
					for(Synapse* sy = nn->head_axon; sy != nullptr ; sy = sy->next_synapse) 
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


	Layer* mergeLayers(Layer* a, Layer* b) {
		// merging the two layers 
		// a will always be first one and b is always going to be second one

		if (a == nullptr || b == nullptr)
			return nullptr;

		Neuron* temp;

		for (Neuron* colt1 = a->top_left, *colt2 = b->top_left; colt1 && colt2; colt1 = colt1->right, colt2 = colt2->right) {

			for (Neuron* s1 = colt1, *s2 = colt2; s1 && s2; s1 = s1->down) {

				s1->weight = floor((s1->weight + s2->weight) / 2);
				
				clearAxons(s2);// only clear the axons of s2 because they are going to be deleted and no need 
				// for creting new connectings of this layer
				temp = s2;
				s2 = s2->down;
				delete temp;
				temp = nullptr;
				if(b->current_count > 0)
				b->current_count--;

			}


		}

		a->next = b->next;
		if (b->next)
			b->next->prev = a;

		delete b;
		b = nullptr ;

		if (a)
			rebuildLayerForwardSynapses(a);

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

		for (Layer* l = head_layer; l != nullptr; l = l->next) {

			Neuron* n;

			n = l->top_left;

			cout << "Layer : " << l->layerId << endl;

			while (n != nullptr) {

				Neuron* st = n;

				for (; n->right != nullptr; n = n->right) {
					cout << n->id << " ";
				}

				cout << n->id << endl;

				n = st->down;
			}


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
	char forwardPropagate(char inputLetter);

	void backwardPropagate(char targetLetter);

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


	void findNeuronPos(Layer* l,int& row, int& col , Neuron* n) {
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

public:
	RaylibGUI(NeuroMesh& mesh): mesh(mesh) {
		currentLayer = 0;
		currentRow = 0;
		currentCol = 0;

		running = true;

		screenWidth = 1200;
		screenHeight = 700;

		gridStartX = 500.0f;
		gridStartY = 160.0f;

		spacing = 100.0f;
		nodeRadius = 18.0f;
	}

	void run() {
		if (mesh.getHeadLayer() == nullptr)
			return;

		initialize();

		while (!WindowShouldClose() && running) {
			handleInput();

			BeginDrawing();

			ClearBackground(BLACK);

			draw();

			EndDrawing();
		}

		CloseWindow();
	}

private:

	void initialize() {

		InitWindow(screenWidth, screenHeight, "NeuroMesh");

		SetTargetFPS(60);
		
	}
	void handleInput() {
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
			//cout << "running";
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
			mesh.mergeNeurons(n->head_axon);
		}
		else if (IsKeyPressed(KEY_SEVEN)) {
			running = false;
		}




	}
	void draw() {
		drawTitle();
		drawLayer();
		drawNodeInfo();
		drawLayerInfo();

	}

	void moveUp() {
		if (currentRow <= 0) return;

		int newRow = currentRow - 1;

		if (positionExists(newRow, currentCol)) {
			currentRow = newRow;
		}
	}
	void moveDown() {
		int newRow = currentRow + 1;
		if (positionExists(newRow, currentCol)) {
			currentRow = newRow;
		}
	}
	void moveLeft() {
		if (currentCol <= 0) {
			return;
		}

		int newCol = currentCol - 1;

		if (positionExists(currentRow, newCol)) {
			currentCol = newCol;
		}
}
	void moveRight() {
		int newCol = currentCol + 1;

		if (positionExists(currentRow, newCol)) {
			currentCol = newCol;
		}
	}
	void previousLayer() {
		if (currentLayer <= 0)
			return;
		
		int newLayer = currentLayer - 1;
		Layer* layer = mesh.getLayerById(newLayer);

		if (layer == nullptr) return;
		currentLayer = newLayer;

		currentRow = 0;
		currentCol = 0;

		Neuron* n = getCurrentNeuron();

		if (n == nullptr) {
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
	}
	void nextLayer() {
		Layer* current = getCurrentLayer();

		if (current == nullptr) return;

		if (current->next == nullptr) return;

		currentLayer = current->next->layerId;

		currentRow = 0;
		currentCol = 0;

		Neuron* n = getCurrentNeuron();

		if (n == nullptr) {
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
	}

	Layer* getCurrentLayer() {
		return mesh.getLayerById(currentLayer);
	}
	Neuron* getCurrentNeuron() {
		return mesh.silentNeuronReturnByPosition(currentLayer, currentRow, currentCol);
	}

	int getLayerCount() {
		int count = 0;
		count = 0;
		
		for (Layer* l = mesh.getHeadLayer(); l != nullptr; l = l->next) {
			count++;
		}
		return count;
	}
	int getMaximumRow(Layer* layer) {
		if (layer == nullptr) return -1;

		int maxRow = -1;

		int row = 0;

		for (Neuron* col = layer->top_left; col != nullptr; col = col->right) {
			row = 0;

			for (Neuron* n = col; n; n = n->down) {
				if (row > maxRow) {
					maxRow = row;
				}
				row++;
			}
		}

		return maxRow;
	}
	int getMaximumColumn(Layer* layer) {
		if (layer == nullptr) return -1;

		int maxCol = -1;

		int col = 0;

		for (Neuron* c = layer->top_left; c; c = c->right) {
			maxCol = col;
			col++;
		}
		return maxCol;
	}

	void drawLayer() {
		Layer* l = getCurrentLayer();

		if (l == nullptr) return;

		int c = 0;

		for (Neuron* col = l->top_left; col; col = col->right, c++) {
			int row = 0;

			for (Neuron* n = col; n; n = n->down, row++) {
				float x = gridStartX + c * spacing;
				float y = gridStartY + row * spacing;

				bool selected = (row == currentRow && c == currentCol);

				if (selected) {
					DrawCircle((int)x, (int)y, nodeRadius, LIGHTGRAY);
				}
				else {
					DrawCircle((int)x, (int)y, nodeRadius, WHITE);
				}

				const char* idText = TextFormat("%d", n->id);
				int textWidth = MeasureText(idText, 14);

				DrawText(idText, (int)x - textWidth / 2, (int)y - 7, 14, selected ? WHITE : BLACK);
				

			}

		

			
		}
	}
	void drawNodeInfo() {
		Neuron* current = getCurrentNeuron();

		DrawText("current node", 50, 150, 26, WHITE);

		if (current == nullptr) {
			DrawText("No node", 50, 200, 20, WHITE);

			return;
		}

		DrawText(TextFormat("Id: %d", current->id), 50, 205, 22, WHITE);
		DrawText(TextFormat("WEIGHT: %.2f", current->weight), 50, 245, 22, WHITE);
		DrawText(TextFormat("Layer: %d", currentLayer), 50, 285, 22, WHITE);
		DrawText(TextFormat("Row: %d", currentRow), 50, 325, 22, WHITE);
		DrawText(TextFormat("Col: %d", currentCol), 50, 365, 22, WHITE);

	}
	// void drawNavigationInfo(); work on this if the deadline got extended
	void drawLayerInfo() {

		


	}


	void drawTitle() {
		DrawText("NeuroMesh", 40, 35, 30, WHITE);
	}

	bool positionExists(int row, int col) {
		if (row < 0 || col < 0) return false;

		if (row >= mesh.getN() || col >= mesh.getN()) return false;

		Neuron* n = mesh.silentNeuronReturnByPosition(currentLayer, row, col);

		return n != nullptr;
	}

};

