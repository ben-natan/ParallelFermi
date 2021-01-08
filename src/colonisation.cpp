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
    // On coupe horizontalement
    int taskH = (height/ (nbp-1)) +2;

    
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


      galaxie g(width, height, param.apparition_civ);
      galaxie g_next(width, height);
      galaxie_renderer gr(window);

      std::vector<char> g_nextData;
      g_nextData.resize(height*width);

      std::vector<char> g_nextBuffer;
      g_nextBuffer.resize(taskH*width);

      unsigned long long temps = 0;

      std::chrono::time_point<std::chrono::system_clock> start, end;
      while (1) 
      {
          MPI_Status status;
          start = std::chrono::system_clock::now();
          gr.render(g);
          for (int i=1; i<nbp; i++) {
            MPI_Recv(g_nextBuffer.data(), g_nextBuffer.size(), MPI_CHAR, i, 0, globComm, &status);
            g.updateByHBlocks(g_nextBuffer, i, nbp);
          }
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
      } //(while)

      SDL_DestroyWindow(window);
      SDL_Quit();

      MPI_Finalize();
      return EXIT_SUCCESS;

    } //(rank 0)
    else 
    {
        galaxie g(width, taskH, param.apparition_civ);
        galaxie g_next(width, taskH);

        std::vector<char> g_nextData;
        g_nextData.resize(taskH*width);
    
        while (1) {
          mise_a_jour(param, width, taskH, g.data(), g_next.data());

          MPI_Status status;
          // ATTENTION AU DEADLOCK ( évités avec le % 2 )
          if (rank == 1) {
            MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, 2, 0, globComm); // au rank 2 
            MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, 2, 0, globComm, &status);

            g_next.replaceLine(g_nextData,taskH - 2);
          } else if (rank == nbp-1) {
            if (rank % 2 == 1) {
              MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, nbp-2, 0, globComm, &status);
              MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, nbp-2, 0, globComm); // au rank nbp-2
            } else {
              MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, nbp-2, 0, globComm); // au rank nbp-2
              MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, nbp-2, 0, globComm, &status);
            }

            g_next.replaceLine(g_nextData, 1);
          } else {
            if (rank % 2 == 1) {
              MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, rank-1, 0, globComm, &status); // rank n - 1
              MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, rank+1, 0, globComm); // rank n + 1
              MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, rank+1, 0, globComm, &status); // rank n + 1
              MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, rank-1, 0, globComm); // rank n - 1
            } else {
              MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, rank+1, 0, globComm); // rank n + 1
              MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, rank-1, 0, globComm, &status); // rank n - 1
              MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, rank-1, 0, globComm); // rank n - 1
              MPI_Recv(g_nextData.data(), g_nextData.size(), MPI_CHAR, rank+1, 0, globComm, &status); // rank n + 1
            }

            g_next.replaceLine(g_nextData, taskH -2);
            g_next.replaceLine(g_nextData, 1);
          }

          MPI_Send(g_next.data(), g_nextData.size(), MPI_CHAR, 0, 0, globComm);
          // MPI_Gather(g_next.data(), g_nextData.size(), g_nextData.data(), width*height, MPI_CHAR, 0, globComm) // Mettre aussi le gather dans le rank 0
          g.swap(g_next);
        }

        MPI_Finalize();
        return EXIT_SUCCESS;

    } //(autres rank)
}
