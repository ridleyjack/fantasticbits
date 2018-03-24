#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <math.h>

# define PI           3.14159265358979323846
using namespace std;

constexpr int boardWidth = 16001;
constexpr int boardHeight = 7501;

class Point;
class Entity;
class Bludger;
class Snaffle;
class Wizard;

//****************************************
class Point {
public:
	double x;
	double y;

	Point() {};

	Point(double x, double y) {
		this->x = x;
		this->y = y;
	}
};

inline double calcDist(int x1, int y1, int x2, int y2) {
	return (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1);
}

inline double calcDistSqr(int x1, int y1, int x2, int y2) {
	return sqrt(calcDist(x1, y1, x2, y2));
}

inline double calcDist(Point* p1, Point* p2) {
	return (p2->x - p1->x)*(p2->x - p1->x) + (p2->y - p1->y)*(p2->y - p1->y);
}

inline double calcDistSqr(Point* p1, Point* p2) {
	return sqrt(calcDist(p1, p2));
}
inline bool inMiddle_X(Point* lower, Point* mid, Point* upper) {
	return (lower->x > mid->x && mid->x > upper->x) || (lower->x < mid->x && mid->x < upper->x);
}

inline int findIndxOfLowestDist(double* arr, int len) {
	double small = -1;
	int id = 0;
	for (int i = 0; i < len; i++) {
		if (arr[i] > 0 && arr[i] < small || small == -1) {
			small = arr[i];
			id = i;
		}
	}
	return id;
}


//****************************************
class Entity : public Point
{
public:
	int id;
	string type;
	int state;
	bool dead;

	double mass;
	double rad;
	double frict;

	double vx;
	double vy;

	Entity() {
		type = "";
	}

	Entity(const Entity &ent) {
		this->id = ent.id;
		this->x = ent.x;
		this->y = ent.y;
		this->type = ent.type;
		this->state = ent.state;
		this->vx = ent.vx;
		this->vy = ent.vy;
		this->mass = ent.mass;
		this->rad = ent.rad;
		this->frict = ent.frict;
	}

	virtual void update(int id, double x, double y, double vx, double vy, string type, int state){
		this->x = x;
		this->y = y;
		this->id = id;
		this->type = type;
		this->state = state;
		this->vx = vx;
		this->vy = vy;
	}

	virtual double speedTo(Point* p) {
		Point dist;
		dist.x = x - p->x;
		dist.y = y - p->y;
		double dotProduct = dist.x*vx + dist.y*vy;
		return  dotProduct / sqrt(dist.x*dist.x + dist.y*dist.y);
	}

	virtual inline void thrust(Point* dest, double thrust){
		thrust = thrust / this->mass;
		double adj = dest->x - x;
		double opp = dest->y - y;
		double theta = atan(opp / adj);
		double deltvx = cos(theta)*thrust;
		double deltvy = sin(theta)*thrust;

		if (adj < 0 && deltvx > 0) deltvx = deltvx * -1;
		if (adj > 0 && deltvx < 0) deltvx = deltvx * -1;

		if (opp < 0 && deltvy > 0) deltvy = deltvy * -1;
		if (opp > 0 && deltvy < 0) deltvy = deltvy * -1;
		vx += deltvx;
		vy += deltvy;
	}

	virtual inline void move() {
		x += vx;
		y += vy;
	}

};

class Bludger : public Entity {
public:
	Bludger() {
		this->mass = 1;
		this->rad = 200;
		this->frict = 0.9;
	}
};

class Snaffle : public Entity {
public:
	Wizard * carriedBy;
	int petCooldown;

	Snaffle() {
		this->mass = 0.5;
		this->rad = 150;
		this->frict = 0.75;
		this->petCooldown = 0;
	}

	Snaffle(const Snaffle &snaf)
		:Entity(snaf)
	{
	}

	void update(int id, double x, double y, double vx, double vy, string type, int state) {
		Entity::update(id, x, y, vx, vy, type, state);
		if (petCooldown > 0) petCooldown--;
	}
};

