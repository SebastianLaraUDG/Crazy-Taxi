#include "include/raylib/raylib.h"
#include "include/raylib/raymath.h"
#include "include/ode/ode.h"

#include <cstdio> // sprintf
#include <string> // std::to_strinng()
// TODO: Hacer que el punto de recoleccion varie mas su distancia de respawn

static dWorldID world;
static dSpaceID space;
static dBodyID ball;
static dGeomID ballGeom;
static dMass m;
static dJointGroupID contactGroup;
const dReal RADIUS = 0.5;

/// @brief Convierte una posicion ODE a un Vector3 de Raylib (falta testearse)
/// @param pos Posicion en coordenadas ODE (comunmente la posicion de un dBodyID)
/// @return La posicion en coordenadas del sistema de Raylib
static Vector3 ODE_TO_RAYLIB_V3(const dReal *pos)
{
    Vector3 res = {};
    res.x = (float)pos[0];
    res.y = (float)pos[2];
    res.z = (float)pos[1];
    return res;
}

/// @brief Convierte una posicion de Raylib a posicion de ODE (falta testear)
/// @param pos Una posicion en sistema de Raylib
/// @return El equivalente en sistema de coordenadas de ODE
static dReal *RAYLIB_TO_ODE_V3(const Vector3 &pos)
{
    dReal *res = new dReal[3];
    res[0] = pos.x;
    res[1] = pos.z;
    res[2] = pos.y;
    return res;
}

static void crearBolita()
{
    ball = dBodyCreate(world);
    dMass m;
    dMassSetSphere(&m, 1, RADIUS);
    dBodySetMass(ball, &m);
    dBodySetPosition(ball, 0, 0, 2);

    ballGeom = dCreateSphere(space, RADIUS);
    dGeomSetBody(ballGeom, ball);
}

static void crearRampa(dBodyID &outRampaBodyID, dGeomID &outRampaGeomID, const Vector3 &pos, const float &lengthX, const float &lengthY, const float &lengthZ)
{
    outRampaBodyID = dBodyCreate(world);
    dMass m;
    dMassSetBox(&m, 1, lengthX, lengthY, lengthZ);
    dBodySetMass(outRampaBodyID, &m);
    const dReal *posOrigen = RAYLIB_TO_ODE_V3(pos);
    // dBodySetPosition(outRampaBodyID, 0, 0, 0.5);

    dBodySetPosition(outRampaBodyID, posOrigen[0], posOrigen[1], posOrigen[2]);
    delete[] posOrigen;

    outRampaGeomID = dCreateBox(space, lengthX, lengthY, lengthZ);

//dBodySetMass(outRampaBodyID,&m);
    dGeomSetBody(outRampaGeomID, outRampaBodyID);
    dBodyDisable(outRampaBodyID);
}

static void simulaRotacion(Model &modelo, dGeomID &geomID)
{
    Vector3 pos = {10, 0, 10};

    // Obtener rotacion actual de la geometria
    const dReal *rampRotP = dGeomGetRotation(geomID);
    Matrix rotacion = {(float)rampRotP[0], (float)rampRotP[4], (float)rampRotP[8], 1.0f,
                       (float)rampRotP[1], (float)rampRotP[5], (float)rampRotP[9], 1.0f,
                       (float)rampRotP[2], (float)rampRotP[6], (float)rampRotP[10], 0.0f,
                       (float)rampRotP[3], (float)rampRotP[7], (float)rampRotP[11], 1.0f};

    modelo.transform = rotacion;

    dMatrix3 nuevaRot;
    static int n = 0;
    // dRSetIdentity(nuevaRot);
    dRFromAxisAndAngle(nuevaRot, 0, 1, 0, PI + n++ / 20);

    dGeomSetRotation(geomID, nuevaRot);
    DrawModel(modelo, pos, 1.0f, GREEN);
}

