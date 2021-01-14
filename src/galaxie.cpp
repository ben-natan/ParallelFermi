#include <stdlib.h>
#include <iostream>
#include <SDL_image.h>
#include "galaxie.hpp"

//_ ______________________________________________________________________________________________ _
galaxie::galaxie(int width, int height)
    :   m_width(width),
        m_height(height),
        m_planetes(width*height, habitable)
{}
//_ ______________________________________________________________________________________________ _
galaxie::galaxie(int width, int height, double chance_habitee)
    :   m_width(width),
        m_height(height),
        m_planetes(width*height)
{
    int i,j;
    for ( i = 0; i < height; ++i )
        for ( j = 0; j < width; ++j )
        {
            double val = std::rand()/(1.*RAND_MAX);
            if (val < chance_habitee)
            {
                m_planetes[i*width+j] = habitee;
            }
            else
                m_planetes[i*width+j] = habitable;
        }
}
//_ ______________________________________________________________________________________________ _
void 
galaxie::rend_planete_habitee(int x, int y)
{
    m_planetes[y*m_width + x] = habitee;
}
//_ ______________________________________________________________________________________________ _
void
galaxie::rend_planete_inhabitable(int x, int y)
{
    m_planetes[y*m_width + x] = inhabitable;
}
//_ ______________________________________________________________________________________________ _
void
galaxie::rend_planete_inhabitee(int x, int y)
{
    m_planetes[y*m_width + x] = habitable;
}
//_ ______________________________________________________________________________________________ _
void
galaxie::swap(galaxie& g)
{
    g.m_planetes.swap(this->m_planetes);
}
//_ ______________________________________________________________________________________________ _
void
galaxie::updateWithData(std::vector<char> data)
{
    m_planetes = data;
}
//_ ______________________________________________________________________________________________ _
void
galaxie::replaceLine(std::vector<char> data, int line)
{
    for (int i=0; i<m_width; i++) {
        m_planetes[line*m_height + i] = data[i];
    }
}
//_ ______________________________________________________________________________________________ _
void 
galaxie::updateByHBlocks(std::vector<char> data, int block, int nbp) 
{
    // int taskH  = (m_height/ (nbp-1)) +2;
    // if (block == 1) {
    //     for (int i = 0; i < (taskH-1)*m_width; i++) {
    //         m_planetes[i] = data[i+m_width];
    //     }
    // } else if (block == nbp-1) {
    //     for (int i = 0; i < (taskH-1)*m_width; i++) {
    //         m_planetes[(m_height-(taskH-2))*m_width + i] = data[i];
    //     }
    // } else { 
    //     for (int i = 0; i < m_width; i++) {
    //         m_planetes[(block-1)*(taskH-2)*m_width - m_width + i] = data[i];
    //     };
    //     for (int i = 0; i < m_width; i++) {
    //         m_planetes[block*(taskH-2)*m_width + m_width + i] = data[(taskH-1)*m_width + i];
    //     }
    //     for (int i = 0; i < (taskH-4)*m_width; i++) {
    //         m_planetes[(block-1)*(taskH-2)*m_width + m_width + i] = data[2*m_width +i];
    //     };
    // }
    int taskH = (m_height/(nbp-1)) + 2;
    if (block == 1) {
        for (int i = 0; i < (taskH-2)*m_width; i++) {
            m_planetes[i] = data[i+m_width];
        }
    } else if (block == nbp -1) {
        for (int i = 0; i < (taskH - 2)*m_width; i++) {
            m_planetes[(m_height - (taskH -2))*m_width + i] = data[i + m_width];
        }
    } else {
        for (int i = 0; i < (taskH-2)*m_width; i++) {
            m_planetes[(block-1)*(taskH-2)*m_width + i] = data[i + m_width];
        }
    }
}

//# ############################################################################################## #

galaxie_renderer::galaxie_renderer(SDL_Window* win)
{
    m_renderer = SDL_CreateRenderer(win, -1, 0);
    IMG_Init(IMG_INIT_JPG);
    m_texture = IMG_LoadTexture(m_renderer, "data/galaxie.jpg");
}
//_ ______________________________________________________________________________________________ _
galaxie_renderer::~galaxie_renderer()
{
    SDL_DestroyTexture(m_texture);
    IMG_Quit();
    SDL_DestroyRenderer(m_renderer);
}
//_ ______________________________________________________________________________________________ _
void 
galaxie_renderer::rend_planete_habitee(int x, int y)
{
    SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 64);// Couleur verte
    SDL_RenderDrawPoint(m_renderer, x, y);
}
//_ ______________________________________________________________________________________________ _
void 
galaxie_renderer::rend_planete_inhabitable(int x, int y)
{
    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 64);// Couleur rouge
    SDL_RenderDrawPoint(m_renderer, x, y);
}
//_ ______________________________________________________________________________________________ _
void 
galaxie_renderer::rend_planete_inhabitee(int x, int y)
{
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);// Couleur noire
    SDL_RenderDrawPoint(m_renderer, x, y);    
}
//_ ______________________________________________________________________________________________ _
void 
galaxie_renderer::render(const galaxie& g)
{
    int i, j;
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
    const char* data   = g.data();
    int   width  = g.width();
    int   height = g.height();

    for (i = 0; i < height; ++i )
        for (j = 0; j < width; ++j )
        {
            if (data[i*width+j] == habitee)
                rend_planete_habitee(j, i);
            if (data[i*width+j] == inhabitable)
                rend_planete_inhabitable(j, i);
        }

    SDL_RenderPresent(m_renderer);
}