class Wizard : public Entity {
public:
	int targetId;
	int flipCooldown;

	Wizard() {
		this->mass = 1;
		this->rad = 200;
		this->frict = 0.75;
		this->flipCooldown = 0;
	}

	Wizard(const Wizard &wiz)
		:Entity(wiz)
	{
	}

	void update(int id, double x, double y, double vx, double vy, string type, int state) {
		Entity::update(id, x, y, vx, vy, type, state);
		if (flipCooldown > 0) flipCooldown--;
	}
};

class GoalPost : public Entity {
	GoalPost(int x, int y) {
		this->rad = 300;
		this->x = x;
		this->y = y;
	}
};

inline int getClosestId(Entity* e, Snaffle* targets[], int len) {
	if (len <= 0) return -1;
	int smallestId = -1;
	double smallest = -1;
	for (int i = 0; i < len; i++) {
		if (targets[i] == NULL || targets[i]->dead) continue;
		double dist = calcDist(e, targets[i]);
		if (smallestId == -1 || dist < smallest) {
			smallest = dist;
			smallestId = i;
		}
	}
	return smallestId;
}

class WizardAction {
public:
	string action;
	Entity * target;

	WizardAction() {
		action = "";
	}
};

struct distance_id {
	int id;
	double dist;
};

inline void sortByDistance(Entity* e, Entity* ents[], int len, Entity* out[], int* out_len) {
	int count = 0;
	for (int i = 0; i < len; i++)
		if (ents[i] != NULL && ents[i]->dead == false) count++;
	
	double** dist = new double*[count];
	for (int i = 0; i < count; i++) {
		dist[i] = new double[2];
		dist[i][0] = 0;
		dist[i][1] = 0;
	}
	int num = 0;
	for (int i = 0; i < len; i++) {
		if (ents[i] == NULL || ents[i]->dead) continue;
		dist[num][0] = i;
		dist[num][1] = calcDistSqr(e, ents[i]);
		num++;
	}

	bool sorted = false;

	while (sorted == false) {
		sorted = true;
		for (int i = 1; i < count; i++) {
			if (dist[i - 1][1] > dist[i][1]) {
				sorted = false;
				double* temp = dist[i - 1];

				dist[i - 1] = dist[i];
				
				dist[i] = temp;
			}
		}
	}

	*out_len = count;
	for (int i = 0; i < count; i++) {
			out[i] = ents[ (int)(dist[i][0]) ];
	}

	for (int i = 0; i < count; i++) {
		delete[] dist[i];
	}
	delete[] dist;
}

enum ENT_TYPE {
	TYPE_GOOD_WIZ,
	TYPE_BAD_WIZ,
	TYPE_BLDG,
	TYPE_SNAF
};

class GameTurn {
public:
	static int myTeamId;
	static int num_entity_t;
	static int num_snaf_t;
	static int num_bludger;

	int entities;
	int myScore;
	int myMagic;
	int opponentScore;
	int opponentMagic;
	
	int num_snaf;

	Point* center;
	Point* myGoal;
	Point* badGoal;

	Entity** entity;
	//Wizard** wizards;
	Wizard** myWizard;
	Wizard** badWizard;
	Snaffle** snaffle;
	Bludger** bludger;

