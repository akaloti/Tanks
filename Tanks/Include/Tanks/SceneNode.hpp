/*
  Authors of original version: Artur Moreira, Henrik Vogelius Hansson, and
    Jan Haller
*/

#ifndef TANKS_SCENENODE_HPP
#define TANKS_SCENENODE_HPP

#include <Tanks/Category.hpp>
#include <Tanks/Quadtree.hpp>

#include <SFML/System/NonCopyable.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Drawable.hpp>

#include <vector>
#include <memory>
#include <set>
#include <utility>


struct Command;
class CommandQueue;

class SceneNode : public sf::Transformable, public sf::Drawable, private sf::NonCopyable
{
	public:
		typedef std::unique_ptr<SceneNode> Ptr;
    typedef std::pair<SceneNode*, SceneNode*> Pair;


	public:
		explicit			SceneNode(Category::Type category = Category::None);

		void					attachChild(Ptr child);
		Ptr						detachChild(const SceneNode& node);
		
		void					update(sf::Time dt, CommandQueue& commands);

		sf::Vector2f			getWorldPosition() const;
		sf::Transform			getWorldTransform() const;

    void          checkNodeCollision(SceneNode& node, 
                                     std::set<Pair>& collisionPairs);
    void          checkSceneCollision(SceneNode& sceneGraph,
                                      std::set<Pair>& collisionPairs);
    void          checkCollisionsInQuadtree(const Quadtree& quadtree,
                                            std::set<Pair>& collisionPairs);
    void          removeWrecks();

		void					onCommand(const Command& command, sf::Time dt);
		virtual unsigned int	getCategory() const;

    virtual sf::FloatRect getBoundingRect() const;
		virtual bool      isMarkedForRemoval() const;
    virtual bool			isDestroyed() const;

    void              insertIntoQuadtree(Quadtree& quadtree);


	private:
		virtual void			updateCurrent(sf::Time dt, CommandQueue& commands);
		void					updateChildren(sf::Time dt, CommandQueue& commands);

		virtual void			draw(sf::RenderTarget& target, sf::RenderStates states) const;
		virtual void			drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
		void					drawChildren(sf::RenderTarget& target, sf::RenderStates states) const;
		void					drawBoundingRect(sf::RenderTarget& target, sf::RenderStates states) const;


	private:
		std::vector<Ptr>		mChildren;
		SceneNode*				mParent;
		Category::Type			mDefaultCategory;
};

bool collision(const SceneNode& lhs, const SceneNode& rhs);

#endif // TANKS_SCENENODE_HPP
