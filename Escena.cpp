#include "../include/Escena.hpp"
#include "include/raylib/raymath.h"
// #include <cstdio> // sprintf
// #include <string> // std::to_string()

// Liberacion de memoria dinamica para evitar exceso de uso/asignacion en la Gestion de escenas de base.cpp
void Escena::DeInit()
{
    delete this;
}

//--------------------------------------------------------------

// Inicializamos punteros necesarios en la funcion de actualizacion
// de fisicas
dWorldID *EscenaJugable::worldAuxCallback = nullptr;
dJointGroupID *EscenaJugable::contactGroupAuxCallback = nullptr;

/// @brief "Trigger" de clic sobre botones
/// @param boton Un boton definido por un rectangulo
/// @return
bool ClicEnBoton(const Rectangle &boton)
{
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        const Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, boton))
            return true;
    }
    return false;
}

//--------------------------------------------------------------

void EscenaInicio::Init()
{
    // Modelo del taxi
    taxi = LoadModel("assets/taxi_base.glb");

    // Boton de inicio
    botones[0] = {
        static_cast<float>(GetScreenWidth() / 2 - ANCHO_BOTON / 2),
        static_cast<float>(GetScreenHeight() / 2 - ALTO_BOTON / 2),
        static_cast<float>(ANCHO_BOTON),
        static_cast<float>(ALTO_BOTON)};
    // Boton de salir
    botones[1] = {
        static_cast<float>(GetScreenWidth() / 2 - ANCHO_BOTON / 2),
        static_cast<float>(GetScreenHeight() / 2 + ALTO_BOTON / 2),
        static_cast<float>(ANCHO_BOTON),
        static_cast<float>(ALTO_BOTON)};

    TraceLog(LOG_INFO, "Cargada escena INICIO");
}
void EscenaInicio::Update(int &indiceEscenaActual)
{
    // Clic en boton de inicio
    if (ClicEnBoton(botones[0]))
    {
        // Vamos a la escena de juego
        indiceEscenaActual = 1;
    }

    // Clic en boton de salir
    if (ClicEnBoton(botones[1]))
    {
        indiceEscenaActual = INDICE_SALIDA;
    }
}
void EscenaInicio::Draw() const
{

    // cam.position = {0.0f, 5.0f, -10.0f};

    Camera camera = {0};
    camera.position = {0.0f, 0.0f, 5.0f};
    camera.target = Vector3Zero();
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    ClearBackground(BLACK);

    BeginMode3D(camera);

    DrawModel(taxi, Vector3Zero(), 1.0f, WHITE);

    EndMode3D();

    // Boton de inicio
    DrawRectangleRec(botones[0], BLUE);
    DrawText("INICIO", botones[0].x + ANCHO_BOTON * 6 / 15, botones[0].y + ALTO_BOTON / 3, 20, WHITE);

    // Boton de salir
    DrawRectangleRec(botones[1], RED);
    DrawText("SALIR", botones[1].x + ANCHO_BOTON * 6 / 15, botones[1].y + ALTO_BOTON / 3, 20, WHITE);
}

void EscenaInicio::DeInit()
{
    TraceLog(LOG_INFO, "Saliendo de la escena de INICIO...");

    UnloadModel(taxi);
    // Liberacion de memoria
    Escena::DeInit();
}

//--------------------------------------------------------------

