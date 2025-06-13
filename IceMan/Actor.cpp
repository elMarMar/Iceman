#include "Actor.h"
#include "StudentWorld.h"
// ----****----
#include <cmath>
#include <queue>

Actor::~Actor() {}

Entity::~Entity() {}

Iceman::~Iceman() {}

void Iceman::update() {
	if (!isActive())
		return;
	handleInput();
}

void Iceman::takeDamage(unsigned int damageAmount)
{
	// This is a simplified version. We can add stunning effects later.
	Entity::takeDamage(damageAmount);
}

void Iceman::interactWith(Actor* a) {
	if (!a->isActive())
		return;
	/*
	if (a->isGoodieObject()) {
		//TODO: PROBABLY A BETTER WAY TO DO THIS:
		else if (dynamic_cast<SonarKit*>(a) != nullptr) {
			incSonarCharges();
		}
	}
	*/
}

void Iceman::handleInput()
{
	int ch;
	if (getStudentWorld()->getKey(ch) == true)
	{
		int x = getX();
		int y = getY();
		const StudentWorld* sw = getStudentWorld();

		switch (ch) {
		case KEY_PRESS_LEFT:
			if (getDirection() == Direction::left && sw->checkCoordsAreValid(x - 1, y, getHitBoxSize()))
				moveTo(x - 1, y);
			else
				setDirection(Direction::left);
			break;
		case KEY_PRESS_RIGHT:
			if (getDirection() == Direction::right && sw->checkCoordsAreValid(x + 1, y, getHitBoxSize()))
				moveTo(x + 1, y);
			else
				setDirection(Direction::right);
			break;
		case KEY_PRESS_UP:
			if (getDirection() == Direction::up && sw->checkCoordsAreValid(x, y + 1, getHitBoxSize()))
				moveTo(x, y + 1);
			else
				setDirection(Direction::up);
			break;
		case KEY_PRESS_DOWN:
			if (getDirection() == Direction::down && sw->checkCoordsAreValid(x, y - 1, getHitBoxSize()))
				moveTo(x, y - 1);
			else
				setDirection(Direction::down);
			break;
		case 'z':
		case 'Z':
			handleSonarKeyInput();
			break;
		case KEY_PRESS_TAB:
			if (getNumGoldNuggets() > 0)
			{
				decNumGoldNuggets();
				// Create a new temporary nugget at the Iceman's position
				GoldNugget* nugget = new GoldNugget(getX(), getY(), getStudentWorld());
				nugget->setCanProtestorPickUp(true); // Make it for protesters only
				nugget->setVisible(true);
				getStudentWorld()->addActor(nugget);
			}
			break;

		case KEY_PRESS_SPACE:
			handleSquirtKeyInput();
			break;

		case KEY_PRESS_ESCAPE:
			takeDamage(10000);
			//getStudentWorld()->decLives();
			break;
		}
	}
}

void Iceman::handleSonarKeyInput()
{
	if (getNumSonarCharges() > 0) {
		getStudentWorld()->playSound(SOUND_SONAR);
		getStudentWorld()->useSonarKit(this);
		decNumSonarCharges();
	}
}

void Iceman::handleGoldNuggetKeyInput()
{
	if (getNumGoldNuggets() > 0) {
		GoldNugget* g = new GoldNugget(getX(), getY(), getStudentWorld());
		g->setCanProtestorPickUp(true);
		g->setVisible(true);
		getStudentWorld()->addActor(g);
	}
}

void Iceman::handleSquirtKeyInput()
{
	if (getNumWaterSquirts() > 0) {
		// find coords to place squirt in front of Player:
		int x = getX();
		int y = getY();


		switch (getDirection()) {
		case GraphObject::Direction::down:
			y -= 2;
			break;
		case GraphObject::Direction::left:
			x -= 2;
			break;
		case GraphObject::Direction::up:
			y += 2;
			break;
		
		case GraphObject::Direction::right:
		default:
			x += 4;
			break;
		}
		
		Squirt* s = new Squirt(x, y, getDirection(), getStudentWorld());
		getStudentWorld()->addActor(s);
		decNumWaterSquirts();
	}
}

Environment::~Environment() {}

Goodies::~Goodies() {}

Boulder::~Boulder() {}
void Boulder::update() {
	if (!isActive())
		return;

	if (getState() == "stable") {}

	else if (getState() == "waiting") {
		if (getCurrentWaitingTick() > 0)
			decCurrentWaitingTick();
		else {
			setState("falling");
			setClippable(false);
			getStudentWorld()->playSound(SOUND_FALLING_ROCK);
		}
	}

	else if (getState() == "falling") {
		if (getY() < 0)
			setActive(false);
		else {
			moveTo(this->getX(), this->getY() - 1);
		}
	}

	else if (getState() == "crashed")
		setActive(false);
}

