#include <Tanks/World.hpp>
#include <Tanks/DataTables.hpp>
#include <Tanks/Foreach.hpp>
#include <Tanks/Category.hpp>
#include <Tanks/Utility.hpp>
// #include <Tanks/Globals.hpp>

#include <SFML/Graphics/RenderWindow.hpp>

#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>


namespace
{
  const std::vector<LevelData> Table = initializeLevelData();
}

World::World(sf::RenderWindow& window, FontHolder& fonts,
             Player& player)
: mWindow(window)
, mWorldView(window.getDefaultView())
, mFonts(fonts)
, mLevel(player.getLevel())
// , mQuadtree()
, mTextures() 
, mSceneGraph()
, mSceneLayers()
, mWorldBounds(Table[mLevel].worldBounds)
, mPlayerTank(nullptr)
, mEnemySpawnPoints()
, mHuntingEnemies()
, mNumberOfEnemies(0)
, mNumberOfAliveEnemies(0)
, mNeedSortEnemies(false)
// , mOfstream("CollisionTestsData/CollisionTests.txt")
{
	loadTextures();
	buildScene();
  
	// Prepare the view
  if (Table[mLevel].viewType == World::Static)
	  mWorldView.setCenter(sf::Vector2f(mWindow.getSize()) / 2.f);
}

/*World::~World()
{
  // mOfstream.close();
}*/

void World::update(sf::Time dt)
{
  // Update world view and quadtree bounds
	if (Table[mLevel].viewType == World::Following)
    mWorldView.setCenter(mPlayerTank->getPosition());
  // sf::Vector2f newPosition(mWorldView.getCenter().x - mWorldView.getSize().x / 2.f,
  //                          mWorldView.getCenter().y - mWorldView.getSize().y / 2.f);
  // mQuadtree.setBounds(sf::FloatRect(newPosition, mWorldView.getSize()));

	// Reset player velocity and rotation offset
	mPlayerTank->setVelocity(0.f, 0.f);
  mPlayerTank->setRotationOffset(0.f);

  // These further update mCommandQueue 
  destroyProjectilesOutsideView();
  despawnEnemiesOutsideView();
  despawnBlocksOutsideView();
  updateEnemyCounters();
  updateHuntingEnemies();

	// Forward commands to scene graph, adapt velocity (scrolling, diagonal correction)
	while (!mCommandQueue.isEmpty())
		mSceneGraph.onCommand(mCommandQueue.pop(), dt);
	adaptPlayerVelocity();

  // Update the quadtree's objects
  // mQuadtree.clear();
  // mSceneGraph.insertIntoQuadtree(mQuadtree);

  // Collision detection and response (may destroy entities)
	handleCollisions();

  // Remove all entities that are marked for removal; create new ones
	mSceneGraph.removeWrecks();
	spawnEnemies();

  // Create new blocks if necessary
  spawnBlocks();

	// Regular update step
	mSceneGraph.update(dt, mCommandQueue);
}

void World::draw()
{
	mWindow.setView(mWorldView);
	mWindow.draw(mSceneGraph);
  // mWindow.draw(mQuadtree);
}

CommandQueue& World::getCommandQueue()
{
	return mCommandQueue;
}

bool World::hasAlivePlayer() const
{
	return !mPlayerTank->isMarkedForRemoval();
}

bool World::hasEnemies() const
{
  return (mNumberOfAliveEnemies > 0 || !mEnemySpawnPoints.empty());
}

void World::loadTextures()
{
	mTextures.load(Textures::HeroTank, "Media/Textures/HeroTank.png");
  mTextures.load(Textures::DummyTank, "Media/Textures/DummyTank.png");
  mTextures.load(Textures::HuntingTank1, "Media/Textures/HuntingTank1.png");
  mTextures.load(Textures::HuntingTank2, "Media/Textures/HuntingTank2.png");
  mTextures.load(Textures::GuardingTank1, "Media/Textures/GuardingTank1.png");
  mTextures.load(Textures::Metal, "Media/Textures/Metal.png");

  mTextures.load(Textures::Bullet, "Media/Textures/Bullet.png");
}

