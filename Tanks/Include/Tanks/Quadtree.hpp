#ifndef TANKS_QUADTREE_HPP
#define TANKS_QUADTREE_HPP

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <array>
#include <memory>
#include <vector>

class SceneNode;  // prevent include loop


// For use with objects of type SceneNode
class Quadtree : public sf::Drawable
{
  public:
            Quadtree();
            Quadtree(int level, sf::FloatRect bounds);

    void                      clear();
    void                      split();
    int                       getIndex(const SceneNode& sceneNode) const;
    void                      insert(SceneNode& sceneNode);
    std::vector<SceneNode*>   retrieve(const SceneNode& sceneNode) const;

    sf::FloatRect             getBounds() const;
    void                      setBounds(const sf::FloatRect& rect);

    virtual void	draw(sf::RenderTarget& target, sf::RenderStates states) const;

  
  private:
    void                      drawBounds(sf::RenderTarget& target) const;


  private:
    static const int            mMaxObjects;
    static const int            mMaxLevels;

    int                         mLevel;
    std::vector<SceneNode*>     mObjects;
    sf::FloatRect               mBounds;
    std::array<Quadtree*, 4>    mTreeNodes;  // subdivisions of the quadtree
};

#endif // TANKS_QUADTREE_HPP