void Boulder::interactWith(Actor* a)
{
	if (a->isEntityObject()) {
		std::string state = getState();
		if (state == "falling") {
			static_cast<Entity*>(a)->takeDamage(10);
		}
	}
}

void Squirt::update()
{
	if (!isActive())
		return;

	// Check for collisions with Protesters
	if (getStudentWorld()->annoyProtesterAt(this, 2)) { // Annoy for 2 HP
		setActive(false); // The squirt disappears after hitting
		return;
	}

	if (getTravelDistance() >= MAX_TRAVEL_DISTANCE || !getStudentWorld()->isProtesterPathClear(getX(), getY())) {
		setActive(false);
		return;
	}

	// Move one step forward
	incrementDistanceTraveled();
	int x = getX(), y = getY();
	switch (getDirection()) {
	case up:    moveTo(x, y + 1); break;
	case down:  moveTo(x, y - 1); break;
	case left:  moveTo(x - 1, y); break;
	case right: moveTo(x + 1, y); break;
	case none: break;
	}
}

GoldNugget::~GoldNugget() {}

// In Actor.cpp, replace the GoldNugget::update() method

void GoldNugget::update()
{
	if (!isActive())
		return;

	// Logic for player-pickup-able nuggets (hidden in ice)
	if (getCanPlayerPickUp())
	{
		if (!isVisible() && getStudentWorld()->calculateDistance(this, getStudentWorld()->getIceman()) <= 4.0)
		{
			setVisible(true);
			return;
		}
		if (getStudentWorld()->calculateDistance(this, getStudentWorld()->getIceman()) <= 3.0)
		{
			setActive(false);
			getStudentWorld()->playSound(SOUND_GOT_GOODIE);
			getStudentWorld()->increaseScore(10);
			getStudentWorld()->getIceman()->incGoldNuggets();
		}
		return;
	}

	// NEW LOGIC: For protester-pickup-able nuggets (dropped by player)
	if (getCanProtestorPickUp())
	{
		// Check if a protester is close enough to pick it up
		if (getStudentWorld()->bribeProtesterAt(this))
		{
			setActive(false); // Nugget gets picked up and disappears
			getStudentWorld()->playSound(SOUND_PROTESTER_FOUND_GOLD);
		}

		// Handle lifetime of temporary nuggets
		lifetimeTicks--;
		if (lifetimeTicks <= 0) {
			setActive(false);
		}
	}
}

void GoldNugget::interactWith(Actor* a)
{
	if (a->isPlayerObject() && canPlayerPickUp) {
		Iceman* player = dynamic_cast<Iceman*>(a);
		if (player != nullptr) {
			player->incGoldNuggets();
			getStudentWorld()->increaseScore(25);
			getStudentWorld()->playSound(SOUND_GOT_GOODIE);
			setActive(false);
		}
	}
}

BarrelOfOil::~BarrelOfOil() {}

void BarrelOfOil::update()
{
	if (!isActive())
		return;
}

void BarrelOfOil::interactWith(Actor* a) {
	if (!isActive())
		return;

	if (a->isPlayerObject()) {
		StudentWorld* playerWorld = a->getStudentWorld();
		setActive(false);
		playerWorld->playSound(SOUND_FOUND_OIL);
		playerWorld->increaseScore(1000);
		playerWorld->decNumBarrelsOfOil();
	}
}

Ice::~Ice() {}

void SonarKit::update()
{
	if (!isActive())
		return;

	if (lifetimeTicks <= 0)
		setActive(false);

	lifetimeTicks--;
}

void SonarKit::interactWith(Actor* a) {
	if (a->isPlayerObject()) {
		Iceman* player = dynamic_cast<Iceman*>(a);
		if (player != nullptr) {
			player->incSonarCharges();
			getStudentWorld()->increaseScore(75);
			getStudentWorld()->playSound(SOUND_GOT_GOODIE);
			setActive(false);
		}
	}
}

void WaterPool::update()
{
	if (!isActive())
		return;

	if (lifetimeTicks <= 0)
		setActive(false);

	lifetimeTicks--;
}

void WaterPool::interactWith(Actor* a)
{
	if (a->isPlayerObject()) {
		Iceman* player = dynamic_cast<Iceman*>(a);
		if (player != nullptr) {
			setActive(false);
			getStudentWorld()->playSound(SOUND_GOT_GOODIE);
			for (int i = 1; i <= 5; i++)
				player->incWaterSquirts();
			getStudentWorld()->increaseScore(100);
		}

	}
}