void World::adaptTankPositions()
{
	// Keep all tanks' positions inside the screen bounds, 
  // at least borderDistance units from the border
	sf::FloatRect viewBounds(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
  const float borderDistance = 50.f;

  // Handle player's tank
	sf::Vector2f position = mPlayerTank->getPosition();
	position.x = std::max(position.x, viewBounds.left + borderDistance);
	position.x = std::min(position.x, viewBounds.left + viewBounds.width - borderDistance);
	position.y = std::max(position.y, viewBounds.top + borderDistance);
	position.y = std::min(position.y, viewBounds.top + viewBounds.height - borderDistance);
	mPlayerTank->setPosition(position);
  /*
  // Handle enemy tanks
  FOREACH(Tank* enemyTank, mActiveEnemies)
  {
    sf::Vector2f position = enemyTank->getPosition();
	  position.x = std::max(position.x, viewBounds.left + borderDistance);
	  position.x = std::min(position.x, viewBounds.left + viewBounds.width - borderDistance);
	  position.y = std::max(position.y, viewBounds.top + borderDistance);
	  position.y = std::min(position.y, viewBounds.top + viewBounds.height - borderDistance);
	  enemyTank->setPosition(position);
  }
  */
}

void World::adaptPlayerVelocity()
{
	sf::Vector2f velocity = mPlayerTank->getVelocity();

	// If moving diagonally, reduce velocity (to have always same velocity)
	if (velocity.x != 0.f && velocity.y != 0.f)
		mPlayerTank->setVelocity(velocity / std::sqrt(2.f));
}

bool matchesCategories(SceneNode::Pair& colliders,
                       Category::Type type1,
                       Category::Type type2)
{
  unsigned int category1 = colliders.first->getCategory();
  unsigned int category2 = colliders.second->getCategory();

  if (type1 & category1 && type2 & category2)
  {
    return true;
  }
  else if (type1 & category2 && type2 & category1)
  {
    // Flip the elements of the pair to avoid having both
    // A-B and B-A collisions recorded
    std::swap(colliders.first, colliders.second);
    return true;
  }
  else
  {
    return false;
  }
}

void World::handleCollisions()
{
  // numberOfCollisionTests = 0;

  std::set<SceneNode::Pair> collisionPairs;
  mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);
  // mSceneGraph.checkCollisionsInQuadtree(mQuadtree, collisionPairs);

  // std::cout << "Tests: " << numberOfCollisionTests << '\n';
  // mOfstream << numberOfCollisionTests << '\n';

  FOREACH(SceneNode::Pair pair, collisionPairs)
  {
    if (matchesCategories(pair, Category::EnemyTank, 
                               Category::AlliedProjectile)
             || matchesCategories(pair, Category::PlayerTank,
                                  Category::EnemyProjectile))
    {
      auto& tank = static_cast<Tank&>(*pair.first);
      auto& projectile = static_cast<Projectile&>(*pair.second);

      tank.damage(projectile.getDamage());
      projectile.destroy();
    }
    else if (matchesCategories(pair, Category::PlayerTank, 
                                     Category::NonToxicTank)
             || matchesCategories(pair, Category::EnemyTank,
                                        Category::EnemyTank))
    {
      auto& tank1 = static_cast<Tank&>(*pair.first);
      auto& tank2 = static_cast<Tank&>(*pair.second);

      // Update the intersection rectangles of both tanks
      sf::FloatRect intersection;
      tank1.getBoundingRect().intersects(tank2.getBoundingRect(), 
                                         intersection);
      tank1.addCollisionWithTank(intersection);
      tank2.addCollisionWithTank(intersection);
    }
    else if (matchesCategories(pair, Category::PlayerTank, 
                                     Category::ToxicTank))
    {
      auto& player = static_cast<Tank&>(*pair.first);
      auto& toxicTank = static_cast<Tank&>(*pair.second);

      player.damage(1);

      // Update the intersection rectangles of both tanks
      sf::FloatRect intersection;
      player.getBoundingRect().intersects(toxicTank.getBoundingRect(), 
                                         intersection);
      player.addCollisionWithTank(intersection);
      toxicTank.addCollisionWithTank(intersection);
    }
    else if (matchesCategories(pair, Category::Tank, Category::Block))
    {
      auto& tank = static_cast<Tank&>(*pair.first);
      auto& block = static_cast<Block&>(*pair.second);

      // Update the intersection rectangle of the tank
      sf::FloatRect intersection;
      tank.getBoundingRect().intersects(block.getBoundingRect(), 
                                        intersection);
      tank.addCollisionWithBlock(intersection);
    }
    else if (matchesCategories(pair,
                               Category::Projectile,
                               Category::Block))
    {
      auto& projectile = static_cast<Projectile&>(*pair.first);
      auto& block = static_cast<Block&>(*pair.second);

      block.damage(projectile.getDamage());
      projectile.destroy();
    }
  }
}

