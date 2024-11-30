#include "../include/Escena.hpp"
#include "include/raylib/raymath.h"

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
    // Gira el taxi durante el menu
    taxi.transform = MatrixMultiply(taxi.transform, MatrixRotateY(0.4f * GetFrameTime()));
}
void EscenaInicio::Draw() const
{
    // Camara 3D para ver el modelo al inicio del juego
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
    segundosRestantes = 61;

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
    posRelativa = camOffset;

    // Carga de modelos
    taxi = LoadModel("assets/taxi_base.glb");
    llantaI = LoadModel("assets/llanta_izq.glb");
    llantaD = LoadModel("assets/llanta_der.glb");

    // Creacion del marcador
    marcador = LoadModelFromMesh(GenMeshCube(2.0f, 5.0f, 2.0f));
    traslacion = MatrixTranslate(4.0f, 0.5f, 0.0f);
    marcador.transform = MatrixMultiply(marcador.transform, traslacion);

    // Inicializacion de ODE (sistema de fisicas)
    dInitODE();
    world = dWorldCreate();
    space = dSimpleSpaceCreate(0);
    contactGroup = dJointGroupCreate(0);

    dWorldSetGravity(world, 0, 0, -9.81);
    dWorldSetAngularDamping(world, 0.15);

    // Crear la colision del taxi
    ball = dBodyCreate(world);
    dMass m;
    dMassSetSphere(&m, 1, RADIUS);

    dBodySetMass(ball, &m);
    dBodySetPosition(ball, 0, 0, .7); // Posicion inicial del taxi
    ballGeom = dCreateSphere(space, RADIUS);

    dGeomSetBody(ballGeom, ball);

    // Crear el piso
    ground = dCreatePlane(space, 0, 0, 1, 0);

    // Creacion de los obstaculos
    const int CANTIDAD_DE_OBSTACULOS = 80; // Modificable
    Vector3 posicionesObstaculos[CANTIDAD_DE_OBSTACULOS];
    Vector3 escalaObstaculos[CANTIDAD_DE_OBSTACULOS];
    for (size_t i = 0; i < CANTIDAD_DE_OBSTACULOS; i++)
    {
        posicionesObstaculos[i] = {generaNumeroAleatorio(), generaNumeroAleatorio(), 1.0f};
        escalaObstaculos[i] = {(float)GetRandomValue(1, 6), (float)GetRandomValue(1, 6), (float)GetRandomValue(1, 2)};
    }
    // Ajustamos el tamanio de los std::vector para que no haya errores de indice invalido
    obstaculosBody.resize(CANTIDAD_DE_OBSTACULOS);
    obstaculosGeom.resize(CANTIDAD_DE_OBSTACULOS);
    for (size_t i = 0; i < CANTIDAD_DE_OBSTACULOS; i++)
        crearObstaculo(obstaculosBody[i], obstaculosGeom[i], posicionesObstaculos[i], escalaObstaculos[i], world, space);

    
    // Guardamos punteros a variables de mundo (responsables de fisicas)
    // Esto por el metodo nearCallback que no acepta mas de 3 parametros
    worldAuxCallback = &world;
    contactGroupAuxCallback = &contactGroup;
}

