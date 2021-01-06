#include <cstdlib>
#include <string>
#include <iostream>
#include <SDL.h>        
#include <SDL_image.h>
#include <fstream>
#include <ctime>
#include <iomanip>      // std::setw
#include <chrono>
#include <mpi.h>
#include "parametres.hpp"
#include "galaxie.hpp"
 
int main(int argc, char ** argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm globComm;
    MPI_Comm_dup(MPI_COMM_WORLD, &globComm);
    int nbp;
    MPI_Comm_size(globComm, &nbp);
    int rank;
    MPI_Comm_rank(globComm, &rank);

    char commentaire[4096];
    int width, height;
    SDL_Event event;
    SDL_Window   * window;

    parametres param;


    std::ifstream fich("parametre.txt");
    fich >> width;
    fich.getline(commentaire, 4096);
    fich >> height;
    fich.getline(commentaire, 4096);
    fich >> param.apparition_civ;
    fich.getline(commentaire, 4096);
    fich >> param.disparition;
    fich.getline(commentaire, 4096);
    fich >> param.expansion;
    fich.getline(commentaire, 4096);
    fich >> param.inhabitable;
    fich.getline(commentaire, 4096);
    fich.close();

    int deltaT = (20*52840)/width;

    if (rank == 0) {
      

      std::cout << "Resume des parametres (proba par pas de temps): " << std::endl;
      std::cout << "\t Chance apparition civilisation techno : " << param.apparition_civ << std::endl;
      std::cout << "\t Chance disparition civilisation techno: " << param.disparition << std::endl;
      std::cout << "\t Chance expansion : " << param.expansion << std::endl;
      std::cout << "\t Chance inhabitable : " << param.inhabitable << std::endl;
      std::cout << "Proba minimale prise en compte : " << 1./RAND_MAX << std::endl;
      std::srand(std::time(nullptr));

      SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

      window = SDL_CreateWindow("Galaxie", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                width, height, SDL_WINDOW_SHOWN);

     
      std::cout << "Pas de temps : " << deltaT << " années" << std::endl;
      std::cout << std::endl; 
    } 

    galaxie_renderer gr(window);
    galaxie g(width, height, param.apparition_civ);
    galaxie g_next(width, height);

    std::vector<char> g_nextData;
    g_nextData.resize(height*width);

    unsigned long long temps = 0;


    std::chrono::time_point<std::chrono::system_clock> start, end;
    while (1) {
        
        if (rank == 0) 
        {
          MPI_Status status;
          start = std::chrono::system_clock::now();
          gr.render(g);
          std::cout << "En attente de réception..." << std::endl;
          MPI_Recv(&g_nextData[0], g_nextData.size(), MPI_CHAR, 1, 0, globComm, &status);
          std::cout << "Reçu! " << std::endl;
          g.updateWithData(g_nextData);
          end = std::chrono::system_clock::now();

          std::chrono::duration<double> elaps = end - start;
          
          temps += deltaT;
          std::cout << "Temps passe : "
                    << std::setw(10) << temps << " années" 
                    << std::fixed << std::setprecision(3)
                    << "  " << "|  CPU(ms) :  " << elaps.count()*1000
                    << "\r" << std::flush;
          //_sleep(1000);
          if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            std::cout << std::endl << "The end" << std::endl;
            break;
          }
        } // (if)
          else 
        {
          mise_a_jour(param, width, height, g.data(), g_next.data());
          for (int i = 0; i < width*height; i++) {
            g_nextData[i] = g_next.data()[i];
            std::cout << g_nextData[i] << std::endl;
          }
          std::cout << "On va envoyer..." << std::endl;
          MPI_Ssend(&g_nextData[0], g_nextData.size(), MPI_CHAR, 0, 0, globComm);
          std::cout << "Contenu envoyé!" << std::endl;
        } // (else)

    } // (while)

    if (rank == 0) {
      SDL_DestroyWindow(window);
      SDL_Quit();
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