/// @brief Dibuja posicion, matriz de rotacion y otros (solo para DEBUG)
/// @param pos En coordenadas ODE
/// @param rotacion En sistema raylib
static void dibujaInfoTaxi(const dReal *pos, const Matrix &rotacion)
{
    char buff[20] = {};
    // Posicion
    sprintf(buff, "(%.0f,%.0f,%.0f)", (float)pos[0], (float)pos[1], (float)pos[2]);
    DrawText(buff, GetScreenWidth() - 200, 0, 20, BLACK);

    // Rotacion
    sprintf(buff, "(%.0f,%.0f,%.0f,%.0f)", rotacion.m0, rotacion.m4, rotacion.m8, rotacion.m12);
    DrawText(buff, GetScreenWidth() - 200, 100, 20, BLUE);

    sprintf(buff, "(%.0f,%.0f,%.0f,%.0f)", rotacion.m1, rotacion.m5, rotacion.m9, rotacion.m13);
    DrawText(buff, GetScreenWidth() - 200, 200, 20, BLUE);

    sprintf(buff, "(%.0f,%.0f,%.0f,%.0f)", rotacion.m2, rotacion.m6, rotacion.m10, rotacion.m14);
    DrawText(buff, GetScreenWidth() - 200, 300, 20, BLUE);

    sprintf(buff, "(%.0f,%.0f,%.0f,%.0f)", rotacion.m3, rotacion.m7, rotacion.m11, rotacion.m15);
    DrawText(buff, GetScreenWidth() - 200, 100, 20, BLUE);
}

void nearCallback(void *data, dGeomID o1, dGeomID o2)
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

        dJointID c = dJointCreateContact(world, contactGroup, &contact[i]);
        dJointAttach(c, dGeomGetBody(o1), dGeomGetBody(o2));
    }
}

/// @brief Funcion de prueba para imprimir el tiempo restante, falta parametrizar los segundos
/// @param segundosRestantes
static void imprimeTiempo(int &segundosRestantes,const int& segundosParaAmarillo = 30, const int& segundosParaRojo = 10)
{
    static int framesCounter = 0;
    framesCounter++;
    char buff[20] = {};
    // Imprime la cantidad de frames transcurridos
    sprintf(buff, "%d", framesCounter);
    DrawText(buff, 20, 30, 22, PURPLE);

    // 60 frames restan un segundo
    if (framesCounter % 60 == 0)
        segundosRestantes--;

    // Imprime el tiempo restante dependiendo del color
    const int posX = GetScreenWidth()/2 - 20;
    sprintf(buff, "%d", segundosRestantes);
    DrawText(buff, posX, 0, 50, GREEN);
    if (segundosRestantes <= segundosParaAmarillo)
        DrawText(buff, posX, 0, 50, YELLOW);
    if (segundosRestantes <= segundosParaRojo)
        DrawText(buff, posX, 0, 50, RED);
}


static bool TaxiEstaQuieto(const dBodyID& taxiBody){
    const dReal* fuerza = dBodyGetLinearVel(taxiBody);
    const dReal VEL_CERO = 0.8;
    if ((fuerza[0] < VEL_CERO && fuerza[0] > -VEL_CERO) && (fuerza[1] < VEL_CERO && fuerza[1] > -VEL_CERO))
        return true;

    return false;
}