void World::buildScene()
{
	// Initialize the different layers
	for (std::size_t i = 0; i < LayerCount; ++i)
	{
		Category::Type category = (i == MainGround) ? Category::SceneGroundLayer : Category::None;

		SceneNode::Ptr layer(new SceneNode(category));
		mSceneLayers[i] = layer.get();

		mSceneGraph.attachChild(std::move(layer));
	}

	// Prepare the tiled background
  sf::Texture& texture = mTextures.get(Table[mLevel].backgroundTexture);
	sf::IntRect textureRect(mWorldBounds);
	texture.setRepeated(true);

	// Add the background sprite to the scene
	std::unique_ptr<SpriteNode> backgroundSprite(new SpriteNode(texture, textureRect));
	backgroundSprite->setPosition(mWorldBounds.left, mWorldBounds.top);
	mSceneLayers[Background]->attachChild(std::move(backgroundSprite));

	// Add player's tank
	std::unique_ptr<Tank> leader(new Tank(Tank::Hero, mTextures, mFonts));
	mPlayerTank = leader.get();
	mPlayerTank->setPosition(Table[mLevel].playerSpawnPosition);
	mSceneLayers[MainGround]->attachChild(std::move(leader));

	// Add enemy tanks
	addEnemies();
  spawnEnemies();

  // Add blocks
  addBlocks();
  spawnBlocks();
}

int World::getNumberOfDestroyedEnemies()
{
  return (mNumberOfEnemies - mEnemySpawnPoints.size() - mNumberOfAliveEnemies);
}

void World::addEnemies()
{
  // Set spawn points according to the data read from a file;
  // set number of enemies to be used to determine if level is finished
  mEnemySpawnPoints = Table[mLevel].enemySpawnPoints;
  mNumberOfEnemies = mEnemySpawnPoints.size();
	mNeedSortEnemies = true;
}

void World::addEnemy(const Tank& tank)
{
  EnemySpawnPoint spawn(tank.getType(),
                        tank.getPosition().x,
                        tank.getPosition().y,
                        tank.getRotation(),
                        getNumberOfDestroyedEnemies(),
                        tank.getHitpoints(),
                        tank.getGuardingPathLength(),
                        tank.getGuardingAngle(),
                        tank.getTravelledDistance(),
                        tank.getAmountRotated(),
                        tank.getDirectionIndex());
  mEnemySpawnPoints.push_back(spawn);
  mNeedSortEnemies = true;
}

