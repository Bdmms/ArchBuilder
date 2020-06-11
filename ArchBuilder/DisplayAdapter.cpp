#include "DisplayAdapter.h"

DisplayAdaptor::DisplayAdaptor(unsigned char* ptr) : memory(ptr), updater(&DisplayAdaptor::update, this)
{
    
}

void DisplayAdaptor::update()
{
    sf::RenderWindow window(sf::VideoMode(240, 160), "GBA");

    // activate the window's context
    window.setActive(true);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            window.clear();
            window.display();
        }
    }
}