Protester::Protester(int imageID, int startX, int startY, StudentWorld* sw, int hp)
	: Entity(imageID, startX, startY, left, 1.0, 0, sw, hp),
	m_leaveOilFieldState(false),
	m_numSquaresToMoveInCurrentDirection((rand() % 53) + 8),
	m_stareTicks(0),
	m_restingTicks(0),
	m_shoutTickCount(0),
	m_perpendicularTurnTickCount(200) {
	setClippable(false);
}

Protester::~Protester() {}

void Protester::update()
{
	if (!isActive())
		return;
	if (m_restingTicks > 0)
	{
		m_restingTicks--;
		return;
	}
	else
	{
		m_restingTicks = getTicksToWaitBetweenMoves();
		m_shoutTickCount++;
		m_perpendicularTurnTickCount++;
	}
	if (m_leaveOilFieldState)
	{
		if (getX() == 60 && getY() == 60) {
			setActive(false);
			return;
		}
		Direction dir = getStudentWorld()->getDirectionToExit(getX(), getY());
		setDirection(dir);
		int targetX = getX(), targetY = getY();
		if (dir == Actor::Direction::right) targetX++;
		else if (dir == Actor::Direction::left) targetX--;
		else if (dir == Actor::Direction::up) targetY++;
		else if (dir == Actor::Direction::down) targetY--;
		moveTo(targetX, targetY);
		return;
	}
	performNormalProtesterBehavior();
}

void Protester::takeDamage(unsigned int damageAmount, bool bonkedByBoulder)
{
	if (m_leaveOilFieldState) // Already leaving, cannot be annoyed further
		return;

	Entity::takeDamage(damageAmount); // Decrease HP from Entity base class

	if (getHitPoints() > 0)
	{
		// Didn't give up yet, just stunned
		getStudentWorld()->playSound(SOUND_PROTESTER_ANNOYED);
		int ticksToStun = std::max(50, 100 - (int)getStudentWorld()->getLevel() * 10);
		m_restingTicks += ticksToStun; // Add stun time to resting ticks
	}
	else
	{
		// Gave up!
		m_leaveOilFieldState = true;
		getStudentWorld()->playSound(SOUND_PROTESTER_GIVE_UP);
		m_restingTicks = 0; // Start leaving immediately

		if (bonkedByBoulder) {
			getStudentWorld()->increaseScore(500);
		}
		else {
			// Check which type of protester this is to award correct points
			if (dynamic_cast<HardcoreProtester*>(this))
				getStudentWorld()->increaseScore(250);
			else
				getStudentWorld()->increaseScore(100);
		}
	}
}

bool Protester::isLeaving() const
{
	return m_leaveOilFieldState;
}

void Protester::pickedUpGoldNugget() {
}

int Protester::getTicksToWaitBetweenMoves() const {
	return (std::max)(0, (int)(3 - getStudentWorld()->getLevel() / 4.0));
}

void Protester::activateLeaveOilFieldState() {
}

void Protester::calcuatePathToExit() {
}

void Protester::handleShouting() {
}

void Protester::performNormalProtesterBehavior() {
	doCommonAI();
}

RegularProtester::RegularProtester(int startX, int startY, StudentWorld* sw)
	: Protester(IID_PROTESTER, startX, startY, sw, 5) {
}

RegularProtester::~RegularProtester() {}

