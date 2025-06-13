// DESIGN PRINCIPLE: StudentWorld handles/Signals to Actor classes that they are SUPPOSED to interact with one another.

#include "StudentWorld.h"
#include <string>
using namespace std;
//---****---
#include <vector>
#include "Actor.h"

GameWorld* createStudentWorld(string assetDir)
{
	return new StudentWorld(assetDir);
}

StudentWorld::~StudentWorld()
{
	// Delete ice2DArray
	for (int col = 0; col < ICE_ARRAY_SIZE; col++) {
		for (int row = 0; row < ICE_ARRAY_SIZE; row++) {
			delete ice2DArray[col][row];
		}
	}
	// Delete all Actors
	for (int i = 0; i < actors.size(); i++)
		delete actors[i];
	// Delete Player/Iceman
	delete iceman;
}

int StudentWorld::init() {

	// Initialize ice
	for (int col = 0; col < ICE_ARRAY_SIZE; col++) {
		for (int row = 0; row < ICE_ARRAY_SIZE; row++) {
			ice2DArray[col][row] = new Ice(col, row, this);
			ice2DArray[col][row]->setVisible(true);
		}
	}
	// Create Tunnel by deactivating Ice
	for (int col = 30; col <= 33; col++) {
		for (int row = 4; row <= 59; row++) {
			delete ice2DArray[col][row];
			ice2DArray[col][row] = nullptr;
		}
	}

	// Set Environment amounts:
	int currentLevel = int(getLevel());
	numBoulders = min(currentLevel / 2 + 2, 9);
	numGoldNuggets = max(5 - currentLevel / 2, 2);
	numBarrelsOfOil = min(2 + currentLevel, 21);

	// Distribute Environment Actors
	distributeOilFieldContents(actors);
	iceman = new Iceman(30, 60, this);
	iceman->setVisible(true);
	m_ticksSinceLastProtester = 0;

	return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
	updateDisplayText();
	computeExitMap();
	iceman->update();
	mineIce();
	handleCollisions(actors);
	updateAllActors();


	int level = getLevel();
	int T = std::max(25, 200 - level);
	int P = std::min(15, 2 + static_cast<int>(level * 1.5));
	int numProtesters = 0;
	for (Actor* a : actors)
	{
		if (a && dynamic_cast<Protester*>(a) && a->isActive())
		{
			numProtesters++;
		}
	}

	bool firstProtester = (numProtesters == 0);
	if ((firstProtester || m_ticksSinceLastProtester >= T) && numProtesters < P)
	{
		int probabilityOfHardcore = std::min(90, level * 10 + 30);
		Actor* protester = nullptr;

		if (rand() % 100 < probabilityOfHardcore)
		{
			protester = new HardcoreProtester(60, 60, this);
		}
		else
		{
			protester = new RegularProtester(60, 60, this);
		}

		if (protester != nullptr)
		{
			protester->setVisible(true);
			actors.push_back(protester);
		}

		m_ticksSinceLastProtester = 0;
	}
	else
	{
		m_ticksSinceLastProtester++;
	}

	spawnRandomGoodies();
	removeDeadGameObjects();

	return determineGameStatus();
}

void StudentWorld::cleanUp()
{
	// Clean-up Ice Array
	for (int col = 0; col < ICE_ARRAY_SIZE; col++)
		for (int row = 0; row < ICE_ARRAY_SIZE; row++)
			delete ice2DArray[col][row];

	// Clean-up Actors
	for (int i = 0; i < actors.size(); i++) {
		delete actors[i];
		actors[i] = nullptr;
	}
	actors.clear();

	delete iceman;
}

// Private Functions:

// init() helpers:
void StudentWorld::distributeOilFieldContents(std::vector<Actor*>& actors)
{
	distributeBoulders(actors);
	distributeGoldNuggets(actors);
	distributeBarrelsOfOil(actors);
}