	GameTurn(bool firstTurn = false) {

		 // if 0 you need to score on the right of the map, if 1 you need to score on the left
		int myTeamId = 0;
		cin >> myTeamId; cin.ignore();

		cin >> myScore >> myMagic; cin.ignore();
		cin >> opponentScore >> opponentMagic; cin.ignore();		
		cin >> entities; cin.ignore();

		if (firstTurn) {
			this->myTeamId = myTeamId;
			this->num_entity_t = entities;

			//create entities array
			entity = new Entity*[entities];
			
			//create the wizards
			myWizard = new Wizard*[2];
			badWizard = new Wizard*[2];
			for (int i = 0; i < 2; i++) {
				myWizard[i] = new Wizard();
				badWizard[i] = new Wizard();
			}

			center = new Point(8000, 3750);

			//assign wizards to teams
			if (myTeamId == 0) {
				entity[0] = myWizard[0];
				entity[1] = myWizard[1];
				entity[2] = badWizard[0];
				entity[3] = badWizard[1];

				myGoal = new Point(0, 3750);
				badGoal = new Point(16000, 3750);
			}
			else {
				entity[2] = myWizard[0];
				entity[3] = myWizard[1];
				entity[0] = badWizard[0];
				entity[1] = badWizard[1];

				myGoal = new Point(16000, 3750);
				badGoal = new Point(0, 3750);
			}
		}
		else { //if not first turn
			for (int i = 0; i < num_snaf_t; i++) {
				snaffle[i]->dead = true;
			}
		}
		
		num_snaf = 0;
		for (int i = 0; i < entities; i++)
		{
			int entityId; // entity identifier
			string entityType; // "WIZARD", "OPPONENT_WIZARD" or "SNAFFLE" (or "BLUDGER" after first league)
			int x; // position
			int y; // position
			int vx; // velocity
			int vy; // velocity
			int state; // 1 if the wizard is holding a Snaffle, 0 otherwise
			cin >> entityId >> entityType >> x >> y >> vx >> vy >> state; cin.ignore();

			if (entityType == "WIZARD" || entityType == "OPPONENT_WIZARD") {
				//no special action
			}
			else if (entityType == "SNAFFLE") {
				if (firstTurn) entity[entityId] = new Snaffle();
				entity[entityId]->dead = false;
				num_snaf++;
			}
			else if (entityType == "BLUDGER") {
				if (firstTurn) entity[entityId] = new Bludger();
			}

			entity[entityId]->update(entityId, x, y, vx, vy, entityType, state);
		}//end of read all entitites

		if (firstTurn) {
			num_snaf_t = num_snaf;
			snaffle = new Snaffle*[num_snaf_t];		
			for (int i = 4; i < num_snaf_t + 4; i++)
				snaffle[i] = (Snaffle*)entity[i];

			num_bludger = num_entity_t - 4 - num_snaf_t;
			for (int i = 4 + num_snaf_t; i < 4 + num_snaf_t + num_bludger; i++)
				bludger[i] = (Bludger*)entity[i];
		}

	} // end GameTurn(..)

};