static void GiraCamara(Camera &camera, const Vector3 &posTaxi,const Model& taxi)
{
    //
    // TraceLog(LOG_INFO,std::to_string(GetMouseDelta().x).c_str()); // -x - - - - x
    // TraceLog(LOG_INFO,std::to_string(GetMouseDelta().y).c_str()); // -y
    // y
    const Vector3 rightVector =     {1.0f, 0.0f, 0.0f};
    const Vector3 upVector =        {0.0f, 1.0f, 0.0f};
    const Vector3 forwardVector =   {0.0f, 0.0f, 1.0f};

    Vector3 transformedRightVector = Vector3Transform(rightVector,taxi.transform);
    Vector3 transformedUpVector = Vector3Transform(upVector, taxi.transform);
    Vector3 transformedForwardVector = Vector3Transform(forwardVector, taxi.transform);

    const Vector2 delta = GetMouseDelta();
    const float SENSIBILIDAD = 0.2f;
    

    //camera.target = posTaxi;
   /*
   camera.position = Vector3RotateByAxisAngle(camera.position, {1.0f, 0.0f, 0.0f}, -delta.y * DEG2RAD * SENSIBILIDAD);
   camera.position = Vector3RotateByAxisAngle(camera.position, {0.0f, 1.0f, 0.0f}, -delta.x * DEG2RAD * SENSIBILIDAD);
   */

    camera.position = Vector3RotateByAxisAngle(camera.position, transformedRightVector, -delta.y * DEG2RAD * SENSIBILIDAD);
    camera.position = Vector3RotateByAxisAngle(camera.position, transformedUpVector, -delta.x * DEG2RAD * SENSIBILIDAD);

    //camera.position = Vector3Add(posTaxi,Vector3Normalize(camera.position));
    camera.target = posTaxi;
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

    int segundosRestantes = 60; // Inicializar siempre mayor a cero
    const int TIEMPO_ADICIONAL = 5; // Tiempo a agregar al tocar un punto de recoleccion
    const float VELOCIDAD_TAXI = 35.0f;

    SetWindowMonitor(1);
    // Definimos la camara para ver nuestro espacio 3D
    Camera camera = {0};
    camera.position = {0.0f, 10.0f, 10.0f};
    // camera.position = {0.0f, 1.0f, 10.0f}; Para ver a nivel de suelo TODO: Solo debug
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Anadimos el modelo del auto (y llantas)
    Model taxi = LoadModel("assets/taxi_base.glb");
    Model llantaI = LoadModel("assets/llanta_izq.glb");
    Model llantaD = LoadModel("assets/llanta_der.glb");

    // Aniadimos el modelo del marcador
    Model marcador = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    Matrix traslacion = MatrixTranslate(4.0f, 0.5f, 0.0f);
    marcador.transform = MatrixMultiply(marcador.transform, traslacion);

    BoundingBox boundBoxTaxi = GetModelBoundingBox(taxi);

    Model rampa = LoadModelFromMesh(GenMeshCube(1, 1, 1)); //--

    // Inicializacion del sistema de fisica
    const dReal timeStep = 0.01;

    dInitODE();
    world = dWorldCreate();
    space = dSimpleSpaceCreate(0);
    contactGroup = dJointGroupCreate(0);

    dWorldSetGravity(world, 0, 0, -9.81);
    dWorldSetAngularDamping(world, 0.1);

    // Crear la bolita
    crearBolita();

    // Crear el piso
    dGeomID ground = dCreatePlane(space, 0, 0, 1, 0);

    SetTargetFPS(60); // Hacemos que el juego corra a 60 fps (idealmente)
    // Loop principal
    while (!WindowShouldClose()) // Detectar si se cierra la ventana o se presiona ESC
    {
        // Update
        //----------------------------------------------------------------------------------
        
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
        Vector3 wheelLPos = {0.36f, -0.28f, 0.78f};
        Vector3 wheelRPos = {-0.36f, -0.28f, 0.78f};
        // Transform forward vector using model matrix
        Vector3 transformedForwardVector = Vector3Transform(forwardVector, taxi.transform);
        wheelLPos = Vector3Transform(wheelLPos, taxi.transform);
        wheelRPos = Vector3Transform(wheelRPos, taxi.transform);
        
        Matrix taxiRotation;
        Vector3 accelerationVector = Vector3Zero();
        if (IsKeyDown(KEY_W))
        {
            taxiRotation = MatrixRotateY(rotationStep);
            taxi.transform = MatrixMultiply(taxi.transform, taxiRotation);
            accelerationVector = Vector3Scale(transformedForwardVector, VELOCIDAD_TAXI);
           // dBodyAddForce(ball, accelerationVector.x, accelerationVector.z, accelerationVector.y);
        }
        if (IsKeyDown(KEY_S))
        { // Reversa
            taxiRotation = MatrixRotateY(-rotationStep);
            taxi.transform = MatrixMultiply(taxi.transform, taxiRotation);
            accelerationVector = Vector3Scale(transformedForwardVector, VELOCIDAD_TAXI);
            accelerationVector = Vector3Scale(accelerationVector,-1.0f);
            //dBodyAddForce(ball, -accelerationVector.x, -accelerationVector.z, -accelerationVector.y);
        }
        dBodyAddForce(ball, accelerationVector.x, accelerationVector.z, accelerationVector.y);

        // Actualizacion del sistema de fisica
        dSpaceCollide(space, 0, &nearCallback);
        dWorldStep(world, timeStep);
        dJointGroupEmpty(contactGroup);

        const dReal *pos = dBodyGetPosition(ball);
        Vector3 correctedPos = {(float)pos[0], (float)pos[2], (float)pos[1]};
        //camera.position = Vector3Add(correctedPos, {0, 10, 10});
        //camera.target = correctedPos;
        
        if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            GiraCamara(camera, correctedPos, taxi);

        // Rotacion del muro TODO:
        //simulaRotacion
        

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        
        // Dibuja el taxi
        DrawModel(taxi, Vector3Add(correctedPos, {0, -0.5, 0}), 1.0f, WHITE);
        DrawModel(llantaI, Vector3Add(correctedPos, wheelLPos), 1.0f, WHITE);
        DrawModel(llantaD, Vector3Add(correctedPos, wheelRPos), 1.0f, WHITE);

        DrawSphereWires( correctedPos, 1.0f, 16, 16, LIME);

        DrawLine3D(correctedPos, Vector3Add(correctedPos, Vector3Scale(transformedForwardVector,3.0f)), RED);

        // Dibuja posible muro TODO TEST
    /*    const dReal *posMuro = dBodyGetPosition(rampaBody);
        const Vector3 posMuroV = {(float)posMuro[0], (float)posMuro[2], (float)posMuro[1]};
        DrawCubeWires(posMuroV, 2.0f, 6.0f, 3.0f, RED);
        DrawCube(posMuroV, 2.0f, 6.0f, 3.0f, BLUE);

    */
        // Dibuja marcador(objeto a recoger)
        DrawModel(marcador, Vector3Zero(), 1, GOLD);

        DrawGrid(100, 1.0f);             // Draw a grid
      
    /*  simulaRotacion(rampa, geomTest); //--
    */

        // Dibuja la bounding box del taxi
        DrawBoundingBox(boundBoxTaxi, RED);

        // Colision con el marcadot cambia su posicion
        BoundingBox marcadorbb = GetModelBoundingBox(marcador);
        bool enMarcador = CheckCollisionBoxSphere(marcadorbb, correctedPos, 1.0f);
        if (enMarcador && TaxiEstaQuieto(ball))
        {
            DrawModel(marcador, Vector3Zero(), 1, RED);
            traslacion = MatrixTranslate(GetRandomValue(-10, 10), 0.0f, GetRandomValue(-10, 10));
            marcador.transform = MatrixMultiply(marcador.transform, traslacion);
            segundosRestantes += TIEMPO_ADICIONAL;
        }

        DrawBoundingBox(marcadorbb, PURPLE);

        EndMode3D();

        dibujaInfoTaxi(dBodyGetPosition(ball), taxi.transform);
        imprimeTiempo(segundosRestantes);
        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Inicializacion
    //--------------------------------------------------------------------------------------
    dJointGroupDestroy(contactGroup);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();
    UnloadModel(taxi);
    UnloadModel(llantaI);
    UnloadModel(llantaD);
    UnloadModel(rampa); //--
    UnloadModel(marcador);
    CloseWindow(); // Cerrar ventana y conteexto OpenGL
    //--------------------------------------------------------------------------------------

    return 0;
}