void StudentWorld::distributeBoulders(vector<Actor*>& actors)
{
	int boulderSpawnX = 0;
	int boulderSpawnY = 0;
	for (int i = 0; i < numBoulders; i++) {
		do {
			boulderSpawnX = static_cast<int>(rand() % (61 - 4) + 0);
			boulderSpawnY = static_cast<int>(rand() % 37 + 20);
		} while (isSpawnTooCloseToOtherActors(boulderSpawnX, boulderSpawnY, 1.0, actors) || isInTunnelSpawn(boulderSpawnX, boulderSpawnY, 1.0));

		// Clear 4x4 Ice Around Boulders
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				delete ice2DArray[boulderSpawnX + j][boulderSpawnY + k];
				ice2DArray[boulderSpawnX + j][boulderSpawnY + k] = nullptr;
			}
		}
		Actor* temp = new Boulder(boulderSpawnX, boulderSpawnY, this);
		temp->setVisible(true);
		actors.push_back(temp);
	}

}

void StudentWorld::distributeGoldNuggets(std::vector<Actor*>& actors)
{
	int goldNuggetSpawnX = 0;
	int goldNuggetSpawnY = 0;

	for (int i = 0; i < numGoldNuggets; i++) {
		do {
			goldNuggetSpawnX = static_cast<int>(rand() % (61 - 4) + 0);
			goldNuggetSpawnY = static_cast<int>(rand() % 56 + 0);
		} while (isSpawnTooCloseToOtherActors(goldNuggetSpawnX, goldNuggetSpawnY, 1.0, actors) || isInTunnelSpawn(goldNuggetSpawnX, goldNuggetSpawnY, 1.0));

		actors.push_back(new GoldNugget(goldNuggetSpawnX, goldNuggetSpawnY, this));
	}
}

void StudentWorld::distributeBarrelsOfOil(std::vector<Actor*>& actors)
{
	int barrelOfOilSpawnX = 0;
	int barrelOfOilSpawnY = 0;

	for (int i = 0; i < numBarrelsOfOil; i++) {
		do {
			barrelOfOilSpawnX = static_cast<int>(rand() % (61 - 4) + 0);
			barrelOfOilSpawnY = static_cast<int>(rand() % 56 + 0);
		} while (isSpawnTooCloseToOtherActors(barrelOfOilSpawnX, barrelOfOilSpawnY, 1.0, actors) || isInTunnelSpawn(barrelOfOilSpawnX, barrelOfOilSpawnY, 1.0));

		actors.push_back(new BarrelOfOil(barrelOfOilSpawnX, barrelOfOilSpawnY, this));
	}
}

bool StudentWorld::isSpawnTooCloseToOtherActors(int x, int y, double size, const std::vector<Actor*>& actors) {
	for (int i = 0; i < actors.size(); i++) {
		if (actors[i] == nullptr)
			continue;

		double distance = calculateDistance(x, y, size, actors[i]);
		if (distance < 6)
			return true;
	}

	return false;
}

bool StudentWorld::isEmptySpace(int x, int y, double size) {
	int hitBoxSize = static_cast<int>(size * 4);
	// Counts space ABOVE icefield as empty space:
	// x: [0, 60)
	// y: [60, 64)
	if (y >= 60 && y < 64 && x >= 0 && x < 60)
		return true;

	// Out of bounds for ice2DArray
	// if (x < 0|| y < 0 || x + hitBoxSize > ICE_ARRAY_SIZE || y + hitBoxSize > ICE_ARRAY_SIZE)
		// return false; 

	int validXCheckPixels = hitBoxSize;
	int validYCheckPixels = hitBoxSize;

	if (x < 0 || x + hitBoxSize > ICE_ARRAY_SIZE)
		validXCheckPixels = x + hitBoxSize - ICE_ARRAY_SIZE;

	if (y < 0 || y + hitBoxSize > ICE_ARRAY_SIZE)
		validYCheckPixels = y + hitBoxSize - ICE_ARRAY_SIZE;


	for (int i = 0; i < validXCheckPixels; i++) {
		for (int j = 0; j < validYCheckPixels; j++) {
			if (ice2DArray[x + i][y + j] != nullptr)
				return false;
		}
	}

	return true;
}


bool StudentWorld::isInTunnelSpawn(int x, int y, double size) {
	int hitBoxSize = static_cast<int>(size * 4);
	for (int i = 0; i < hitBoxSize; i++) {
		for (int j = 0; j < hitBoxSize; j++) {
			int currX = x + i;
			int currY = y + j;
			if (currX >= 30 && currX <= 33 && currY >= 4 && currY <= 59)
				return true;
		}
	}
	return false;
}

