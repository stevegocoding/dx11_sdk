#pragma once

#include "prerequisites.h"

struct app_event 
{
    app_event(const std::string _name)
        : name(_name) {}
    std::string name; 
};

struct swap_chain_resize_event : public app_event
{
    typedef app_event super;
    swap_chain_resize_event(unsigned int _width, unsigned int _height)
        : super("swap_chain_resized_event")
        , width(_width)
        , height(_height)
    {}
    
    unsigned int width, height; 
}; 

struct gui_event : public app_event
{
}; 