void EscenaJugable::Init()
{
    // Inicializar el temporizador
    segundosRestantes = 60;

    // Camara
    camera = {0};
    camera.position = {0.0f, 5.0f, -10.0f};
    camOffset = {0.0f, 5.0f, -10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    rotationX = 0.0f;
    rotationY = 0.0f;

    // Carga de modelos
    taxi = LoadModel("assets/taxi_base.glb");
    llantaI = LoadModel("assets/llanta_izq.glb");
    llantaD = LoadModel("assets/llanta_der.glb");

    // Creacion del marcador
    marcador = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    traslacion = MatrixTranslate(4.0f, 0.5f, 0.0f);
    marcador.transform = MatrixMultiply(marcador.transform, traslacion);

    // Inicializacion de ODE (sistema de fisicas)
    dInitODE(); // TODO: es correcta la inicializacion de ODE aqui o deberia
                // colocarse en cada update?
    world = dWorldCreate();
    space = dSimpleSpaceCreate(0);
    contactGroup = dJointGroupCreate(0);

    dWorldSetGravity(world, 0, 0, -9.81);
    dWorldSetAngularDamping(world, 0.15); // TODO: 0.1?

    // Crear la colision del taxi
    ball = dBodyCreate(world);
    dMass m;
    dMassSetSphere(&m, 1, RADIUS);

    dBodySetMass(ball, &m);
    dBodySetPosition(ball, 0, 0, 1);
    ballGeom = dCreateSphere(space, RADIUS);

    dGeomSetBody(ballGeom, ball);

    // Crear el piso
    ground = dCreatePlane(space, 0, 0, 1, 0);

    worldAuxCallback = &world;
    contactGroupAuxCallback = &contactGroup;
}

void EscenaJugable::Update(int &indiceEscenaActual)
{
    const dReal timeStep = 0.01;

    // Agregar fuerza a bolita
    // Definir rotacion de las llantas y a  aplicar al taxi
    float rotationStep = 0;
    float wheelRotation = 0;
    if (IsKeyDown(KEY_D))
    {
        wheelRotation = -0.5;
        rotationStep = -0.04;
    }
    else if (IsKeyDown(KEY_A))
    {
        wheelRotation = 0.5;
        rotationStep = 0.04;
    }
    // Aplicar rotacion a llantas
    Matrix llantaIRotationM = MatrixRotateY(wheelRotation);
    llantaI.transform = MatrixMultiply(taxi.transform, llantaIRotationM);
    llantaD.transform = llantaI.transform;
    Vector3 forwardVector = {0.0f, 0.0f, 1.0f};
    // Vector3 wheelLPos = {0.36f, -0.28f, 0.78f};
    //  Vector3 wheelRPos = {-0.36f, -0.28f, 0.78f};
    // Transform forward vector using model matrix
    Vector3 transformedForwardVector = Vector3Transform(forwardVector, taxi.transform);
    //   wheelLPos = Vector3Transform(wheelLPos, taxi.transform);
    //    wheelRPos = Vector3Transform(wheelRPos, taxi.transform);

    Matrix taxiRotation;
    Vector3 accelerationVector = Vector3Zero();
    if (IsKeyDown(KEY_W))
    {
        taxiRotation = MatrixRotateY(rotationStep);
        taxi.transform = MatrixMultiply(taxi.transform, taxiRotation);
        accelerationVector = Vector3Scale(transformedForwardVector, VELOCIDAD_TAXI);
        dBodyAddForce(ball, accelerationVector.x, accelerationVector.z, accelerationVector.y);
    }
    if (IsKeyDown(KEY_S))
    { // Reversa
        taxiRotation = MatrixRotateY(-rotationStep);
        taxi.transform = MatrixMultiply(taxi.transform, taxiRotation);
        accelerationVector = Vector3Scale(transformedForwardVector, VELOCIDAD_TAXI);
        dBodyAddForce(ball, -accelerationVector.x, -accelerationVector.z, -accelerationVector.y);
    }

    // Actualizacion del sistema de fisica
    dSpaceCollide(space, 0, &EscenaJugable::nearCallback);
    dWorldStep(world, timeStep);
    dJointGroupEmpty(contactGroup);

    const dReal *pos = dBodyGetPosition(ball);
    Vector3 correctedPos = {(float)pos[0], (float)pos[2], (float)pos[1]};
    // camera.position = Vector3Add(correctedPos, {0, 10, 10});
    // camera.target = correctedPos;
    static Vector3 posRelativa = camOffset;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        GiraCamara(camera, rotationX, rotationY, correctedPos, taxi, camOffset, posRelativa);
    }
    else
    {
        camera.position = Vector3Add(correctedPos, posRelativa);
        camera.target = correctedPos;
    }
    // Limitamos la altura de la camara para no atravesar el suelo
    camera.position.y = Clamp(camera.position.y, 1.5f, 10.0f);

    // Colision con el marcador cambia su posicion
    BoundingBox marcadorbb = GetModelBoundingBox(marcador);
    bool enMarcador = CheckCollisionBoxSphere(marcadorbb, correctedPos, 1.0f);
    if (enMarcador && TaxiEstaQuieto(ball))
    {
        traslacion = MatrixTranslate(GetRandomValue(-10, 10), 0.0f, GetRandomValue(-10, 10));
        marcador.transform = MatrixMultiply(marcador.transform, traslacion);
        segundosRestantes += TIEMPO_ADICIONAL;
    }

    // Gestion del timer de la escena
    GestionaTimer();

    // Al terminar el tiempo, cambiamos a la escena de derrota
    if (segundosRestantes < 0)
        indiceEscenaActual = 50;
}