bool StudentWorld::isEmptySpace(Actor* a) {
	return isEmptySpace(a->getX(), a->getY(), a->getSize());
}

bool StudentWorld::checkCoordsAreValid(int x, int y, int size) const {
	if (x < 0 || y < 0 || x > ICE_ARRAY_SIZE || y > ICE_ARRAY_SIZE + size)
		return false;

	for (int i = 0; i < actors.size(); i++)
		if (actors[i] != nullptr
			&& actors[i]->isClippable()
			&& willActorsCollide(x, y, size, actors[i]))
			return false;

	return true;
}
void StudentWorld::updateDisplayText() {
	string msg = "";
	msg += "Lvl: " + std::to_string(getLevel());
	msg += " Lives : " + std::to_string(getLives());
	msg += " Hlth : " + std::to_string(iceman->getHitPoints() * 10) + "%";
	msg += " Wtr: " + std::to_string(iceman->getNumWaterSquirts());
	msg += " Gld: " + std::to_string(iceman->getNumGoldNuggets());
	msg += " Oil Left : " + std::to_string(getNumBarrlesOfOil());
	msg += " Sonar: " + std::to_string(iceman->getNumSonarCharges());
	msg += " Scr: " + std::to_string(getScore());
	setGameStatText(msg);

}

// move() helpers:
void StudentWorld::removeDeadGameObjects() {
	// Delete Ice objects from ice2DArray
	for (int i = 0; i < ICE_ARRAY_SIZE; i++)
		for (int j = 0; j < ICE_ARRAY_SIZE; j++)
			if (ice2DArray[i][j] != nullptr && !ice2DArray[i][j]->isActive()) {
				delete ice2DArray[i][j];
				ice2DArray[i][j] = nullptr;
			}

	// Delete all other inactive Actors
	for (int i = 0; i < actors.size(); i++)
		if (actors[i] != nullptr && !actors[i]->isActive()) {
			delete actors[i];
			actors[i] = nullptr;
		}
}

void StudentWorld::spawnRandomGoodies() {
	int G1 = getLevel() * 30 + 290;
	if (rand() % (G1 + 1) == 0) {
		// TODO:
		// Spawn 1/5 Sonar Kit 4/5 Water Goodie:
		int G2 = (rand() % 5) + 1;
		if (G2 <= 4) {
			// Spawn Water
			int x = 0;
			int y = 0;
			do {
				x = rand() % (ICE_ARRAY_SIZE + 1);
				y = rand() % (ICE_ARRAY_SIZE + 1);
			} while (!isEmptySpace(x, y, 1.0));
			
			Actor* temp = new WaterPool(x,y, this, getLevel());
			actors.push_back(temp);
			temp = nullptr;
		}
		else if (G2 == 5) {
			// Spawn Sonar Kit
			Actor* temp = new SonarKit(this, getLevel());
			actors.push_back(temp);
			temp = nullptr;
		}
	}
}

void StudentWorld::handleCollisions(std::vector<Actor*>& actors) {
	// Handle Player Collisions/Interactions
	for (int i = 0; i < actors.size(); i++) {
		if (actors[i] != nullptr && doActorsCollide(iceman, actors[i])) {
			//iceman->interactWith(actors[i]);
			actors[i]->interactWith(iceman);
		}
	}

}

bool StudentWorld::doActorsCollide(const Actor* a1, const Actor* a2) const {
	if (a1 == nullptr || a2 == nullptr)
		return false;

	int x1 = a1->getX();
	int y1 = a1->getY();
	int size1 = a1->getHitBoxSize();

	int x2 = a2->getX();
	int y2 = a2->getY();
	int size2 = a2->getHitBoxSize();

	// Checks if 2D boxes overlap
	bool noOverlap = (x1 + size1 - 1 < x2 || x2 + size2 - 1 < x1 ||
		y1 + size1 - 1 < y2 || y2 + size2 - 1 < y1);

	return !noOverlap;

}

bool StudentWorld::willActorsCollide(int x1, int y1, int size1, const Actor* a2) const {
	if (a2 == nullptr)
		return false;

	int x2 = a2->getX();
	int y2 = a2->getY();
	int size2 = a2->getHitBoxSize();

	// Checks if 2D boxes overlap
	bool noOverlap = (x1 + size1 - 1 < x2 || x2 + size2 - 1 < x1 ||
		y1 + size1 - 1 < y2 || y2 + size2 - 1 < y1);

	return !noOverlap;
}