void World::spawnEnemies()
{
  if (mNeedSortEnemies)
  {
    // Sort all enemies according to the number of tanks that must be killed
    // before each appears, 
    // such that enemies with a lower amount of required kills are checked 
    // first for spawning
	  std::sort(mEnemySpawnPoints.begin(), mEnemySpawnPoints.end(), 
      [] (EnemySpawnPoint lhs, EnemySpawnPoint rhs)
	  {
		  return lhs.n < rhs.n;
	  });

    mNeedSortEnemies = false;
  }

  // Only check the spawns that are satisfied by the number of destroyed
  // enemies
  for (auto spawn = mEnemySpawnPoints.begin(); 
    (spawn != mEnemySpawnPoints.end() && spawn->n <= getNumberOfDestroyedEnemies()); )
  {
    // Only spawn an enemy if it is within the battlefield bounds;
    // ignore the spawn point otherwise
    if (getBattlefieldBounds().contains(spawn->x, spawn->y))
    {
      std::unique_ptr<Tank> enemy(new Tank(spawn->type, mTextures, mFonts));
      enemy->setPosition(spawn->x, spawn->y);
      enemy->setRotation(spawn->r);
      enemy->setTravelledDistance(spawn->td);
      enemy->setAmountRotated(spawn->ar);
      enemy->setDirectionIndex(spawn->di);
      enemy->setGuardingPathLength(spawn->gpl);
      enemy->setGuardingAngle(spawn->ga);
      
      // Change the enemy's hitpoints if it did not have full hitpoints when
      // made into a spawn (i.e. it was despawned)
      if (spawn->h != Tank::getMaxHitpoints(spawn->type))
        enemy->damage(enemy->getHitpoints() - spawn->h);

      mSceneLayers[MainGround]->attachChild(std::move(enemy));
    
      // Update alive enemies counter
      ++mNumberOfAliveEnemies;
    
		  // Enemy is spawned, remove from the list to spawn;
      // update the iterator
      spawn = mEnemySpawnPoints.erase(spawn);
    }
    else
    {
      ++spawn;
    }
  }
}

void World::addBlocks()
{
  mBlockSpawnPoints = Table[mLevel].blockSpawnPoints;
}

void World::addBlock(Block::Type type, sf::Vector2f spawnPosition,
                     sf::Vector2f size, int hitpoints)
{
  BlockSpawnPoint spawn(type, spawnPosition.x, spawnPosition.y,
                        size.x, size.y, hitpoints);
  mBlockSpawnPoints.push_back(spawn);
}

void World::spawnBlocks()
{
  // Spawn all blocks that are able to appear based on if seen by
  // the view
  for (auto spawn = mBlockSpawnPoints.begin(); 
    spawn != mBlockSpawnPoints.end(); )
  {
    sf::FloatRect blockRect = 
      sf::FloatRect(spawn->posX - (spawn->sizeX / 2.f), 
      spawn->posY - (spawn->sizeY / 2.f), spawn->sizeX, spawn->sizeY);
    if (getBattlefieldBounds().intersects(blockRect))
    {
      std::unique_ptr<Block> block(new Block(
        spawn->type, sf::Vector2f(spawn->sizeX, spawn->sizeY)));
      block->setPosition(spawn->posX, spawn->posY);
      
      // Change the block's hitpoints if it did not have full hitpoints when
      // made into a spawn (i.e. it was despawned)
      if (spawn->h != Block::getMaxHitpoints(spawn->type))
        block->damage(block->getHitpoints() - spawn->h);

      mSceneLayers[UpperGround]->attachChild(std::move(block));

      // Block is spawned, remove from the list to spawn; update the iterator
      spawn = mBlockSpawnPoints.erase(spawn);
    }
    else
    {
      ++spawn;
    }
  }
}

void World::destroyProjectilesOutsideView()
{
  Command command;
  command.category = Category::Projectile;
  command.action = derivedAction<Projectile>(
    [this] (Projectile& p, sf::Time)
  {
    if (!getBattlefieldBounds().intersects(p.getBoundingRect()))
      p.destroy();
  });

  mCommandQueue.push(command);
}

void World::despawnEnemiesOutsideView()
{
  Command command;
  command.category = Category::EnemyTank;
  command.action = derivedAction<Tank>(
    [this] (Tank& t, sf::Time)
  {
    if (!t.isDestroyed() && !getBattlefieldBounds().contains(t.getPosition()))
    {
      addEnemy(t);
      t.destroy();
    }
  });

  mCommandQueue.push(command);
}