void EscenaJugable::Draw() const
{
    const dReal *pos = dBodyGetPosition(ball);
    const Vector3 correctedPos = {(float)pos[0], (float)pos[2], (float)pos[1]};
    Vector3 wheelLPos = {0.36f, -0.28f, 0.78f};
    Vector3 wheelRPos = {-0.36f, -0.28f, 0.78f};
    wheelLPos = Vector3Transform(wheelLPos, taxi.transform);
    wheelRPos = Vector3Transform(wheelRPos, taxi.transform);
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);
    // Dibuja el taxi
    DrawModel(taxi, Vector3Add(correctedPos, {0, -0.5, 0}), 1.0f, WHITE);
    DrawModel(llantaI, Vector3Add(correctedPos, wheelLPos), 1.0f, WHITE);
    DrawModel(llantaD, Vector3Add(correctedPos, wheelRPos), 1.0f, WHITE);

    DrawSphereWires(correctedPos, 1.0f, 16, 16, LIME);

    //   DrawLine3D(correctedPos, Vector3Add(correctedPos, Vector3Scale(transformedForwardVector,3.0f)), RED);

    // Dibuja marcador(objeto a recoger)
    DrawModel(marcador, Vector3Zero(), 1, GOLD);

    DrawGrid(100, 1.0f); // Draw a grid
    // DrawModel(marcador, Vector3Zero(), 1, RED);
    BoundingBox marcadorbb = GetModelBoundingBox(marcador);
    DrawBoundingBox(marcadorbb, PURPLE);
    EndMode3D();

    DrawFPS(10, 10);
    DibujaTiempoRestante();
}
void EscenaJugable::DeInit()
{
    worldAuxCallback = nullptr;
    contactGroupAuxCallback = nullptr;

    dJointGroupDestroy(contactGroup);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();

    UnloadModel(taxi);
    UnloadModel(llantaI);
    UnloadModel(llantaD);
    UnloadModel(marcador);

    Escena::DeInit();
}

void EscenaJugable::nearCallback(void *data, dGeomID o1, dGeomID o2)
{
    const int MAX_CONTACTS = 10;
    dContact contact[MAX_CONTACTS];

    int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact));

    for (int i = 0; i < numc; i++)
    {
        contact[i].surface.mode = dContactBounce;
        contact[i].surface.mu = dInfinity;
        contact[i].surface.bounce = 0.7;
        contact[i].surface.bounce_vel = 0.1;
        dJointID c = dJointCreateContact(*worldAuxCallback, *contactGroupAuxCallback, &contact[i]);
        dJointAttach(c, dGeomGetBody(o1), dGeomGetBody(o2));
    }
}

bool EscenaJugable::TaxiEstaQuieto(const dBodyID &taxiBody) const
{
    const dReal *fuerza = dBodyGetLinearVel(taxiBody);
    // Por alguna razon, ODE no permite una velocidad de cero absoluto
    // por lo que definimos un valor pequenio como velocidad cero
    const dReal VEL_CERO = 0.8;
    if ((fuerza[0] < VEL_CERO && fuerza[0] > -VEL_CERO) && (fuerza[1] < VEL_CERO && fuerza[1] > -VEL_CERO))
        return true;

    return false;
}

