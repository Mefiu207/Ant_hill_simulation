#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <windows.h>
#include <GL/glu.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>


struct Ant {
    float x, y, z;
    float dirX, dirZ;

    bool carryingFood = false;
};


struct Food {
    float x, y, z;
    int amount;
};

std::vector<Food> foods;

const std::size_t MAX_FOOD_SOURCES = 20;

struct Obstacle {
    float x, y, z;
    float size;
};

std::vector<Ant> ants;
std::vector<Obstacle> obstacles;


const float ANTHILL_BASE_RADIUS = 10.0f;
const float ANTHILL_TOP_RADIUS = 3.0f;
const float ANTHILL_HEIGHT = 12.0f;
const float ANTHILL_HOLE_RADIUS = 1.0f;
const int MAX_ANTS = 2000;

const float ANT_SPEED = 3.0f;

const std::size_t MAX_OBSTACLES = 35;


float camAngleY = 30.0f;
float camAngleX = 20.0f;
float camDist = 25.0f;

GLUquadric* g_quadric = nullptr;


void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 10.0f, 15.0f, 10.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.2f,  0.2f,  0.2f,  1.0f };
    GLfloat lightDiffuse[] = { 0.8f,  0.8f,  0.8f,  1.0f };
    GLfloat lightSpec[] = { 1.0f,  1.0f,  1.0f,  1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
}

void drawFood()
{
    if (!g_quadric) return;

    glColor3f(0.9f, 0.9f, 0.1f);

    for (const auto& f : foods) {
        glPushMatrix();
        glTranslatef(f.x, f.y, f.z);
        gluSphere(g_quadric, 0.7, 12, 12);
        glPopMatrix();
    }
}


void drawGround()
{
    float size = 50.0f;
    float step = 2.0f;

    glNormal3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.2f, 0.6f, 0.2f);

    for (float z = -size; z < size; z += step) {
        glBegin(GL_TRIANGLE_STRIP);
        for (float x = -size; x <= size; x += step) {
            glVertex3f(x, 0.0f, z);
            glVertex3f(x, 0.0f, z + step);
        }
        glEnd();
    }
}

float getGroundHeightAt(float x, float z)
{
    float r = std::sqrt(x * x + z * z);


    if (r >= ANTHILL_BASE_RADIUS)
        return 0.0f;


    if (r >= ANTHILL_TOP_RADIUS) {
        float t = (ANTHILL_BASE_RADIUS - r) / (ANTHILL_BASE_RADIUS - ANTHILL_TOP_RADIUS);
        return ANTHILL_HEIGHT * t;
    }

    if (r >= ANTHILL_HOLE_RADIUS)
        return ANTHILL_HEIGHT;


    return ANTHILL_HEIGHT * 0.95f;
}

void drawCube(float size)
{
    float s = size * 0.5f;

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-s, s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, s, s);
    glVertex3f(-s, s, s);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-s, -s, -s);
    glVertex3f(-s, -s, s);
    glVertex3f(s, -s, s);
    glVertex3f(s, -s, -s);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-s, -s, s);
    glVertex3f(s, -s, s);
    glVertex3f(s, s, s);
    glVertex3f(-s, s, s);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-s, -s, -s);
    glVertex3f(-s, s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, -s, -s);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-s, -s, -s);
    glVertex3f(-s, -s, s);
    glVertex3f(-s, s, s);
    glVertex3f(-s, s, -s);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(s, -s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, s, s);
    glVertex3f(s, -s, s);
    glEnd();
}

void drawObstacles()
{
    glColor3f(0.4f, 0.2f, 0.1f);

    for (const auto& o : obstacles) {
        glPushMatrix();
        glTranslatef(o.x, o.y, o.z);
        drawCube(o.size);
        glPopMatrix();
    }
}

void addRandomObstacle()
{
    if (obstacles.size() >= MAX_OBSTACLES)
        return;

    Obstacle o;
    o.size = 6.0f;

    float HALF_SIZE = 50.0f - o.size;

    float rx = (std::rand() % 1000) / 1000.0f;
    float rz = (std::rand() % 1000) / 1000.0f;

    o.x = -HALF_SIZE + 2.0f * HALF_SIZE * rx;
    o.z = -HALF_SIZE + 2.0f * HALF_SIZE * rz;

    float groundY = getGroundHeightAt(o.x, o.z);
    o.y = groundY + o.size * 0.5f;

    obstacles.push_back(o);
}

