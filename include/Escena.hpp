#pragma once

#ifndef ESCENA_H
#define ESCENA_H

#include "include/raylib/raylib.h"
#include "include/ode/ode.h"

// Clase abstracta
class Escena
{
public:
    virtual void Init() = 0;
    virtual void Update(int &) = 0;
    virtual void Draw() const = 0;
    virtual void DeInit(); // Incluye liberacion de memoria
    static const int INDICE_SALIDA = -1;
};

static bool ClicEnBoton(const Rectangle &);

// Clases derivadas ----------------------------------------------

// La escena del menu principal
class EscenaInicio : public Escena
{
public:
    void Init();
    void Update(int &);
    void Draw() const;
    void DeInit();

private:
    const int ANCHO_BOTON = 300;
    const int ALTO_BOTON = 50;

    // El sprite que se muestra en pantalla
    Model taxi;
    // [0] es boton inicio, [1] es boton salir
    Rectangle botones[2];
};

//--------------------------------------------------------------------------
class EscenaJugable : public Escena
{
public:
    void Init();
    void Update(int &);
    void Draw() const;
    void DeInit();

private:
    // Variables de fisica
    dWorldID world;
    dSpaceID space;
    dBodyID ball;
    dGeomID ballGeom;
    dMass m;
    dJointGroupID contactGroup;
    const dReal RADIUS = 0.5;
    dGeomID ground;

    // Auxiliares de nearCallback(colisiones)
    static dWorldID *worldAuxCallback;
    static dJointGroupID *contactGroupAuxCallback;

    // Variables de game loop

    // Inicializar siempre mayor a cero
    int segundosRestantes;

    // Tiempo a agregar al tocar un punto de recoleccion
    const int TIEMPO_ADICIONAL = 5;

    const float VELOCIDAD_TAXI = 60.0f;
    Camera camera;
    // La distancia a la que la camara esta del taxi siempre
    Vector3 camOffset;
    // Rotacion de la camara relativa al taxi (en ambos x,y)
    float rotationX;
    float rotationY;
    // Matriz encargada de desplazar el punto de recoleccion
    Matrix traslacion;

    // Assets
    Model taxi;
    Model llantaI;
    Model llantaD;
    // Punto de recoleccion
    Model marcador;

    bool TaxiEstaQuieto(const dBodyID &taxiBody) const;
    void GiraCamara(Camera &camera, float &rotationX, float &rotationY, const Vector3 &posTaxi, const Model &taxi, const Vector3 &camOffset, Vector3 &posRelativa);
    void GestionaTimer();
    void DibujaTiempoRestante(int segundosParaAmarillo = 30, int segundosParaRojo = 10) const;
    void Movimiento();

    // Funcion de fisicas (colisiones)
    static void nearCallback(void *data, dGeomID o1, dGeomID o2);
};

class EscenaDerrota : public Escena
{
public:
    void Init();
    void Update(int &);
    void Draw() const;
    void DeInit();

private:
    Rectangle botonReiniciar;
    Rectangle botonSalir;
    const int ANCHO_BOTON = 200;
};

#endif // !ESCENA_H