void EscenaJugable::Update(int &indiceEscenaActual)
{
    const dReal timeStep = 0.01;

    // Input y movimiento
    Movimiento();

    // Actualizacion del sistema de fisica
    dSpaceCollide(space, 0, &EscenaJugable::nearCallback);
    dWorldStep(world, timeStep);
    dJointGroupEmpty(contactGroup);

    const dReal *pos = dBodyGetPosition(ball);
    Vector3 correctedPos = {(float)pos[0], (float)pos[2], (float)pos[1]};

    // Actualizacion de la posicion de la camara
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
        traslacion = MatrixTranslate((float)GetRandomValue(-50, 50), 0.0f, (float)GetRandomValue(-50, 50));
        marcador.transform = MatrixMultiply(marcador.transform, traslacion);

        // Mantenemos el marcador dentro de un rango
        // El mismo rango que el grid
        while (marcador.transform.m12 > 50 || marcador.transform.m12 < -50)
            marcador.transform.m12 = (float)GetRandomValue(-50, 50);
        while (marcador.transform.m14 > 50 || marcador.transform.m14 < -50)
            marcador.transform.m14 = (float)GetRandomValue(-50, 50);

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

    ClearBackground(BLACK);

    BeginMode3D(camera);
    // Dibuja el taxi
    DrawModel(taxi, Vector3Add(correctedPos, {0, -0.5, 0}), 1.0f, WHITE);
    DrawModel(llantaI, Vector3Add(correctedPos, wheelLPos), 1.0f, WHITE);
    DrawModel(llantaD, Vector3Add(correctedPos, wheelRPos), 1.0f, WHITE);

    // Dibuja marcador(objeto a recoger)
    DrawModel(marcador, Vector3Zero(), 1, GOLD);
    BoundingBox marcadorbb = GetModelBoundingBox(marcador);
    DrawBoundingBox(marcadorbb, PURPLE);

    DrawGrid(100, 1.0f); // Draw a grid

    // Dibuja todos los obstaculos definidos en Init()
    dibujaObstaculos(obstaculosBody, obstaculosGeom);

    // Suelo
    DrawPlane(Vector3Zero(), {50.0f, 50.0f}, GRAY);

    EndMode3D();

    DrawFPS(10, 10);
    DibujaTiempoRestante();
}
void EscenaJugable::DeInit()
{
    // Liberamos los punteros necesarios para el nearCallback
    worldAuxCallback = nullptr;
    contactGroupAuxCallback = nullptr;

    // Liberaciones de ODE y modelos
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
    const int MAX_CONTACTS = 50;
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

void EscenaJugable::crearObstaculo(dBodyID &obsBody, dGeomID &obsGeom, const Vector3 &pos, const Vector3 &escala, dWorldID &world, dSpaceID &space)
{
    obsBody = dBodyCreate(world);
    dMass m;
    dMassSetBox(&m, 1, escala.x, escala.y, escala.z);

    dBodySetMass(obsBody, &m);
    dBodySetPosition(obsBody, pos.x, pos.y, pos.z);
    obsGeom = dCreateBox(space, escala.x, escala.y, escala.z);

    dGeomSetBody(obsGeom, obsBody);
}

void EscenaJugable::dibujaObstaculos(const std::vector<dBodyID> &obstBody, const std::vector<dGeomID> &obstGeom)
{
    for (size_t i = 0; i < obstBody.size(); i++)
    {
        const dReal *pos = dBodyGetPosition(obstBody[i]);
        Vector3 posV = {(float)pos[0], (float)pos[2], (float)pos[1]};
        dVector3 escala;
        dGeomBoxGetLengths(obstGeom[i], escala);
        DrawCubeV(posV, {(float)escala[0], (float)escala[2], (float)escala[1]}, RED);
        DrawCubeWiresV(posV, {(float)escala[0], (float)escala[2], (float)escala[1]}, BLACK);
    }
}

/// @brief Genera un numero aleatorio en un rango, excepto por el numero 0 (se usa para la posicion de los obstaculos)
/// @return 
float EscenaJugable::generaNumeroAleatorio(){
    float num = 0.0f;
    do{
        num = (float)GetRandomValue(-50, 50);
    }while(num == 0.0f);
    return num;
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
    /*
    Version creada por mi, pero no es comoda
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

    // Guardamos la posicion relativa para que siga a cierta distancia
    // del taxi mientras no se presione el boton del mouse
    posRelativa = cameraPosition;
    */

   // Version asistida por chatgpt (bastante mejor que la anterior)

   Vector2 delta = GetMouseDelta();
    const float SENSIBILIDAD = 0.2f;
    const float ROTACION_X_MAX = 80.0f; // Limite en el eje X para evitar que la camara se voltee

    // Rotación de la camara basada en el movimiento del raton
    rotationY -= delta.x * SENSIBILIDAD; // Rotación alrededor del eje Y (horizontal)
    rotationX += delta.y * SENSIBILIDAD; // Rotación alrededor del eje X (vertical)

    // Limitar la rotacion en el eje X para evitar que la camara se voltee
    rotationX = Clamp(rotationX, -ROTACION_X_MAX, ROTACION_X_MAX);

    // Calcular la posicion de la camara usando coordenadas esfericas (polares)
    float distance = Vector3Length(camOffset);

    // Calcula la nueva posicion de la cámara
    float cameraX = posTaxi.x + distance * sinf(DEG2RAD * rotationY) * cosf(DEG2RAD * rotationX);
    float cameraY = posTaxi.y + distance * sinf(DEG2RAD * rotationX);
    float cameraZ = posTaxi.z + distance * cosf(DEG2RAD * rotationY) * cosf(DEG2RAD * rotationX);

    // Asignamos la nueva posicion de la camara
    camera.position = {cameraX, cameraY, cameraZ};

    // Establecer el objetivo de la camara como el taxi
    camera.target = posTaxi;

    // Actualizar la posicion relativa de la camara
    posRelativa = Vector3Subtract(camera.position, posTaxi);
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

void EscenaJugable::Movimiento()
{
    // En caso de que el taxi salga disparado hacia arriba, no podra acelerar
    // (esto ya que acelerar en el aire genera un movimiento mucho mas rapido que en el suelo, ademas no es realista)
    if (dBodyGetPosition(ball)[2] > RADIUS + 0.05)
        return;

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
    const Vector3 forwardVector = {0.0f, 0.0f, 1.0f};

    // Transform forward vector using model matrix
    Vector3 transformedForwardVector = Vector3Transform(forwardVector, taxi.transform);

    // Aplica fuerzas (simula movimiento)
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