void StudentWorld::updateAllActors()
{
	for (int i = 0; i < actors.size(); i++)
	{
		Actor* a = actors[i];
		if (a == nullptr || !a->isActive())
			continue;

		a->update();

		if (a->isEnvironmentObject())
			if (calculateDistance(iceman, a) <= 4)
				a->setVisible(true);

		if (a->hasGravity())
		{
			bool isStable = checkBelowForIce(a);

			if (isStable && a->getState() == "falling")
				a->setState("crashed");
			else if (isStable)
				a->setState("stable");
			else if (!isStable && a->getState() == "stable")
				a->setState("waiting");
		}
	}

	for (auto actor : actors)
	{
		Boulder* boulder = dynamic_cast<Boulder*>(actor);
		if (boulder && boulder->getState() == "falling")
		{
			for (auto other_actor : actors)
			{
				Protester* p = dynamic_cast<Protester*>(other_actor);
				if (p && p->isActive() && !p->isLeaving())
				{
					if (calculateDistance(boulder, p) <= 3.0)
					{
						p->takeDamage(100, true);
					}
				}
			}
		}
	}
}

bool StudentWorld::checkBelowForIce(Actor* a) {
	int spriteSize = a->getHitBoxSize();
	int x = a->getX();
	int y = a->getY() - 1;
	for (int i = 0; i < spriteSize; i++) {
		if (ice2DArray[x + i][y] != nullptr)
			return true;
	}

	return false;
}

void StudentWorld::mineIce() {
	Actor::Direction d = iceman->getDirection();
	int icemanX = iceman->getX();
	int icemanY = iceman->getY();
	int iceX = 0;
	int iceY = 0;

	switch (d) {
	case Actor::Direction::right:
		iceX = icemanX + 3;
		iceY = icemanY - 1;

		for (int i = 0; i < 4; i++) {
			iceY++;
			if (iceX >= 0 && iceX < ICE_ARRAY_SIZE && iceY >= 0 && iceY < ICE_ARRAY_SIZE)
				auxMineIce(ice2DArray[iceX][iceY], iceX, iceY);
		}
		break;
	case Actor::Direction::left:
		iceX = icemanX;
		iceY = icemanY - 1;

		for (int i = 0; i < 4; i++) {
			iceY++;
			if (iceX >= 0 && iceX < ICE_ARRAY_SIZE && iceY >= 0 && iceY < ICE_ARRAY_SIZE)
				auxMineIce(ice2DArray[iceX][iceY], iceX, iceY);
		}
		break;
	case Actor::Direction::down:
		iceX = icemanX - 1;
		iceY = icemanY;

		for (int i = 0; i < 4; i++) {
			iceX++;
			if (iceX >= 0 && iceX < ICE_ARRAY_SIZE && iceY >= 0 && iceY < ICE_ARRAY_SIZE)
				auxMineIce(ice2DArray[iceX][iceY], iceX, iceY);
		}
		break;
	case Actor::Direction::up:
		iceX = icemanX - 1;
		iceY = icemanY + 3;

		for (int i = 0; i < 4; i++) {
			iceX++;
			if (iceX >= 0 && iceX < ICE_ARRAY_SIZE && iceY >= 0 && iceY < ICE_ARRAY_SIZE)
				auxMineIce(ice2DArray[iceX][iceY], iceX, iceY);
		}
		break;
	}
}

void StudentWorld::auxMineIce(Ice* iceToBreak, int iceX, int iceY) {
	if (iceToBreak != nullptr) {
		iceToBreak->setActive(false);
		playSound(SOUND_DIG);
	}
}

int StudentWorld::determineGameStatus() {
	if (iceman->getHitPoints() <= 0) {
		decLives();
		return GWSTATUS_PLAYER_DIED;
	}

	else if (numBarrelsOfOil <= 0) {
		return GWSTATUS_FINISHED_LEVEL;
	}

	return GWSTATUS_CONTINUE_GAME;
}