int main()
{
	bool isFirstTurn = true;
 
	int lastAccio = 0;
    // game loop
	while (1) {

		GameTurn turn(isFirstTurn);
		if (isFirstTurn) isFirstTurn = false;


		//adjust snaffles positions being carried by bad wizards;
		for (int i = 0; i < 7; i++) {
			Snaffle* snaf = snaffle[i];
			if (snaf == NULL || snaf->dead || snaf->state == 0) continue;
			for (int j = 0; j < 2; j++) {
				Wizard* wiz = badWizard[j];
				if (snaf->x == wiz->x && snaf->y == wiz->y) {
					snaf->carriedBy = wiz;
					snaf->thrust(myGoal, 500);
				}
			}
		}

		WizardAction wizAction[2];
		if (myWizard[0]->state == 1) wizAction[0].action = "THROW";
		if (myWizard[1]->state == 1) wizAction[1].action = "THROW";

		//************** find wizard targets
		int snafMySide = 0;
		int snafBadSide = 0;

		double wiz0SnafDist[num_snaffle];
		double wiz1SnafDist[num_snaffle];
		for (int j = 0; j < num_snaffle; j++) {
			Snaffle* snaf = snaffle[j];
			
			if (snaf == NULL || snaf->dead) {
				wiz0SnafDist[j] = -1;
				wiz1SnafDist[j] = -1;
			}
			else {
				if (inMiddle_X(myGoal, snaf, &center)) snafMySide++;
				else snafBadSide++;
				wiz0SnafDist[j] = calcDistSqr(myWizard[0], snaf);
				wiz1SnafDist[j] = calcDistSqr(myWizard[1], snaf);
			}
		}
		myWizard[0]->targetId = findIndxOfLowestDist(wiz0SnafDist, num_snaffle);
		myWizard[1]->targetId = findIndxOfLowestDist(wiz1SnafDist, num_snaffle);

		//if both wizards have the same target, nothing to throw and there is more than one snaffle left
		if (myWizard[0]->targetId == myWizard[1]->targetId && myWizard[0]->state == 0 && myWizard[1]->state == 0 && snaffleCount > 1) {
			wiz0SnafDist[myWizard[0]->targetId] = -1;
			wiz1SnafDist[myWizard[1]->targetId] = -1;
			int secId0 = findIndxOfLowestDist(wiz0SnafDist, num_snaffle);
			int secId1 = findIndxOfLowestDist(wiz1SnafDist, num_snaffle);

			if (wiz0SnafDist[myWizard[0]->targetId] + wiz1SnafDist[secId1] > wiz1SnafDist[myWizard[1]->targetId] + wiz0SnafDist[secId0]) {
				myWizard[0]->targetId = secId0;
			}
			else {
				myWizard[1]->targetId = secId1;
			}
		}

		myWizard[0]->targetId += 4;
		myWizard[1]->targetId += 4;

		/*
		if (snafMySide + opponentScore > snafBadSide + myScore) {
			double best = 0;
			int id = -1;
			Wizard* wiz;
			int otherWizId = -1;

			if (calcDist(myWizard[0], snaffle[myWizard[0]->targetId]) < calcDist(myWizard[1], snaffle[myWizard[1]->targetId])) {
				wiz = myWizard[0];
				otherWizId = 1;
			}
			else {
				wiz = myWizard[1];
				otherWizId = 0;
			}

			for (int j = 0; j < num_snaffle; j++) {
				Snaffle* snaf = snaffle[j];
				if (snaf == NULL || snaf->dead) continue;
				if (myTeamId = 0 && snaf->x > center.x) continue;
				if (myTeamId = 1 && snaf->x < center.x) continue;
				if (snaf->id == myWizard[otherWizId]->targetId) continue;
				double d = calcDist(snaf, wiz);
				if (id == -1 || d < best) {
				best = d;
				id = j;
				}
			}
			if(id != -1) wiz->targetId = id;
		}
		*/


		//********************** Flipendo
		for (int i = 0; i < 2; i++) {
			if (myMagic > 20 && wizAction[i].action != "") {
				Wizard* wiz = myWizard[i];
				Entity* targs[7];
				int targs_len = 0;
				sortByDistance(wiz, (Entity**)snaffle, 7, targs, &targs_len);
				for (int j = 0; j < targs_len; j++) {					
					Entity* snaf = targs[j];

					double rise = (snaf->y - wiz->y);
					double run = (snaf->x - wiz->x);
					if (run == 0) continue;
					// y = mx + b
					double slope = rise / run;
					double b = wiz->y - slope * wiz->x;

					//look for rebounds 										
					//x = (y - b) / m  | y = 0 bottom wall | y = height top wall
					if   (  (0 - b) / slope > 0 && (0 - b) / slope < boardWidth)  {
						double x = b / slope;						
						slope = - slope;
						b = slope * x;
					}						
					else if ( (boardHeight - b) / slope > 0 &&  (boardHeight - b) / slope <  boardWidth ) {
						double x = (boardHeight - b) / slope;
						slope = -slope;
						b = slope * x;
					}
					//else b and slope stays the same

					double x = badGoal->x;
					double y = slope * x + b;
					cerr << "wiz:" << i << " y:" << "snaf:" << snaf->id << y << endl;
					double distFromGoal = abs(badGoal->x - snaf->x);
					if (y < badGoal->y + 1800 && y > badGoal->y - 1800 && inMiddle_X(badGoal, snaf, myWizard[i]) && myWizard[i]->flipCooldown == 0) {
						wizAction[i].action = "FLIPENDO";
						wizAction[i].target = snaf;
						myMagic -= 20;
						break;
					}
				}
			}
		}
	
		//********************** ACCIO Maybe
		if (lastAccio == 0 && myMagic > 20) {
			Entity* good_closest[2];
			double good_dist[2];

			//closest snaffles to badguys
			Entity* bad_closest[2];
			bad_closest[0] = snaffle[getClosestId(badWizard[0], snaffle, 7)];
			bad_closest[1] = snaffle[getClosestId(badWizard[1], snaffle, 7)];

			for (int i = 0; i < 2; i++) {				
				double dist[2];
				dist[0] = calcDist(myWizard[i], bad_closest[0]);
				dist[1] = calcDist(myWizard[i], bad_closest[1]);
				
				if (dist[0] < dist[1]) {
					good_closest[i] = bad_closest[0];
					good_dist[i] = dist[0];
				}
				else {
					good_closest[i] = bad_closest[1];
					good_dist[i] = dist[1];
				}
			}

			if ( myWizard[0]->state == 0 && wizAction[0].action == "" && (good_dist[0] < good_dist[1] || myWizard[1]->state==1) 
				&& inMiddle_X(myGoal, good_closest[0], myWizard[0]) ) {
				
				wizAction[0].action = "ACCIO";
				wizAction[0].target = good_closest[0];
				lastAccio = 2;
			}
			else if (myWizard[1]->state == 0 && wizAction[1].action == "" && inMiddle_X(myGoal, good_closest[1], myWizard[1]) ) {

				wizAction[1].action = "ACCIO";
				wizAction[1].target = good_closest[1];
				lastAccio = 2;
			}
			//else nobody is in a good position to accio

		}

		//********************* Petrificus
		Snaffle* dangerSnaf = NULL;
		for (int i = 0; i < 7; i++) {
			Snaffle* snaf = snaffle[i];
			if (snaf == NULL || snaf->dead || snaf->state == 1 || snaf->petCooldown > 0) continue;

			Point futurePos(snaf->x, snaf->y);
			double vy = snaf->vy;
			double vx = snaf->vx;
			while (vx >= 1 || vx <= -1) {
				futurePos.x += vx;
				futurePos.y += vy;
				vx *= snaf->frict;
				vy *= snaf->frict;
				if (futurePos.x < 0 || futurePos.x > 16001) break;
			}
			if (snaf->id == 4) cerr << "x:" << futurePos.x << " y:" << futurePos.y << " vx:" << snaf->vx << endl;

			if (futurePos.y > myGoal->y - 1700 && futurePos.y < myGoal->y + 1700 && inMiddle_X(snaf, myGoal, &futurePos)) {
				dangerSnaf = snaf;
				break;
			}
		}

		/*
		if (myMagic >= 10 && dangerSnaf != NULL && snaffleCount <= 3 && dangerSnaf->petCooldown == 0) {
			if (myWizard[0]->state == 0 && calcDist(myWizard[0], dangerSnaf) < calcDist(myWizard[1], dangerSnaf)) {
				wizAction[0].action = "PETRIFICUS";
				wizAction[0].target = dangerSnaf;
				dangerSnaf->petCooldown = 4;
				myMagic -= 10;
			}
			else {
				wizAction[1].action = "PETRIFICUS";
				wizAction[1].target = dangerSnaf;
				dangerSnaf->petCooldown = 4;
				myMagic -= 10;
			}
		}
		*/
		//********Try to guess where bad wizard is going
		Wizard fakeBad[] = { *badWizard[0], *badWizard[1] };
		Snaffle fakeSnaf[7];
		for (int j = 0; j < 7; j++) {
			if (snaffle[j] == NULL) continue;
			fakeSnaf[j] = *snaffle[j];
			//fakeSnaf[j].move();
		}
		for (int j = 0; j < 2; j++) {
			double distBest = -1; int id = -1;
			for (int k = 0; k < 7; k++) {
				if (fakeSnaf[k].type == "" || fakeSnaf[k].dead) continue;
				double dist = calcDist(&fakeSnaf[k], &fakeBad[j]);
				if (distBest == -1 || dist < distBest) {
					distBest = dist;
					id = k;
				}
			}
			cerr << "**** Baddie Id:" << badWizard[j]->id << endl;
			cerr << "target:" << fakeSnaf[id].x << " " << fakeSnaf[id].y << endl;

			fakeBad[j].thrust(&fakeSnaf[id], 150);
			fakeBad[j].move();
			cerr << "baddie:" << j << " EstDest x:" << fakeBad[j].x << " y:" << fakeBad[j].y << " moved to" << id + 4 << endl;
			//cerr << "baddieOld:" << j << " EstDest x:" << badWizard[j]->x << " y:" << badWizard[j]->y << endl;
		}

		//****************************** find moves
        for (int i = 0; i < 2; i++) {
			Wizard* wiz = myWizard[i];

			string action = "";
			Point dest(0,0);
			int thrust = 150;

			if (wiz->state == 0 && wizAction[i].action == "") {
				action = "MOVE";
				dest.x = entity[wiz->targetId]->x;
				dest.y = entity[wiz->targetId]->y;

				dest.x += entity[wiz->targetId]->vx;
				dest.y += entity[wiz->targetId]->vy;

				wiz->thrust(&dest, 150);
				wiz->move();
				if (calcDist(wiz, &dest) < calcDist(wiz, &dest)) {
					thrust = 80;
				}

			}
			else {
				action = "THROW";
				dest.x = badGoal->x;
				dest.y = badGoal->y;

				dest.x - wiz->vx;
				dest.y - wiz->vy;
				thrust = 500;
				Point* estDest = new Point();
				
				//try to guess collisions.
				Snaffle ghostSnaf;
				ghostSnaf.update(-1, wiz->x, wiz->y, wiz->vx, wiz->vy, "ghostSnaf", 1);
				ghostSnaf.thrust(&dest, 500);
				ghostSnaf.move();

				Wizard futureWiz = *myWizard[i];
				futureWiz.move();
				double futureSnafDist = calcDistSqr(&futureWiz, &ghostSnaf);

				cerr << "snafDest" << ghostSnaf.x << " " << ghostSnaf.y << endl;
				for (int j = 0; j < 2; j++) {
					double d = calcDistSqr(&fakeBad[j], &ghostSnaf);					
					if (d < futureSnafDist) {
						if (fakeBad[j].y < wiz->y) {
							cerr << "adjusting throw" << endl;
							dest.y = badGoal->y + 1600;
							break;
						}
						else {
							cerr << "adjusting throw" << endl;
							dest.y = badGoal->y - 1600;
							break;
						}
					}
				}


				/*
				if (calcDistSqr( badWizard[0], &ghostSnaf) < (wiz->rad+ghostSnaf.rad ) || calcDistSqr( badWizard[1], &ghostSnaf) < (wiz->rad + ghostSnaf.rad) ) {
					cerr << "adjusting throw" << endl;
					//dest.x = 0;
					dest.y = wiz->x > 3750 ? 0 : 7500;
				}
				*/
				delete(estDest);
			}
            // Edit this line to indicate the action for each wizard (0 ≤ thrust ≤ 150, 0 ≤ power ≤ 500)
            // i.e.: "MOVE x y thrust" or "THROW x y power"
			if (wizAction[i].action == "ACCIO" || wizAction[i].action == "FLIPENDO" || wizAction[i].action == "PETRIFICUS") cout << wizAction[i].action << " " << wizAction[i].target->id << endl;
            else cout << action << " " << rint(dest.x) << " " << rint(dest.y) << " " << thrust << endl;
        }
    }
}