void drawAnthill()
{
    if (!g_quadric) return;

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);

    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    float baseRadius = ANTHILL_BASE_RADIUS;
    float topRadius = ANTHILL_TOP_RADIUS;
    float height = ANTHILL_HEIGHT;
    float holeRadius = ANTHILL_HOLE_RADIUS;

    glColor3f(0.5f, 0.35f, 0.2f);
    gluDisk(g_quadric, 0.0, baseRadius, 32, 1);

    gluCylinder(g_quadric, baseRadius, topRadius, height, 32, 16);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, height);

    glColor3f(0.45f, 0.30f, 0.18f);
    gluDisk(g_quadric, holeRadius, topRadius, 32, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, height - 0.05f);
    glColor3f(0.08f, 0.05f, 0.02f);
    gluDisk(g_quadric, 0.0, holeRadius * 0.9f, 16, 1);
    glPopMatrix();

    glPopMatrix();
}

void getGroundNormalAt(float x, float z, float& nx, float& ny, float& nz)
{
    const float eps = 0.1f;

    float hL = getGroundHeightAt(x - eps, z);
    float hR = getGroundHeightAt(x + eps, z);
    float hD = getGroundHeightAt(x, z - eps);
    float hU = getGroundHeightAt(x, z + eps);

    float vx_x = 2.0f * eps;
    float vx_y = hR - hL;
    float vx_z = 0.0f;

    float vz_x = 0.0f;
    float vz_y = hU - hD;
    float vz_z = 2.0f * eps;

    nx = vx_y * vz_z - vx_z * vz_y;
    ny = vx_z * vz_x - vx_x * vz_z;
    nz = vx_x * vz_y - vx_y * vz_x;

    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0.0001f) {
        nx /= len;
        ny /= len;
        nz /= len;
    }
    else {
        nx = 0.0f;
        ny = 1.0f;
        nz = 0.0f;
    }
}

