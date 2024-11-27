//#include "include/raylib/raylib.h"
//#include "include/raylib/raymath.h"
//#include "include/ode/ode.h"
#include "include/Escena.hpp"

// TODO: Hacer que el punto de recoleccion varie mas su distancia de respawn
// TODO: Limpieza de codigo
// TODO: agregar los obstaculos

/*
Para compilar checa de nuevo LEEME.TXT
*/


void GestionaEscena(Escena *&escenaActual, int &indiceEscenaActual, int &indiceEscenaPasada,const int& INDICE_SALIDA = -1)
{
    if(!escenaActual)
    return;
    
    // Actualizacion de la escena segun su tipo (indice)
    escenaActual->Update(indiceEscenaActual);

    // En caso de cambio de escena
    if (indiceEscenaActual != indiceEscenaPasada && indiceEscenaActual != INDICE_SALIDA)
    {
        // Desinicializaciones necesarias
        escenaActual->DeInit();
        escenaActual = nullptr;

        if(indiceEscenaActual == INDICE_SALIDA)
        return;
        // Escena correspondiente
        switch (indiceEscenaActual)
        {
        case 0: // Escena de inicio
            escenaActual = new EscenaInicio();
            break;
        case 1: // Cambiamos a escena JUGABLE
            escenaActual = new EscenaJugable();
            break;
        case 50: // Escena de derrota
            escenaActual = new EscenaDerrota();
        }

        // Inicializacion
        escenaActual->Init();
        
        // Actualizacion de indice
        indiceEscenaPasada = indiceEscenaActual;
    }
}


//------------------------------------------------------------------------------------
// Inicio del programa
//------------------------------------------------------------------------------------
int main(void)
{
    // Inicializacion
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "TAXI? si gracias");

    
    SetWindowMonitor(1);
    
   
    SetTargetFPS(60); // Hacemos que el juego corra a 60 fps (idealmente)
    
    // Inicializacion del sistema de escenas
    int indiceEscenaActual = 0;
 
    int indiceEscenaPasada = indiceEscenaActual;
    Escena *escenaActual = new EscenaInicio();
 
    escenaActual->Init();
    
    
    // Loop principal
    while (!WindowShouldClose() && indiceEscenaActual != Escena::INDICE_SALIDA) // Detectar si se cierra la ventana o se presiona ESC
    {
        // Update
        //----------------------------------------------------------------------------------

        GestionaEscena(escenaActual, indiceEscenaActual, indiceEscenaPasada, Escena::INDICE_SALIDA);


        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        escenaActual->Draw();        

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Inicializacion
    //--------------------------------------------------------------------------------------
   
    /*
    NOTA: a pesar de que existe el puntero a Escena (el manejador de escenas), no es necesario
    usar delete con este puntero, ya que lo usamos dentro de la funcion DeInit de las clases derivadas
    */
   TraceLog(LOG_INFO, "sali del WHILE");
   
   
    escenaActual->DeInit();

    CloseWindow(); // Cerrar ventana y conteexto OpenGL
    //--------------------------------------------------------------------------------------

    return 0;
}