void Protester::doCommonAI()
{
	Iceman* player = getStudentWorld()->getIceman();

	if (getStudentWorld()->calculateDistance(this, player) <= 4.0 && m_shoutTickCount >= 15)
	{
		bool facingPlayer = false;
		switch (getDirection()) {
		case Actor::Direction::up:    if (getY() <= player->getY()) facingPlayer = true; break;
		case Actor::Direction::down:  if (getY() >= player->getY()) facingPlayer = true; break;
		case Actor::Direction::left:  if (getX() >= player->getX()) facingPlayer = true; break;
		case Actor::Direction::right: if (getX() <= player->getX()) facingPlayer = true; break;
		case Actor::Direction::none: break;
		}

		if (facingPlayer) {
			getStudentWorld()->playSound(SOUND_PROTESTER_YELL);
			player->takeDamage(2);
			m_shoutTickCount = 0;
			return;
		}
	}

	Actor::Direction sightDirection = getStudentWorld()->lineOfSightToIceman(this);
	if (sightDirection != Actor::Direction::none && getStudentWorld()->calculateDistance(this, player) > 4.0)
	{
		setDirection(sightDirection);
		int targetX = getX(), targetY = getY();
		if (sightDirection == Actor::Direction::right) targetX++;
		else if (sightDirection == Actor::Direction::left) targetX--;
		else if (sightDirection == Actor::Direction::up) targetY++;
		else if (sightDirection == Actor::Direction::down) targetY--;
		moveTo(targetX, targetY);
		m_numSquaresToMoveInCurrentDirection = 0;
		return;
	}

	// If we can't see the player, decrement move counter
	m_numSquaresToMoveInCurrentDirection--;
	if (m_numSquaresToMoveInCurrentDirection <= 0)
	{
		// Time to pick a new direction
		std::vector<Actor::Direction> validDirections;
		for (int i = 1; i <= 4; ++i) {
			Actor::Direction dir = (Actor::Direction)i;
			int nextX = getX(), nextY = getY();
			if (dir == Actor::Direction::up) nextY++; else if (dir == Actor::Direction::down) nextY--; else if (dir == Actor::Direction::left) nextX--; else if (dir == Actor::Direction::right) nextX++;
			if (getStudentWorld()->isProtesterPathClear(nextX, nextY)) validDirections.push_back(dir);
		}
		if (!validDirections.empty()) {
			setDirection(validDirections[rand() % validDirections.size()]);
			m_numSquaresToMoveInCurrentDirection = (rand() % 53) + 8;
		}
		return; // End turn after picking new direction
	}

	if (m_perpendicularTurnTickCount >= 200)
	{
		Direction currentDir = getDirection();
		std::vector<Direction> perpDirs;
		if (currentDir == Actor::Direction::up || currentDir == Actor::Direction::down) {
			if (getStudentWorld()->isProtesterPathClear(getX() - 1, getY())) perpDirs.push_back(Actor::Direction::left);
			if (getStudentWorld()->isProtesterPathClear(getX() + 1, getY())) perpDirs.push_back(Actor::Direction::right);
		}
		else {
			if (getStudentWorld()->isProtesterPathClear(getX(), getY() - 1)) perpDirs.push_back(Actor::Direction::down);
			if (getStudentWorld()->isProtesterPathClear(getX(), getY() + 1)) perpDirs.push_back(Actor::Direction::up);
		}

		if (!perpDirs.empty()) {
			setDirection(perpDirs[rand() % perpDirs.size()]);
			m_numSquaresToMoveInCurrentDirection = (rand() % 53) + 8;
			m_perpendicularTurnTickCount = 0;
		}
	}

	int targetX = getX(), targetY = getY();
	switch (getDirection()) {
	case Actor::Direction::up:    targetY++; break;
	case Actor::Direction::down:  targetY--; break;
	case Actor::Direction::left:  targetX--; break;
	case Actor::Direction::right: targetX++; break;
	case Actor::Direction::none:  return;
	}

	if (getStudentWorld()->isProtesterPathClear(targetX, targetY)) {
		moveTo(targetX, targetY);
	}
	else {
		m_numSquaresToMoveInCurrentDirection = 0; // Blocked, so pick a new direction next time
	}
}

void RegularProtester::performNormalProtesterBehavior() {
	doCommonAI(); 
}

void RegularProtester::pickedUpGoldNugget()
{
	getStudentWorld()->increaseScore(25);
	m_leaveOilFieldState = true; // Immediately decide to leave
	m_restingTicks = 0; // Don't wait, start leaving now
}

HardcoreProtester::HardcoreProtester(int startX, int startY, StudentWorld* sw)
	: Protester(IID_HARD_CORE_PROTESTER, startX, startY, sw, 20) {
}

HardcoreProtester::~HardcoreProtester() {}

void HardcoreProtester::performNormalProtesterBehavior()
{
	// A hardcore protester first tries its special tracking...
	int M = 16 + getStudentWorld()->getLevel() * 2;
	int movesToPlayer = -1;
	Actor::Direction dirToPlayer = getStudentWorld()->findPathToIceman(this, movesToPlayer);

	if (movesToPlayer != -1 && movesToPlayer <= M)
	{
		setDirection(dirToPlayer);
		int targetX = getX(), targetY = getY();
		if (dirToPlayer == Actor::Direction::right) targetX++;
		else if (dirToPlayer == Actor::Direction::left) targetX--;
		else if (dirToPlayer == Actor::Direction::up) targetY++;
		else if (dirToPlayer == Actor::Direction::down) targetY--;
		moveTo(targetX, targetY);
		return;
	}

	// ...if tracking fails, it falls back to the common AI
	doCommonAI();
}

void HardcoreProtester::pickedUpGoldNugget()
{
	getStudentWorld()->increaseScore(50);

	// Stare at the gold for a little while instead of leaving
	int ticksToStare = std::max(50, 100 - (int)getStudentWorld()->getLevel() * 10);
	m_restingTicks += ticksToStare; // Add stare time to resting ticks
}