void StudentWorld::useSonarKit(Actor* a1) {
	for (Actor* a2 : actors) {
		if (a1 == nullptr || a2 == nullptr)
			continue;

		if (a2->isGoodieObject() && calculateDistance(a1, a2) < 12)
			a2->setVisible(true);
	}
}


// general helper functions:
double StudentWorld::calculateDistance(const Actor* a1, const Actor* a2) const {
	if (a1 == nullptr || a2 == nullptr) {
		return 0;
	}
	return calculateDistance(a1->getX(), a1->getY(), a1->getSize(), a2->getX(), a2->getY(), a2->getSize());
}

double StudentWorld::calculateDistance(int x, int y, double a1Size, const Actor* a2) const {
	if (a2 == nullptr)
		return 0;

	return calculateDistance(x, y, a1Size, a2->getX(), a2->getY(), a2->getSize());
}

double StudentWorld::calculateDistance(int x1, int y1, double a1Size, int x2, int y2, double a2Size) const {
	double centerX1 = x1 + a1Size / 2.0;
	double centerY1 = y1 + a1Size / 2.0;

	double centerX2 = x2 + a2Size / 2.0;
	double centerY2 = y2 + a2Size / 2.0;

	double xDiff = centerX2 - centerX1;
	double yDiff = centerY2 - centerY1;

	return sqrt(xDiff * xDiff + yDiff * yDiff);
}

bool StudentWorld::isProtesterPathClear(int x, int y) const {
	// Check if the target coordinates are within the playable area for a 4x4 object
	if (x < 0 || x > 60 || y < 0 || y > 60) {
		return false;
	}

	// Check for Ice in the 4x4 area, but only within the bounds of the ice field (y < 60)
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			int currentX = x + i;
			int currentY = y + j;

			// Only check for ice if we are inside the ice field's bounds (0-59)
			if (currentX < 60 && currentY < 60) {
				if (ice2DArray[currentX][currentY] != nullptr) {
					return false; // Path is blocked by ice
				}
			}
		}
	}
	// Check for Boulders
	for (const auto& actor : actors) {
		if (actor && actor->isClippable() && dynamic_cast<Boulder*>(actor)) {
			// Check if the center of the protester would be within 3.0 units of a boulder's center
			if (calculateDistance(x, y, 1.0, actor->getX(), actor->getY(), 1.0) <= 3.0) {
				return false; // Path is blocked by a boulder
			}
		}
	}

	// If all checks pass, the path is clear
	return true;
}

Actor::Direction StudentWorld::lineOfSightToIceman(const Actor* p) const {
	int pX = p->getX();
	int pY = p->getY();
	int iX = iceman->getX();
	int iY = iceman->getY();

	// They must be in the same row or column to have line of sight
	if (pX != iX && pY != iY) {
		return Actor::Direction::none;
	}

	if (pX == iX) // Potential Vertical line of sight
	{
		int startY = std::min(pY, iY);
		int endY = std::max(pY, iY);
		for (int y = startY; y < endY; ++y) {
			if (!isProtesterPathClear(pX, y)) return Actor::Direction::none;
		}
		return (pY < iY) ? Actor::Direction::up : Actor::Direction::down;
	}
	else // Potential Horizontal line of sight
	{
		int startX = std::min(pX, iX);
		int endX = std::max(pX, iX);
		for (int x = startX; x < endX; ++x) {
			if (!isProtesterPathClear(x, pY)) return Actor::Direction::none;
		}
		return (pX < iX) ? Actor::Direction::right : Actor::Direction::left;
	}
}

bool StudentWorld::annoyProtesterAt(const Actor* annoyer, int damage)
{
	for (auto actor : actors)
	{
		Protester* p = dynamic_cast<Protester*>(actor);
		if (p && p->isActive() && calculateDistance(annoyer, p) <= 3.0)
		{
			p->takeDamage(damage);
			return true; // Annoyed at least one protester
		}
	}
	return false; // No protesters were in range
}

Actor::Direction StudentWorld::getDirectionToExit(int x, int y) {
	return m_exitMap[x][y];
}