void drawAnt(const Ant& ant)
{
    if (!g_quadric) return;

    float nx, ny, nz;
    getGroundNormalAt(ant.x, ant.z, nx, ny, nz);
    float ux = nx, uy = ny, uz = nz;

    float fx = ant.dirX;
    float fy = 0.0f;
    float fz = ant.dirZ;

    float flen = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (flen < 0.0001f) {
        fx = 0.0f; fy = 0.0f; fz = 1.0f;
        flen = 1.0f;
    }
    else {
        fx /= flen;
        fy /= flen;
        fz /= flen;
    }

    float dotFN = fx * ux + fy * uy + fz * uz;
    fx = fx - dotFN * ux;
    fy = fy - dotFN * uy;
    fz = fz - dotFN * uz;

    flen = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (flen < 0.0001f) {
        if (std::fabs(ux) < 0.9f)
        {
            fx = uz;
            fy = 0.0f;
            fz = -ux;
        }
        else
        {
            fx = 0.0f;
            fy = uz;
            fz = -uy;
        }
        flen = std::sqrt(fx * fx + fy * fy + fz * fz);
    }
    fx /= flen;
    fy /= flen;
    fz /= flen;

    float rx = uy * fz - uz * fy;
    float ry = uz * fx - ux * fz;
    float rz = ux * fy - uy * fx;

    float rlen = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (rlen > 0.0001f) {
        rx /= rlen;
        ry /= rlen;
        rz /= rlen;
    }

    float m[16] = {
        rx, ry, rz, 0.0f,
        ux, uy, uz, 0.0f,
        fx, fy, fz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    glPushMatrix();
    glTranslatef(ant.x, ant.y, ant.z);
    glMultMatrixf(m);
    glScalef(0.3f, 0.3f, 0.5f);

    glColor3f(0.1f, 0.1f, 0.1f);

    gluSphere(g_quadric, 0.5, 16, 16);

    glTranslatef(0.0f, 0.0f, 0.8f);
    gluSphere(g_quadric, 0.4, 16, 16);

    glTranslatef(0.0f, 0.0f, 0.7f);
    gluSphere(g_quadric, 0.35, 16, 16);
    glPopMatrix();
}

void drawAnts()
{
    for (const auto& a : ants) {
        drawAnt(a);
    }
}

void updateAnts(float dt)
{
    if (dt <= 0.0f) return;

    const float HALF_SIZE = 50.0f;
    const float BOUNCE_MARGIN = 1.0f;

    const float REORIENT_PROB_PER_SEC = 0.5f;
    const float TURN_SPEED = 4.0f;

    const float AVOID_RADIUS = 2.0f;
    const float AVOID_RADIUS2 = AVOID_RADIUS * AVOID_RADIUS;
    const float AVOID_WEIGHT = 5.0f;

    const float OBSTACLE_MARGIN = 1.5f;
    const float OBSTACLE_WEIGHT = 8.0f;

    const float FOOD_DETECT_RADIUS = 8.0f;
    const float FOOD_DETECT_RADIUS2 = FOOD_DETECT_RADIUS * FOOD_DETECT_RADIUS;
    const float FOOD_PICK_RADIUS = 1.5f;
    const float NEST_RADIUS = ANTHILL_TOP_RADIUS + 1.0f;

    for (std::size_t i = 0; i < ants.size(); ++i) {
        Ant& a = ants[i];

        // ----------------- 1) LOGIKA KIERUNKU: SZUKANIE / NIESIENIE -----------------

        if (a.carryingFood) {
            float dx = 0.0f - a.x;
            float dz = 0.0f - a.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist > 0.001f) {
                a.dirX = dx / dist;
                a.dirZ = dz / dist;
            }

            if (dist < NEST_RADIUS) {
                a.carryingFood = false;

                float angle = (std::rand() % 1000) / 1000.0f * 2.0f * 3.14159265f;
                a.dirX = std::cos(angle);
                a.dirZ = std::sin(angle);
            }
        }
        else {
            float p = REORIENT_PROB_PER_SEC * dt;
            float r = (std::rand() % 1000) / 1000.0f;

            if (r < p) {
                float angle = (std::rand() % 1000) / 1000.0f * 2.0f * 3.14159265f;
                a.dirX = std::cos(angle);
                a.dirZ = std::sin(angle);
            }
            else {
                float randTurn = ((std::rand() % 2000) / 1000.0f - 1.0f);
                float deltaAngle = randTurn * TURN_SPEED * dt;

                float cosA = std::cos(deltaAngle);
                float sinA = std::sin(deltaAngle);

                float newDirX = a.dirX * cosA - a.dirZ * sinA;
                float newDirZ = a.dirX * sinA + a.dirZ * cosA;

                float len = std::sqrt(newDirX * newDirX + newDirZ * newDirZ);
                if (len > 0.0001f) {
                    a.dirX = newDirX / len;
                    a.dirZ = newDirZ / len;
                }
            }


            int   bestIndex = -1;
            float bestDist2 = FOOD_DETECT_RADIUS2;

            for (std::size_t fi = 0; fi < foods.size(); ++fi) {
                if (foods[fi].amount <= 0) continue;

                float dx = foods[fi].x - a.x;
                float dz = foods[fi].z - a.z;
                float dist2 = dx * dx + dz * dz;

                if (dist2 < bestDist2) {
                    bestDist2 = dist2;
                    bestIndex = (int)fi;
                }
            }

            if (bestIndex >= 0) {
                float dx = foods[bestIndex].x - a.x;
                float dz = foods[bestIndex].z - a.z;
                float dist = std::sqrt(dx * dx + dz * dz);
                if (dist > 0.001f) {
                    a.dirX = dx / dist;
                    a.dirZ = dz / dist;
                }

                if (dist < FOOD_PICK_RADIUS && foods[bestIndex].amount > 0) {
                    foods[bestIndex].amount--;

                    a.carryingFood = true;

                    if (foods[bestIndex].amount <= 0) {
                        foods.erase(foods.begin() + bestIndex);
                    }
                }
            }
        }

        // ----------------- 2) UNIKANIE INNYCH MRÓWEK -----------------

        float sepX = 0.0f;
        float sepZ = 0.0f;

        for (std::size_t j = 0; j < ants.size(); ++j) {
            if (j == i) continue;
            const Ant& b = ants[j];

            float dx = a.x - b.x;
            float dz = a.z - b.z;
            float dist2 = dx * dx + dz * dz;

            if (dist2 > 0.0001f && dist2 < AVOID_RADIUS2) {
                float dist = std::sqrt(dist2);
                float w = (AVOID_RADIUS - dist) / AVOID_RADIUS;

                sepX += (dx / dist) * w;
                sepZ += (dz / dist) * w;
            }
        }

        if (sepX != 0.0f || sepZ != 0.0f) {
            float lenSep = std::sqrt(sepX * sepX + sepZ * sepZ);
            if (lenSep > 0.0001f) {
                sepX /= lenSep;
                sepZ /= lenSep;

                a.dirX += sepX * AVOID_WEIGHT * dt;
                a.dirZ += sepZ * AVOID_WEIGHT * dt;
            }
        }

        // ----------------- 3) UNIKANIE PRZESZKÓD -----------------

        float obsAvoidX = 0.0f;
        float obsAvoidZ = 0.0f;

        for (const auto& o : obstacles) {
            float dx = a.x - o.x;
            float dz = a.z - o.z;

            float obstacleRadius = std::sqrt(2.0f) * (o.size * 0.5f) + OBSTACLE_MARGIN;
            float obstacleRadius2 = obstacleRadius * obstacleRadius;

            float dist2 = dx * dx + dz * dz;
            if (dist2 < obstacleRadius2 && dist2 > 0.0001f) {
                float dist = std::sqrt(dist2);
                float w = (obstacleRadius - dist) / obstacleRadius;

                obsAvoidX += (dx / dist) * w;
                obsAvoidZ += (dz / dist) * w;
            }
        }

        if (obsAvoidX != 0.0f || obsAvoidZ != 0.0f) {
            float lenObs = std::sqrt(obsAvoidX * obsAvoidX + obsAvoidZ * obsAvoidZ);
            if (lenObs > 0.0001f) {
                obsAvoidX /= lenObs;
                obsAvoidZ /= lenObs;

                a.dirX += obsAvoidX * OBSTACLE_WEIGHT * dt;
                a.dirZ += obsAvoidZ * OBSTACLE_WEIGHT * dt;
            }
        }

        // ----------------- 4) Normalizacja kierunku -----------------

        float lenDir = std::sqrt(a.dirX * a.dirX + a.dirZ * a.dirZ);
        if (lenDir > 0.0001f) {
            a.dirX /= lenDir;
            a.dirZ /= lenDir;
        }

        // ----------------- 5) Ruch po XZ -----------------

        a.x += a.dirX * ANT_SPEED * dt;
        a.z += a.dirZ * ANT_SPEED * dt;

        if (a.x < -HALF_SIZE) {
            a.x = -HALF_SIZE + BOUNCE_MARGIN;
            a.dirX = -a.dirX;
        }
        else if (a.x > HALF_SIZE) {
            a.x = HALF_SIZE - BOUNCE_MARGIN;
            a.dirX = -a.dirX;
        }

        if (a.z < -HALF_SIZE) {
            a.z = -HALF_SIZE + BOUNCE_MARGIN;
            a.dirZ = -a.dirZ;
        }
        else if (a.z > HALF_SIZE) {
            a.z = HALF_SIZE - BOUNCE_MARGIN;
            a.dirZ = -a.dirZ;
        }


        float groundY = getGroundHeightAt(a.x, a.z);
        a.y = groundY + 0.1f;
    }
}