void EscenaJugable::GiraCamara(Camera &camera, float &rotationX, float &rotationY, const Vector3 &posTaxi, const Model &taxi, const Vector3 &camOffset, Vector3 &posRelativa)
{
    Vector2 delta = GetMouseDelta();
    const float SENSIBILIDAD = 0.2f;

    Vector2 mousePosition = GetMousePosition();

    rotationY -= delta.x * SENSIBILIDAD * 0.01f; // Rotacion en el eje y
    rotationX += delta.y * SENSIBILIDAD * 0.01f; // Rotacion en el eje x

    // Calcula la posicion de la camara utilizando la matriz de rotacion del taxi
    Matrix cameraMatrix = MatrixMultiply(MatrixRotateY(rotationY), MatrixRotateX(rotationX));
    Vector3 cameraPosition = Vector3Transform(camOffset, MatrixMultiply(taxi.transform, cameraMatrix));
    camera.position = Vector3Add(posTaxi, cameraPosition);
    // Limitamos la altura de la camara para no atravesar el suelo
    camera.position.y = Clamp(camera.position.y, 0.5f, 10.0f);

    camera.target = posTaxi;

    // Guardamos la posicion relativa para que mientras no se presiona el boton del mouse, siga a cierta distancia
    // del taxi
    posRelativa = cameraPosition;
}

void EscenaJugable::GestionaTimer()
{
    static int framesCounter = 0;

    // 60 frames restan un segundo
    if (framesCounter % 60 == 0)
        segundosRestantes--;

    framesCounter++;
}

void EscenaJugable::DibujaTiempoRestante(int segundosParaAmarillo, int segundosParaRojo) const
{
    char buff[20] = {};

    const int posX = GetScreenWidth() / 2 - 20;

    // Dibuja el texto de color dependiendo del tiempo restante
    sprintf(buff, "%d", segundosRestantes);
    DrawText(buff, posX, 0, 50, GREEN);
    if (segundosRestantes <= segundosParaAmarillo)
        DrawText(buff, posX, 0, 50, YELLOW);
    if (segundosRestantes <= segundosParaRojo)
        DrawText(buff, posX, 0, 50, RED);
}

//--------------------------------------------------------------

void EscenaDerrota::Init()
{
    botonReiniciar = {
        static_cast<float>(GetScreenWidth() / 2 - ANCHO_BOTON - 100),
        static_cast<float>(GetScreenHeight() / 2 - ANCHO_BOTON / 2),
        static_cast<float>(ANCHO_BOTON),
        static_cast<float>(ANCHO_BOTON / 2)};

    botonSalir = {
        static_cast<float>(GetScreenWidth() / 2 + ANCHO_BOTON / 2),
        static_cast<float>(GetScreenHeight() / 2 - ANCHO_BOTON / 2),
        static_cast<float>(ANCHO_BOTON),
        static_cast<float>(ANCHO_BOTON / 2)};
    TraceLog(LOG_INFO, "Escena DERROTA CARGADA");
}

void EscenaDerrota::Update(int &indiceEscenaActual)
{
    // Reinicio de juego
    if (ClicEnBoton(botonReiniciar))
    {
        indiceEscenaActual = 0;
    }

    // Salida del juego
    if (ClicEnBoton(botonSalir))
    {
        indiceEscenaActual = INDICE_SALIDA;
    }
}
void EscenaDerrota::Draw() const
{
    ClearBackground(BLACK);

    DrawText("Fin del juego", GetScreenWidth() / 2 - 100, 0, 32, SKYBLUE);

    // Boton de REINICIAR
    DrawRectangleRec(botonReiniciar, GREEN);
    DrawText("REINICIAR JUEGO", botonReiniciar.x + 10.0f, botonReiniciar.y + ANCHO_BOTON / 4, 20, WHITE);

    // Boton de SALIR
    DrawRectangleRec(botonSalir, RED);
    DrawText("SALIR", botonSalir.x + ANCHO_BOTON * 5 / 15, botonSalir.y + ANCHO_BOTON / 4, 20, WHITE);
}
void EscenaDerrota::DeInit()
{
    TraceLog(LOG_INFO, "Escena VICTORIA LIBERADA");
    Escena::DeInit();
}