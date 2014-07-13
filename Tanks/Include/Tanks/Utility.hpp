#ifndef TANKS_UTILITY_HPP
#define TANKS_UTILITY_HPP

#include <SFML/System/Vector2.hpp>

#include <sstream>


namespace sf
{
	class Sprite;
	class Text;
}

// Since std::to_string doesn't work on MinGW we have to implement
// our own to support all platforms.
template <typename T>
std::string toString(const T& value);

// Calls setOrigin() with the center of the object
void  centerOrigin(sf::Sprite& sprite);
void  centerOrigin(sf::Text& text);

// Calls setScale() to resize the object
void setSize(sf::Sprite& sprite, const sf::Vector2f& desiredSize);

#include <Tanks/Utility.inl>
#endif // TANKS_UTILITY_HPP