void addRandomAnt()
{
    if (ants.size() >= MAX_ANTS)
        return;

    Ant a;

    float radius = ANTHILL_TOP_RADIUS + 0.1f;
    float angle = (std::rand() % 1000) / 1000.0f * 2.0f * 3.14159265f;

    a.x = radius * std::cos(angle);
    a.z = radius * std::sin(angle);

    float groundY = getGroundHeightAt(a.x, a.z);
    a.y = groundY + 0.1f;

    float dirAngle = (std::rand() % 1000) / 1000.0f * 2.0f * 3.14159265f;
    a.dirX = std::cos(dirAngle);
    a.dirZ = std::sin(dirAngle);
    a.carryingFood = false;

    ants.push_back(a);
}


void setCamera()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float radY = camAngleY * 3.14159265f / 180.0f;
    float radX = camAngleX * 3.14159265f / 180.0f;

    float eyeX = camDist * std::cos(radX) * std::sin(radY);
    float eyeY = camDist * std::sin(radX);
    float eyeZ = camDist * std::cos(radX) * std::cos(radY);

    const float minCameraHeight = 1.0f;
    if (eyeY < minCameraHeight)
        eyeY = minCameraHeight;

    gluLookAt(
        eyeX, eyeY, eyeZ,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    );
}

void resizeGL(int width, int height)
{
    if (height == 0) height = 1;
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 0.1, 200.0);

    glMatrixMode(GL_MODELVIEW);
}


