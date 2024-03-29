/*
  Authors of original version: Artur Moreira, Henrik Vogelius Hansson, and
    Jan Haller
*/

#ifndef TANKS_WORLD_HPP
#define TANKS_WORLD_HPP

#include <Tanks/ResourceHolder.hpp>
#include <Tanks/ResourceIdentifiers.hpp>
#include <Tanks/Player.hpp>
#include <Tanks/SceneNode.hpp>
#include <Tanks/SpriteNode.hpp>
#include <Tanks/CommandQueue.hpp>
#include <Tanks/Command.hpp>
#include <Tanks/SpawnPoint.hpp>
#include <Tanks/LevelIdentifiers.hpp>
#include <Tanks/Quadtree.hpp>

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <vector>
#include <fstream>


// Forward declaration
namespace sf
{
	class RenderWindow;
}

class World : private sf::NonCopyable
{
  public:
    enum ViewType
    {
      Static,
      Following,
      Scrolling
    };


  public:
		explicit							World(sf::RenderWindow& window, FontHolder& fonts,
                                Player& player);
    //                       ~World();

		void								update(sf::Time dt);
		void								draw();
		
		CommandQueue&						getCommandQueue();

		bool 								hasAlivePlayer() const;
    bool                hasEnemies() const;


	private:
		void								loadTextures();
		void								adaptTankPositions();
		void								adaptPlayerVelocity();
    void                handleCollisions();
		
    void								buildScene();
    int                 getNumberOfDestroyedEnemies();
    void                addEnemies();
    void                addEnemy(const Tank& tank);
    void                spawnEnemies();
    void                addBlocks();
    void                addBlock(Block::Type type,
                                 sf::Vector2f spawnPosition,
                                 sf::Vector2f size,
                                 int hitpoints);
    void                spawnBlocks();
    void                destroyProjectilesOutsideView();
    void                despawnEnemiesOutsideView();
    void                despawnBlocksOutsideView();
    void                updateEnemyCounters();
    void                updateHuntingEnemies();
    sf::FloatRect       getViewBounds() const;
    sf::FloatRect       getBattlefieldBounds() const;


	private:
		enum Layer
		{
      // Order these layers from least to most visible
			Background,
			MainGround,
      UpperGround,
			LayerCount
		};


	private:
		sf::RenderWindow&					mWindow;
		sf::View							mWorldView;
		TextureHolder						mTextures;
    FontHolder&             mFonts;
    Levels::ID               mLevel;
    // Quadtree                  mQuadtree;

		SceneNode							mSceneGraph;
		std::array<SceneNode*, LayerCount>	mSceneLayers;
		CommandQueue						mCommandQueue;

		sf::FloatRect						mWorldBounds;
		Tank*							mPlayerTank;

    std::vector<EnemySpawnPoint>     mEnemySpawnPoints;
    std::vector<BlockSpawnPoint>     mBlockSpawnPoints;
    std::vector<Tank*>        mHuntingEnemies;
    int                       mNumberOfEnemies;
    int                       mNumberOfAliveEnemies;
    bool                      mNeedSortEnemies;

    // std::ofstream             mOfstream;
};

#endif // TANKS_WORLD_HPP