void StudentWorld::computeExitMap()
{
	// Initialize map with 'none'
	for (int i = 0; i < 64; ++i) {
		for (int j = 0; j < 64; ++j) {
			m_exitMap[i][j] = Actor::Direction::none;
		}
	}

	std::queue<std::pair<int, int>> q;
	q.push({ 60, 60 }); // Start BFS from the exit
	m_exitMap[60][60] = Actor::Direction::none; // At the exit

	while (!q.empty())
	{
		int currX = q.front().first;
		int currY = q.front().second;
		q.pop();

		// Check UP
		if (isProtesterPathClear(currX, currY + 1) && m_exitMap[currX][currY + 1] == Actor::Direction::none) {
			m_exitMap[currX][currY + 1] = Actor::Direction::down;
			q.push({ currX, currY + 1 });
		}
		// Check DOWN
		if (isProtesterPathClear(currX, currY - 1) && m_exitMap[currX][currY - 1] == Actor::Direction::none) {
			m_exitMap[currX][currY - 1] = Actor::Direction::up;
			q.push({ currX, currY - 1 });
		}
		// Check LEFT
		if (isProtesterPathClear(currX - 1, currY) && m_exitMap[currX - 1][currY] == Actor::Direction::none) {
			m_exitMap[currX - 1][currY] = Actor::Direction::right;
			q.push({ currX - 1, currY });
		}
		// Check RIGHT
		if (isProtesterPathClear(currX + 1, currY) && m_exitMap[currX + 1][currY] == Actor::Direction::none) {
			m_exitMap[currX + 1][currY] = Actor::Direction::left;
			q.push({ currX + 1, currY });
		}
	}
}

bool StudentWorld::bribeProtesterAt(const Actor* goldNugget)
{
	for (auto actor : actors)
	{
		Protester* p = dynamic_cast<Protester*>(actor);
		// Find an active protester that isn't already leaving
		if (p && p->isActive() && !p->isLeaving() && calculateDistance(goldNugget, p) <= 3.0)
		{
			p->pickedUpGoldNugget(); // Tell the protester it got bribed
			return true;
		}
	}
	return false;
}

Actor::Direction StudentWorld::findPathToIceman(const Actor* p, int& moves)
{
	// ... (the queue and distance grid setup is the same) ...
	int distance[64][64];
	for (int i = 0; i < 64; ++i) { for (int j = 0; j < 64; ++j) { distance[i][j] = -1; } }
	std::queue<std::pair<int, int>> q;
	int startX = iceman->getX(), startY = iceman->getY();
	q.push({ startX, startY });
	distance[startX][startY] = 0;
	int pX = p->getX(), pY = p->getY();

	while (!q.empty())
	{
		int currX = q.front().first;
		int currY = q.front().second;
		q.pop();

		if (currX == pX && currY == pY)
		{
			moves = distance[currX][currY];
			// BACKTRACKING TO FIND THE CORRECT FIRST STEP
			// Check which neighbor of the protester is one step closer to the Iceman
			if (pY > 0 && distance[pX][pY - 1] == moves - 1) return Actor::Direction::down;
			if (pY < 60 && distance[pX][pY + 1] == moves - 1) return Actor::Direction::up;
			if (pX > 0 && distance[pX - 1][pY] == moves - 1) return Actor::Direction::left;
			if (pX < 60 && distance[pX + 1][pY] == moves - 1) return Actor::Direction::right;
			return Actor::Direction::none;
		}

		// Explore neighbors (using the fully qualified enum names)
		if (isProtesterPathClear(currX, currY - 1) && distance[currX][currY - 1] == -1) {
			distance[currX][currY - 1] = distance[currX][currY] + 1;
			q.push({ currX, currY - 1 });
		}
		if (isProtesterPathClear(currX, currY + 1) && distance[currX][currY + 1] == -1) {
			distance[currX][currY + 1] = distance[currX][currY] + 1;
			q.push({ currX, currY + 1 });
		}
		if (isProtesterPathClear(currX - 1, currY) && distance[currX - 1][currY] == -1) {
			distance[currX - 1][currY] = distance[currX][currY] + 1;
			q.push({ currX - 1, currY });
		}
		if (isProtesterPathClear(currX + 1, currY) && distance[currX + 1][currY] == -1) {
			distance[currX + 1][currY] = distance[currX][currY] + 1;
			q.push({ currX + 1, currY });
		}
	}

	moves = -1;
	return Actor::Direction::none;
}