void initOpenGL()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f);

    setupLighting();

    g_quadric = gluNewQuadric();
    if (g_quadric) {
        gluQuadricNormals(g_quadric, GLU_SMOOTH);
        gluQuadricTexture(g_quadric, GL_FALSE);
    }

    std::srand(static_cast<unsigned>(time(nullptr)));
}

void addRandomFood()
{
    if (foods.size() >= MAX_FOOD_SOURCES)
        return;

    Food f;
    f.amount = 20;

    const float HALF_SIZE = 50.0f - 2.0f;
    float rx = (std::rand() % 1000) / 1000.0f;
    float rz = (std::rand() % 1000) / 1000.0f;

    f.x = -HALF_SIZE + 2.0f * HALF_SIZE * rx;
    f.z = -HALF_SIZE + 2.0f * HALF_SIZE * rz;

    float groundY = getGroundHeightAt(f.x, f.z);
    f.y = groundY + 0.5f;

    foods.push_back(f);
}

void updateCameraFromKeyboard(float dt)
{
    const float angleSpeed = 60.0f;
    const float zoomSpeed = 20.0f;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        camAngleY -= angleSpeed * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        camAngleY += angleSpeed * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        camAngleX += angleSpeed * dt;
        if (camAngleX > 89.0f) camAngleX = 89.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        camAngleX -= angleSpeed * dt;
        if (camAngleX < -10.0f) camAngleX = -10.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
        camDist -= zoomSpeed * dt;
        if (camDist < 5.0f) camDist = 5.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) {
        camDist += zoomSpeed * dt;
        if (camDist > 80.0f) camDist = 80.0f;
    }
}

void showLegend() {
    std::cout << "MOVE CAMERA           :   ARROWS\n";
    std::cout << "ZOOM IN               :   Z\n";
    std::cout << "ZOOM OUT              :   X\n\n";

    std::cout << "ADD OBSTACLE          :   O\n";
    std::cout << "REMOVE OBSTACLE       :   P\n\n";

    std::cout << "ADD FOOD              :   F\n\n";

    std::cout << "ADD ANT               :   A\n";
    std::cout << "KILL AN ANT           :   K\n";
    std::cout << "KILL ALL ANTS         :   Q\n";

}

void killAllAnts() {
    ants.clear();
}

void killAnt() {
    if (!ants.empty())
        ants.pop_back();
}

void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setCamera();
    setupLighting();

    drawFood();
    drawGround();
    drawAnthill();
    drawAnts();
    drawObstacles();
}

int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 2;
    settings.minorVersion = 1;

    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "Anthill Simulation - SFML + OpenGL",
        sf::Style::Default,
        settings
    );

    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    initOpenGL();
    resizeGL(window.getSize().x, window.getSize().y);

    sf::Clock clock;
    showLegend();

    bool running = true;
    while (running && window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false;
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                resizeGL(event.size.width, event.size.height);
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                addRandomAnt();
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::K)) {
                killAnt();
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
                killAllAnts();
            }
            else if (event.key.code == sf::Keyboard::O) {
                addRandomObstacle();
            }
            else if (event.key.code == sf::Keyboard::P && !obstacles.empty()) {
                obstacles.pop_back();
            }
            else if (event.key.code == sf::Keyboard::F) {
                addRandomFood();
            }
        }

        float dt = clock.restart().asSeconds();

        updateCameraFromKeyboard(dt);
        updateAnts(dt);

        drawScene();

        window.display();

    }

    if (g_quadric) {
        gluDeleteQuadric(g_quadric);
    }

    return 0;
}