void World::despawnBlocksOutsideView()
{
  Command command;
  command.category = Category::Block;
  command.action = derivedAction<Block>(
    [this] (Block& b, sf::Time)
  {
    if (!b.isDestroyed() && !getBattlefieldBounds().intersects(b.getBoundingRect()))
    {
      addBlock(b.getType(), b.getPosition(), b.getSize(), b.getHitpoints());
      b.destroy();
    }
  });

  mCommandQueue.push(command);
}

void World::updateEnemyCounters()
{
  Command enemyCollector;
  enemyCollector.category = Category::EnemyTank;
  enemyCollector.action = derivedAction<Tank>([this] (Tank& enemy, sf::Time)
  {
    if (enemy.isMarkedForRemoval())
    {
      --mNumberOfAliveEnemies;
    }
  });

  mCommandQueue.push(enemyCollector);
}

void World::updateHuntingEnemies()
{
  // Update the movements of enemy tanks that move towards the player
  
  // Setup command that guides all hunting enemies towards the player
  Command huntingEnemyGuider;
  huntingEnemyGuider.category = Category::EnemyTank;
  huntingEnemyGuider.action = derivedAction<Tank>([this] (Tank& enemy, sf::Time)
  {
    if (enemy.isMovingTowardsPlayer() && !enemy.isDestroyed())
    {
      sf::Vector2f velocity = sf::Vector2f(0.f, 0.f);

      // Update left/right movement
      if (enemy.getPosition().x > mPlayerTank->getPosition().x)
        velocity.x -= enemy.getMaxMovementSpeed();
      else if (enemy.getPosition().x < mPlayerTank->getPosition().x)
        velocity.x += enemy.getMaxMovementSpeed();

      // Update up/down movement
      if (enemy.getPosition().y > mPlayerTank->getPosition().y)
        velocity.y -= enemy.getMaxMovementSpeed();
      else if (enemy.getPosition().y < mPlayerTank->getPosition().y)
        velocity.y += enemy.getMaxMovementSpeed();
      
      // If moving diagonally, reduce velocity (to have always same velocity)
	    if (velocity.x != 0.f && velocity.y != 0.f)
        velocity /= std::sqrt(2.f);

      // Update rotation; multiply height by negative one to counter SFML's
      // upside down y-axis;
      // use desiredAngle to calculate the needed rotation offset
      float widthBetweenEnemyAndPlayer = 
        mPlayerTank->getPosition().x - enemy.getPosition().x;
      float heightBetweenEnemyAndPlayer =
        (mPlayerTank->getPosition().y - enemy.getPosition().y) * -1;
      float desiredAngle = 
        toDegree(arctan(heightBetweenEnemyAndPlayer, widthBetweenEnemyAndPlayer));
      float currentRotationAngle = toTrigAngle(enemy.getRotation());
      float rotationOffset = 
        (fixAngleToRangeDegrees(desiredAngle - currentRotationAngle) < 180.f) 
        ? -1.f : 1.f;

      enemy.setVelocity(velocity);
      enemy.setRotationOffset(rotationOffset * enemy.getMaxRotationSpeed());
    }
  });

  mCommandQueue.push(huntingEnemyGuider);
}

sf::FloatRect World::getViewBounds() const
{
	return sf::FloatRect(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
}

sf::FloatRect World::getBattlefieldBounds() const
{
  if (Table[mLevel].viewType == World::Static)
    return getViewBounds();
  else // Table[mLevel].viewType == World::Following
  {
    // Return view bounds + some area around all sides, where enemies
    // and blocks spawn
	  sf::FloatRect bounds = getViewBounds();
    const float extraArea = 100.f;
	  bounds.top -= extraArea;
	  bounds.height += extraArea * 2.f;
    bounds.left -= extraArea;
    bounds.width += extraArea * 2.f;

	  return bounds;
  }
}