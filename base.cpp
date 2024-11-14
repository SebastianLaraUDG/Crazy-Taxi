#include <raylib.h>
#include <raymath.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ode/ode.h>

#include<cstdio>

static dWorldID world;
static dSpaceID space;
static dBodyID ball;
static dGeomID ballGeom;
static dMass m;
static dJointGroupID contactGroup;
const dReal radius = 0.5;

// Mi propia figura (test)
static dBodyID rampa;
static dGeomID rampaGeom;

void nearCallback(void* data, dGeomID o1, dGeomID o2) 
{
    const int MAX_CONTACTS = 10;
    dContact contact[MAX_CONTACTS];

    int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact));
    for (int i = 0; i < numc; i++) {
        contact[i].surface.mode = dContactBounce;
        contact[i].surface.mu = dInfinity;
        contact[i].surface.bounce = 0.7;
        contact[i].surface.bounce_vel = 0.1;

        dJointID c = dJointCreateContact(world, contactGroup, &contact[i]);
        dJointAttach(c, dGeomGetBody(o1), dGeomGetBody(o2));
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

    InitWindow(screenWidth, screenHeight, "SANE TAXI TILIN");

    // Cargar modelo
    Model taxi = LoadModel("assets/taxi_base.glb");
	Vector3 offsetPosTaxi = { 0.0f,-0.5f,0.0f };

    // Definimos la camara para ver nuestro espacio 3D
    Camera camera = { 0 };
    camera.position = { 0.0f, 10.0f, 10.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    //Inicialización del sistema de fisica
    const dReal timeStep = 0.01;
    
    dInitODE();
    world = dWorldCreate();
    space = dSimpleSpaceCreate(0);
    contactGroup = dJointGroupCreate(0);

    dWorldSetGravity(world, 0, 0, -9.81);
    dWorldSetAngularDamping(world, timeStep);

    // Crear cubo con rotacion para que parezca rampa
    dGeomID rampa1 = dCreateBox(space, 2, 2, 2);
    dGeomSetPosition(rampa1, 0, 0, -0.5);
    dMatrix3 R;
    dRFromEulerAngles(R, 0, 0, 20);
    dGeomSetRotation(rampa1, R);

    // Crear la bolita
    ball = dBodyCreate(world);
    dMass m;
    dMassSetSphere(&m, 1, radius);
    dBodySetMass(ball, &m);
    dBodySetPosition(ball, 0, 0, 3);

    ballGeom = dCreateSphere(space, radius);
    dGeomSetBody(ballGeom, ball);

    // Crear rampa
    rampa = dBodyCreate(world);
    dMass r;
    dMassSetBox(&r, 1, 1.5, 6, 3);
    dBodySetMass(rampa, &r);
    dBodySetPosition(rampa, 0, 0, 7);

    rampaGeom = dCreateBox(space, 1.5, 6, 3);
    dGeomSetBody(rampaGeom, rampa);

    // Crear el piso
    dGeomID ground = dCreatePlane(space, 0, 0, 1, 0);

    Model cuboRampa = LoadModelFromMesh(GenMeshCube(1, 1, 1));

    SetTargetFPS(60);               // Hacemos que el juego corra a 60 fps (idealmente)


    // Loop principal
    while (!WindowShouldClose())    // Detectar si se cierra la ventana o se presiona ESC
    {
        // Update
        //----------------------------------------------------------------------------------

        // Rotar el taxi
        float cambioRotacion = 0;
        if (IsKeyDown(KEY_D))
            cambioRotacion = -0.04f;
        else if(IsKeyDown(KEY_A))
            cambioRotacion = 0.04f;
        Matrix matrizRotTaxi = MatrixRotateY(cambioRotacion);
        taxi.transform = MatrixMultiply(taxi.transform, matrizRotTaxi);


        // Definir vector de fuerza
        Vector3 forwardVector = { 0.0f, 0.0f,35.0f };
        Vector3 transformForwardVector = Vector3Transform(forwardVector, taxi.transform);
        if (IsKeyDown(KEY_W)) {
            dBodyAddForce(ball, transformForwardVector.x, transformForwardVector.z, transformForwardVector.y);
        }
        if (IsKeyDown(KEY_S)) {
            dBodyAddForce(ball, -transformForwardVector.x, -transformForwardVector.z, -transformForwardVector.y);
        }

        // Actualizacion del sistema de fisica
        dSpaceCollide(space, 0, &nearCallback);


        dWorldStep(world, timeStep);
        dJointGroupEmpty(contactGroup);

        const dReal* pos = dBodyGetPosition(ball);

        Vector3 posCorregida = { (float)pos[0], (float)pos[2], (float)pos[1] };
        camera.target = posCorregida;

        const dReal* posRampa = dBodyGetPosition(rampa);
        Vector3 posRampaV = { (float)posRampa[0], (float)posRampa[2], (float)posRampa[1] };

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        
        Vector3 posModelo = Vector3Add(posCorregida, offsetPosTaxi);
        DrawModel(taxi,posModelo, 1.0f, WHITE);

        DrawSphereWires( posCorregida, 1.0f, 16, 16, LIME);
        DrawCubeWires(posRampaV, 1.5, 6, 3, BLUE);

        DrawGrid(100, 1.0f);        // Draw a grid

        const dReal* rampPosP = dGeomGetPosition(rampa1);
        const dReal* rampRotP = dGeomGetRotation(rampa1);
        Vector3 rampPosR = { (float)rampPosP[0],(float)rampPosP[2],(float)rampPosP[1] };

		Matrix rotation = { (float)rampRotP[0], (float)rampRotP[4], (float)rampRotP[8], (float)rampRotP[0],
							(float)rampRotP[1], (float)rampRotP[5], (float)rampRotP[9], (float)rampRotP[0],
							(float)rampRotP[2], (float)rampRotP[6], (float)rampRotP[10], (float)rampRotP[0],
							(float)rampRotP[3], (float)rampRotP[7], (float)rampRotP[11],(float)rampRotP[0]
		};
        

        EndMode3D();
        char buff[20] = {};
        sprintf(buff, "X: %.0f Y:%.0f Z:%.0f", posModelo.x, posModelo.y, posModelo.z);
        DrawText(buff, 0, 30, 20, DARKBLUE);

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Inicializacion
    //--------------------------------------------------------------------------------------
    UnloadModel(taxi);
    dJointGroupDestroy(contactGroup);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();
    CloseWindow();        // Cerrar ventana y conteexto OpenGL
    //--------------------------------------------------------------------------------------

    return 0;
}