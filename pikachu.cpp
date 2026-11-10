/****************************************************************/
/*                      pikachu_base.cpp                        */
/****************************************************************/

#ifdef __APPLE__                      // Si on compile sur macOS
#include <GLUT/glut.h>                // Inclure la version GLUT spécifique à Apple
#else                                  // Sinon (Linux/Windows)
#include <GL/glut.h>                  // Inclure GLUT standard
#endif

#include <cstdlib>                    // Fonctions utilitaires standard (exit, etc.)
#include <cmath>                      // Fonctions mathématiques (sin, cos, M_PI, etc.)
#include <iostream>                   // Entrées/sorties C++ (cout, cerr)
#include <jpeglib.h>                  // Bibliothèque pour lire des fichiers JPEG
#include <jerror.h>                   // Gestion des erreurs pour libjpeg
using namespace std;                  // Évite d'écrire std:: avant cout, cerr, etc.


/* ============================================================= */
/*                 VARIABLES GLOBALES DU PROGRAMME               */
/* ============================================================= */

char presse;                          // Indique si le bouton gauche de la souris est pressé
int anglex = 0, angley = 0;           // Variables pour stocker les angles (non vraiment utilisées dans la vue actuelle)
int xold, yold;                       // Ancienne position de la souris pour calculer les déplacements
bool idleEnabled = false;             // Active/désactive l’animation de rotation automatique de la caméra
bool jouesRouges = true;              // true = joues rouges, false = joues roses

float jumpHeight = 0.0f;              // Décalage vertical courant pour le saut de Pikachu
float jumpTime = 0.0f;                // Temps accumulé pour calculer l’animation du saut
const float jumpDuration = 3.0f;      // Durée totale d’un cycle de saut (montée + descente)
const float jumpAmplitude = 0.050f;   // Hauteur maximale du saut

/* = Variables caméra = */
float camera_distance = 3.0f;         // Distance de la caméra par rapport à la scène
float camera_angleX = 20.0f;          // Angle de la caméra autour de l’axe X (inclinaison)
float camera_angleY = 0.0f;           // Angle de la caméra autour de l’axe Y (rotation horizontale)

/* = Variables des textures = */
GLuint texSol;                        // Identifiant OpenGL pour la texture du sol
unsigned char imageSol[256*256*3];    // Buffer pour stocker les pixels de l’image du sol

GLuint texCorps;                      // Identifiant OpenGL pour la texture du corps de Pikachu
unsigned char imageCorps[256*256*3];  // Buffer pour stocker les pixels de la texture jaune

GLuint texYeux;                       // Identifiant OpenGL pour la texture des yeux
unsigned char imageYeux[256*256*3];   // Buffer pour stocker les pixels de la texture des yeux


/* ============================================================= */
/*               PROTOTYPES DES FONCTIONS UTILISÉES              */
/* ============================================================= */

void affichage();                     // Fonction de callback pour le dessin de la scène
void clavier(unsigned char touche, int x, int y); // Callback du clavier (touches normales)
void reshape(int x, int y);           // Callback lors du redimensionnement de la fenêtre
void mouse(int bouton, int etat, int x, int y);   // Callback clic souris
void mousemotion(int x, int y);       // Callback mouvement de la souris (drag)
void idle();                          // Callback appelée quand GLUT est inactif (animation)
void drawAxes();                      // Dessine les axes de repère (optionnel)
void drawPikachu();                   // Dessine le modèle complet de Pikachu
void drawSphereParam(float rayon, int slices, int stacks); // Sphère paramétrique non texturée
void drawSphereParamTextured(float r, int slices, int stacks); // Sphère paramétrique texturée
void drawCylinderParam(float rBase, float rTop, float h, int slices); // Cylindre paramétrique non texturé
void drawCylinderParamTextured(float rBase, float rTop, float h, int slices); // Cylindre paramétrique texturé
void drawSolTexture();                // Dessine le sol texturé
void specialKeys(int key, int x, int y); // Callback pour les touches spéciales (flèches)
void loadJpegImage(const char *filename, unsigned char *buffer); // Charge une image JPEG dans un buffer


/* ============================================================= */
/*                        PROGRAMME PRINCIPAL                    */
/* ============================================================= */

int main(int argc, char **argv)       // Point d’entrée du programme
{
    /* Charger les textures avant l’initialisation de GLUT */
    loadJpegImage("sol.jpg", imageSol);    // Charge l’image de la texture du sol dans imageSol
    loadJpegImage("jaune.jpg", imageCorps);// Charge l’image de la texture du corps dans imageCorps
    loadJpegImage("yeux.jpg", imageYeux);  // Charge l’image de la texture des yeux dans imageYeux

    glutInit(&argc, argv);                 // Initialisation de GLUT avec les arguments de la ligne de commande
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); // Mode : RGB, double-buffering, Z-buffer
    glutInitWindowPosition(200, 200);      // Position initiale de la fenêtre (x, y)
    glutInitWindowSize(800, 800);          // Taille initiale de la fenêtre (largeur, hauteur)
    glutCreateWindow("Pikachu");           // Création de la fenêtre avec un titre

    glClearColor(0.05f, 0.1f, 0.3f, 1.0f); // Couleur de fond (bleu nuit)
    glEnable(GL_DEPTH_TEST);               // Active le test de profondeur (Z-buffer)
    glEnable(GL_NORMALIZE);                // Normalise les normales après les transformations
    glShadeModel(GL_SMOOTH);               // Active l’ombrage lisse (interpolation des normales)

    /* ===== Création et paramétrage des textures ===== */

    glGenTextures(1, &texSol);             // Génère un identifiant de texture pour le sol
    glBindTexture(GL_TEXTURE_2D, texSol);  // Sélectionne la texture du sol comme texture active
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtrage minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtrage magnification
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0,  // Envoie les pixels au GPU
                 GL_RGB, GL_UNSIGNED_BYTE, imageSol);    // Format des données : RGB, 8 bits par canal

    glGenTextures(1, &texCorps);           // Génère un identifiant de texture pour le corps
    glBindTexture(GL_TEXTURE_2D, texCorps);// Sélectionne la texture du corps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtrage min
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtrage mag
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0,  // Envoie les pixels au GPU
                 GL_RGB, GL_UNSIGNED_BYTE, imageCorps);  // Données = buffer imageCorps

    glGenTextures(1, &texYeux);            // Génère un identifiant de texture pour les yeux
    glBindTexture(GL_TEXTURE_2D, texYeux); // Sélectionne la texture des yeux
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Filtrage min
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtrage mag
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0,  // Envoie les pixels au GPU
                 GL_RGB, GL_UNSIGNED_BYTE, imageYeux);   // Données = buffer imageYeux

    /* ======== Configuration des lumières ======== */

    /* Lumière ponctuelle (Light0) positionnée dans la scène */
    GLfloat light0_position[] = {2.0f, 3.0f, 2.0f, 1.0f};  // w=1 → source ponctuelle
    GLfloat light0_diffuse[]  = {1.0f, 1.0f, 0.9f, 1.0f};  // Composante diffuse légèrement jaunâtre
    GLfloat light0_specular[] = {1.0f, 1.0f, 0.9f, 1.0f};  // Composante spéculaire
    GLfloat light0_ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};  // Composante ambiante
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);    // Applique la position
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diffuse);     // Applique la lumière diffuse
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);    // Applique la lumière spéculaire
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light0_ambient);     // Applique la lumière ambiante
    glEnable(GL_LIGHT0);                                   // Active la lumière 0

    /* Lumière directionnelle (Light1) simulant une lumière de ciel */
    GLfloat light1_direction[] = {-1.0f, -1.0f, -0.5f, 0.0f}; // w=0 → lumière directionnelle
    GLfloat light1_diffuse[]    = {0.6f, 0.7f, 0.8f, 1.0f};   // Lumière diffuse bleu ciel
    GLfloat light1_specular[]   = {0.6f, 0.7f, 0.8f, 1.0f};   // Composante spéculaire
    GLfloat light1_ambient[]    = {0.1f, 0.1f, 0.15f, 1.0f};  // Composante ambiante légère
    glLightfv(GL_LIGHT1, GL_POSITION, light1_direction);      // Direction de la lumière
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  light1_diffuse);        // Diffuse
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);       // Spéculaire
    glLightfv(GL_LIGHT1, GL_AMBIENT,  light1_ambient);        // Ambiante
    glEnable(GL_LIGHT1);                                     // Active la lumière 1

    glEnable(GL_LIGHTING);                                  // Active le calcul d’éclairage global

    /* ===== Enregistrement des callbacks GLUT ===== */

    glutDisplayFunc(affichage);        // Fonction appelée pour dessiner la scène
    glutKeyboardFunc(clavier);         // Fonction appelée lors d’une touche clavier
    glutSpecialFunc(specialKeys);      // Fonction appelée pour les touches spéciales (flèches)
    glutReshapeFunc(reshape);          // Fonction appelée quand la fenêtre change de taille
    glutMouseFunc(mouse);              // Fonction appelée lors d’un clic souris
    glutMotionFunc(mousemotion);       // Fonction appelée lors d’un déplacement souris (drag)
    glutIdleFunc(idle);                // Fonction appelée quand GLUT est inactif (pour l’animation)

    glutMainLoop();                    // Boucle principale GLUT (ne retourne jamais)
    return 0;                          // (Jamais atteint, mais pour forme)
}


/* ============================================================= */
/*                       AFFICHAGE DE LA SCÈNE                   */
/* ============================================================= */

void affichage()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Efface l’écran et le Z-buffer
    glMatrixMode(GL_MODELVIEW);                         // Passe en matrice de vue/modèle
    glLoadIdentity();                                   // Réinitialise la matrice courante

    // Conversion des angles de la caméra en radians
    float radX = camera_angleX * M_PI / 180.0f;         // Conversion de l’angle X en radians
    float radY = camera_angleY * M_PI / 180.0f;         // Conversion de l’angle Y en radians

    // Calcul de la position de la caméra en coordonnées sphériques
    float camX = camera_distance * sin(radY) * cos(radX); // Coordonnée X caméra
    float camY = camera_distance * sin(radX);             // Coordonnée Y caméra
    float camZ = camera_distance * cos(radY) * cos(radX); // Coordonnée Z caméra

    float yMilieuCorps = 0.70f;                        // Approximation de la hauteur du centre du corps de Pikachu

    // Positionnement de la caméra avec gluLookAt
    gluLookAt(camX, camY, camZ,                        // Position de la caméra
              0.0, yMilieuCorps, 0.0,                  // Point regardé (vers Pikachu)
              0.0, 1.0, 0.0);                          // Direction "up" (verticale)

    // drawAxes();                                      // Appel optionnel pour dessiner les axes (commenté)

    drawSolTexture();                                  // Dessine le sol texturé
    drawPikachu();                                     // Dessine le modèle complet de Pikachu

    glutSwapBuffers();                                 // Échange les buffers (double-buffering)
}


/* ============================================================= */
/*                    REPÈRE XYZ (OPTIONNEL)                     */
/* ============================================================= */
/*
void drawAxes()
{
    glDisable(GL_LIGHTING);                   // Désactive l’éclairage pour des couleurs brutes
    glBegin(GL_LINES);                        // Commence à dessiner des segments de droite
        glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(1,0,0); // Axe X en rouge
        glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,1,0); // Axe Y en vert
        glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,1); // Axe Z en bleu
    glEnd();                                   // Termine le dessin des lignes
    glEnable(GL_LIGHTING);                    // Réactive l’éclairage
}
*/


/* ============================================================= */
/*                         SOL TEXTURÉ                          */
/* ============================================================= */

void drawSolTexture()
{
    glDisable(GL_LIGHTING);                   // Désactive l’éclairage pour que la texture ne soit pas modifiée
    glEnable(GL_TEXTURE_2D);                  // Active le texturage 2D

    glBindTexture(GL_TEXTURE_2D, texSol);     // Associe la texture du sol à GL_TEXTURE_2D

    glColor3f(1,1,1);                         // Couleur blanche (n’altère pas la texture)
    float T = 4.0f;                           // Nombre de répétitions de la texture sur le sol

    glBegin(GL_QUADS);                        // On dessine un grand quadrilatère
        glTexCoord2f(0,0);   glVertex3f(-1,0,-1);   // Coin arrière gauche avec coordonnée texture (0,0)
        glTexCoord2f(T,0);   glVertex3f( 1,0,-1);   // Coin arrière droit, texture (T,0)
        glTexCoord2f(T,T);   glVertex3f( 1,0, 1);   // Coin avant droit, texture (T,T)
        glTexCoord2f(0,T);   glVertex3f(-1,0, 1);   // Coin avant gauche, texture (0,T)
    glEnd();                                    // Fin du quadrilatère

    glDisable(GL_TEXTURE_2D);                  // Désactive le texturage 2D
    glEnable(GL_LIGHTING);                     // Réactive l’éclairage
}


/* ============================================================= */
/*      SPHÈRE PARAMÉTRIQUE (NON TEXTURÉE, POUR DÉTAILS, ETC.)   */
/* ============================================================= */
/* Utilisée pour les joues, le nez, etc.                         */

void drawSphereParam(float r, int slices, int stacks)
{
    for (int i = 0; i < stacks; i++)                 // Boucle sur les "bandes" en latitude
    {
        float phi1 = M_PI * i / stacks;              // Angle phi inférieur
        float phi2 = M_PI * (i + 1) / stacks;        // Angle phi supérieur

        glBegin(GL_TRIANGLE_STRIP);                  // On dessine une bande en triangles
        for (int j = 0; j <= slices; j++)            // Boucle sur la longitude (theta)
        {
            float theta = 2 * M_PI * j / slices;     // Angle theta courant

            // Point 1 sur la bande
            float x1 = r * sin(phi1) * cos(theta);   // Coordonnée x du point 1
            float y1 = r * cos(phi1);                // Coordonnée y du point 1
            float z1 = r * sin(phi1) * sin(theta);   // Coordonnée z du point 1

            // Point 2 sur la bande suivante
            float x2 = r * sin(phi2) * cos(theta);   // Coordonnée x du point 2
            float y2 = r * cos(phi2);                // Coordonnée y du point 2
            float z2 = r * sin(phi2) * sin(theta);   // Coordonnée z du point 2

            glNormal3f(x1 / r, y1 / r, z1 / r);      // Normale au point 1 (normalisée)
            glVertex3f(x1, y1, z1);                  // Sommet 1

            glNormal3f(x2 / r, y2 / r, z2 / r);      // Normale au point 2
            glVertex3f(x2, y2, z2);                  // Sommet 2
        }
        glEnd();                                     // Fin de la bande
    }
}


/* ============================================================= */
/*    SPHÈRE PARAMÉTRIQUE TEXTURÉE (CORPS, BRAS, PIEDS, YEUX)    */
/* ============================================================= */

void drawSphereParamTextured(float r, int slices, int stacks)
{
    glEnable(GL_TEXTURE_2D);                        // Active le texturage 2D

    for (int i = 0; i < stacks; i++)                // Boucle sur les "bandes" en latitude
    {
        float phi1 = M_PI * i / stacks;             // Angle phi inférieur
        float phi2 = M_PI * (i + 1) / stacks;       // Angle phi supérieur

        glBegin(GL_TRIANGLE_STRIP);                 // Début de la bande
        for (int j = 0; j <= slices; j++)           // Boucle sur la longitude (theta)
        {
            float theta = 2 * M_PI * j / slices;    // Angle theta

            // Point 1
            float x1 = r * sin(phi1) * cos(theta);  // Coordonnée x du point 1
            float y1 = r * cos(phi1);               // Coordonnée y du point 1
            float z1 = r * sin(phi1) * sin(theta);  // Coordonnée z du point 1

            // Point 2
            float x2 = r * sin(phi2) * cos(theta);  // Coordonnée x du point 2
            float y2 = r * cos(phi2);               // Coordonnée y du point 2
            float z2 = r * sin(phi2) * sin(theta);  // Coordonnée z du point 2

            glNormal3f(x1 / r, y1 / r, z1 / r);     // Normale au point 1
            glTexCoord2f((float)j / slices, (float)i / stacks);   // Coordonnée texture pour point 1
            glVertex3f(x1, y1, z1);                 // Sommet 1

            glNormal3f(x2 / r, y2 / r, z2 / r);     // Normale au point 2
            glTexCoord2f((float)j / slices, (float)(i + 1) / stacks); // Coordonnée texture pour point 2
            glVertex3f(x2, y2, z2);                 // Sommet 2
        }
        glEnd();                                    // Fin de la bande
    }

    glDisable(GL_TEXTURE_2D);                       // Désactive le texturage 2D
}


/* ============================================================================================================================================= */
/*     CYLINDRE PARAMÉTRIQUE (NON TEXTURÉ, NON UTILISÉ ICI, DE BASE UTILISEE POUR OREILLES MAIS ENSUITE CREATION DrawCylinderParamtextured)      */
/* ============================================================================================================================================= */

void drawCylinderParam(float rBase, float rTop, float h, int slices)
{
    glBegin(GL_QUAD_STRIP);                         // Dessine une bande de quadrilatères
    for (int i = 0; i <= slices; i++)               // Parcourt la circonférence
    {
        float theta = 2 * M_PI * i / slices;        // Angle courant
        float x = cos(theta);                       // Coordonnée x sur le cercle unité
        float z = sin(theta);                       // Coordonnée z sur le cercle unité

        glNormal3f(x, 0, z);                        // Normale radiale (parallèle au vecteur (x,0,z))
        glVertex3f(rTop * x, h, rTop * z);          // Sommet en haut du cylindre
        glVertex3f(rBase * x, 0, rBase * z);        // Sommet en bas du cylindre
    }
    glEnd();                                        // Fin du cylindre
}


/* ============================================================= */
/*      CYLINDRE PARAMÉTRIQUE TEXTURÉ (UTILISÉ POUR OREILLES)    */
/* ============================================================= */

void drawCylinderParamTextured(float rBase, float rTop, float h, int slices)
{
    glEnable(GL_TEXTURE_2D);                        // Active la texture
    glBindTexture(GL_TEXTURE_2D, texCorps);         // Utilise la texture du corps (jaune)

    glBegin(GL_QUAD_STRIP);                         // Dessine un ruban de quadrilatères
    for (int i = 0; i <= slices; i++)               // Boucle angulaire
    {
        float theta = 2 * M_PI * i / slices;        // Angle courant
        float x = cos(theta);                       // Coordonnée x sur le cercle
        float z = sin(theta);                       // Coordonnée z sur le cercle

        glNormal3f(x, 0, z);                        // Normale pour l’éclairage

        float s = (float)i / slices;                // Coordonnée U de la texture (0 → 1 sur le tour)
        glTexCoord2f(s, 0.0f); glVertex3f(rBase * x, 0.0f, rBase * z); // Bas du cylindre
        glTexCoord2f(s, 1.0f); glVertex3f(rTop * x, h,    rTop * z);   // Haut du cylindre
    }
    glEnd();                                        // Fin du cylindre

    glDisable(GL_TEXTURE_2D);                       // Désactive le texturage
}


/* ============================================================= */
/*                         DESSIN DE PIKACHU                      */
/* ============================================================= */
/* Compose Pikachu à partir de sphères et cylindres texturés :   */
/* - Pieds, corps, bras, tête, oreilles, queue, yeux, joues, nez */
/* - Animation : translation verticale selon jumpHeight          */
/* ============================================================= */

void drawPikachu()
{
    glPushMatrix();                                  // Sauvegarde la matrice courante
        glTranslatef(0.0f, jumpHeight, 0.0f);        // Applique le déplacement vertical du saut

        float rPied = 0.13f;                         // Rayon des pieds
        float yPied = rPied;                         // Position en Y du centre du pied (reposant sur y=0)

        /* Pieds texturés */
        glBindTexture(GL_TEXTURE_2D, texCorps);      // Utilise la texture du corps pour les pieds

        glPushMatrix();                              // Pied gauche
            glTranslatef(-0.25f, yPied, 0.0f);       // Décale vers la gauche
            drawSphereParamTextured(rPied, 30, 30);  // Dessine la sphère texturée pour le pied
        glPopMatrix();                               // Restaure la matrice

        glPushMatrix();                              // Pied droit
            glTranslatef(0.25f, yPied, 0.0f);        // Décale vers la droite
            drawSphereParamTextured(rPied, 30, 30);  // Dessine la sphère texturée
        glPopMatrix();                               // Restaure la matrice

        /* Corps texturé */
        float rCorps = 0.35f;                        // Rayon de base du corps
        float yCorps = yPied + rPied + rCorps - 0.15f; // Position en Y du centre du corps (légèrement descendu)

        glPushMatrix();
            glTranslatef(0.0f, yCorps, 0.0f);        // Place le corps au-dessus des pieds
            glScalef(1.0f, 1.2f, 1.0f);              // Étire légèrement le corps en hauteur
            drawSphereParamTextured(rCorps, 40, 40); // Sphère texturée pour le corps
        glPopMatrix();

        /* Bras texturés */
        float rBras = 0.10f;                         // Rayon "de base" pour le bras
        float yBras = yCorps + 0.10f;                // Position en Y approximative des bras
        float zBras = 0.15f;                         // Décalage en Z (vers l’avant)
        float xOffset = 0.25f;                       // Décalage en X pour bras gauche/droit
        float longueurBras = 2.10f;                  // Facteur d’étirement pour la longueur des bras

        glPushMatrix();                              // Bras gauche
            glTranslatef(-xOffset, yBras, zBras);    // Place le bras à gauche et en avant
            glRotatef(-60, 1, 0, 0);                 // Incline le bras vers l’avant
            glScalef(1.0f, longueurBras, 1.0f);      // Étire la sphère en forme de bras
            drawSphereParamTextured(rBras, 30, 30);  // Dessine le bras
        glPopMatrix();

        glPushMatrix();                              // Bras droit
            glTranslatef(xOffset, yBras, zBras);     // Place le bras à droite et en avant
            glRotatef(-60, 1, 0, 0);                 // Même inclinaison
            glScalef(1.0f, longueurBras, 1.0f);      // Même étirement
            drawSphereParamTextured(rBras, 30, 30);  // Dessine le bras
        glPopMatrix();

        /* Tête texturée */
        float rTete = 0.25f;                         // Rayon de la tête
        float yTete = yCorps + (rCorps*1.2f) + rTete - 0.10f; // Position en Y de la tête

        glPushMatrix();
            glTranslatef(0.0f, yTete, 0.0f);         // Place la tête au-dessus du corps
            drawSphereParamTextured(rTete, 40, 40);  // Dessine la tête
        glPopMatrix();

        /* Oreilles texturées */
        float hOreille = 0.32f;                      // Hauteur des oreilles
        float rBaseO = 0.08f;                        // Rayon à la base
        float rTopO = 0.005f;                        // Rayon au sommet (presque pointu)
        float yOreille = yTete + rTete - 0.04f;      // Position en Y du point de départ des oreilles

        // Inclinaison dépendant de la hauteur du saut pour un effet "mou" des oreilles
        float angleOreille = 8.0f * (jumpHeight / jumpAmplitude); // Plus Pikachu monte, plus les oreilles penchent

        glPushMatrix();                              // Oreille gauche
            glTranslatef(-0.14f, yOreille, 0.0f);    // Place l’oreille gauche
            glRotatef(35 + angleOreille, 0, 0, 1);   // Incline l’oreille (dépend du saut)
            glRotatef(10,1,0,0);                     // Légère rotation vers l’avant
            glTranslatef(0,-0.03f,0);                // Ajustement de position
            drawCylinderParamTextured(rBaseO, rTopO, hOreille, 20); // Dessine l’oreille
        glPopMatrix();

        glPushMatrix();                              // Oreille droite
            glTranslatef(0.14f, yOreille, 0.0f);     // Place l’oreille droite
            glRotatef(-35 - angleOreille, 0, 0, 1);  // Inclinaison inversée
            glRotatef(10,1,0,0);                     // Légère rotation vers l’avant
            glTranslatef(0,-0.03f,0);                // Ajustement de position
            drawCylinderParamTextured(rBaseO, rTopO, hOreille, 20); // Dessine l’oreille
        glPopMatrix();

        /* Queue texturée */
        float rQueue = 0.10f;                        // Rayon de base de la queue
        float longueurQueue = 4.0f;                  // Étirement pour la queue
        float yQueue = yPied + 0.6f;                 // Position en Y de la queue
        float zQueue = -0.40f;                       // Position en Z (vers l’arrière)

        glPushMatrix();
            glTranslatef(0.0f, yQueue, zQueue);      // Place la queue
            glRotatef(-15,1,0,0);                    // Légère inclinaison
            glScalef(1.0f, longueurQueue, 1.0f);     // Étire pour former une queue
            drawSphereParamTextured(rQueue,30,30);   // Dessine la queue
        glPopMatrix();

        /* Yeux texturés */
        float rOeil = 0.05f;                         // Rayon des yeux
        float xOeil = 0.12f;                         // Décalage horizontal des yeux
        float yOeil = yTete + 0.05f;                 // Hauteur des yeux
        float zOeil = 0.20f;                         // Décalage en Z (vers l’avant)

        glBindTexture(GL_TEXTURE_2D, texYeux);       // Utilise la texture d’yeux

        glPushMatrix();                              // Œil gauche
            glTranslatef(-xOeil, yOeil, zOeil);      // Place l’œil gauche
            drawSphereParamTextured(rOeil, 30, 30);  // Dessine l’œil
        glPopMatrix();

        glPushMatrix();                              // Œil droit
            glTranslatef(xOeil, yOeil, zOeil);       // Place l’œil droit
            drawSphereParamTextured(rOeil, 30, 30);  // Dessine l’œil
        glPopMatrix();

        /* Joues colorées (non texturées, juste couleur) */
        float rJoue = 0.06f;                         // Rayon des joues
        float yJoue = yTete-0.05f;                   // Hauteur des joues
        float zJoue = 0.15f;                         // Décalage en Z (avant)

        glDisable(GL_LIGHTING);                      // Désactive l’éclairage pour garder la couleur vive
        if(jouesRouges)                              // Selon l’état du booléen jouesRouges
            glColor3f(1.0f,0.0f,0.0f);               // Joues rouges
        else
            glColor3f(1.0f,0.5f,0.8f);               // Joues rosées

        glPushMatrix();                              // Joue gauche
            glTranslatef(-xOeil,yJoue,zJoue);        // Position de la joue gauche
            drawSphereParam(rJoue,20,20);            // Sphère pleine couleur
        glPopMatrix();

        glPushMatrix();                              // Joue droite
            glTranslatef(xOeil,yJoue,zJoue);         // Position de la joue droite
            drawSphereParam(rJoue,20,20);            // Sphère pleine couleur
        glPopMatrix();

        glEnable(GL_LIGHTING);                       // Réactive l’éclairage

        /* Nez */
        float rNez=0.02f;                            // Rayon du nez
        float yNez=yTete-0.02f;                      // Hauteur du nez
        float zNez=0.26f;                            // Décalage en Z

        glDisable(GL_LIGHTING);                      // Couleur pure noire
        glColor3f(0.0f, 0.0f, 0.0f);                 // Nez noir

        glPushMatrix();
            glTranslatef(0.0f, yNez, zNez);          // Place le nez au centre du visage
            drawSphereParam(rNez,20,20);             // Dessine une petite sphère
        glPopMatrix();

        glEnable(GL_LIGHTING);                       // Réactive l’éclairage
        /* Reflet sur le nez avec point (primitive)*/
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);   // Blanc
        glPointSize(4.0f);            // Petit point

        glBegin(GL_POINTS);
            glVertex3f(0.0f, yNez + 0.008f, zNez + 0.022f); // Avancé sur la sphère
        glEnd();

        glEnable(GL_LIGHTING);

        /* Bouche (ligne épaisse) */
        glDisable(GL_LIGHTING);                      // Couleur pure noire
        glColor3f(0.0f,0.0f,0.0f);                   // Bouche noire
        glLineWidth(7.0f);                           // Épaisseur de ligne augmentée

        glBegin(GL_LINE_STRIP);                      // Dessine une ligne brisée (sourire)
            glVertex3f(-0.06f,yTete-0.08f,0.25f);    // Coin gauche de la bouche
            glVertex3f(0.0f,yTete-0.10f,0.26f);      // Centre de la bouche
            glVertex3f(0.06f,yTete-0.08f,0.25f);     // Coin droit de la bouche
        glEnd();

        glLineWidth(1.0f);                           // Remet l’épaisseur à la valeur par défaut
        glEnable(GL_LIGHTING);                       // Réactive l’éclairage

    glPopMatrix();                                   // Restaure la matrice globale (sans saut)
}


/* ============================================================= */
/*                      GESTION DU CLAVIER                       */
/* ============================================================= */
/* z/Z = zoom, p/f/s = modes de polygone, F = joues, r = rotation */
/* q = quitter                                                  */
/* ============================================================= */

void clavier(unsigned char touche, int x, int y)
{
    switch(touche)                                   // Analyse la touche appuyée
    {
        case 'z':                                   // Zoom avant
            camera_distance-=0.1f;                 // Diminue la distance caméra
            if(camera_distance<1.0f)               // Limite minimale
                camera_distance=1.0f;
            break;

        case 'Z':                                  // Zoom arrière
            camera_distance+=0.1f;                 // Augmente la distance caméra
            break;

        case 'p':                                  // Mode remplissage (polygones pleins)
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
            break;

        case 'f':                                  // Mode fil de fer (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            break;

        case 's':                                  // Mode points
            glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
            break;

        case 'F':                                  // Inverse la couleur des joues
            jouesRouges = !jouesRouges;
            break;

        case 'r':                                  // Active/désactive la rotation automatique
            idleEnabled = !idleEnabled;
            break;

        case 'q':                                  // Quitte le programme
            exit(0);
            break;
    }
    glutPostRedisplay();                           // Demande un réaffichage
}


/* ============================================================= */
/*            TOUCHES SPÉCIALES (FLÈCHES POUR LA CAMÉRA)         */
/* ============================================================= */

void specialKeys(int key,int x,int y)
{
    switch(key)                                    // Vérifie quelle touche spéciale est pressée
    {
        case GLUT_KEY_LEFT:                       // Flèche gauche → tourner la caméra à gauche
            camera_angleY-=5;
            break;

        case GLUT_KEY_RIGHT:                      // Flèche droite → tourner à droite
            camera_angleY+=5;
            break;

        case GLUT_KEY_UP:                         // Flèche haut → lever la caméra
            camera_angleX+=5;
            if(camera_angleX>89)                  // Limite pour ne pas passer "derrière"
                camera_angleX=89;
            break;

        case GLUT_KEY_DOWN:                       // Flèche bas → baisser la caméra
            camera_angleX-=5;
            if(camera_angleX<-89)                 // Limite inférieure
                camera_angleX=-89;
            break;
    }
    glutPostRedisplay();                          // Redessine avec la nouvelle orientation
}


/* ============================================================= */
/*              REDIMENSIONNEMENT DE LA FENÊTRE GLUT             */
/* ============================================================= */

void reshape(int w,int h)
{
    glViewport(0,0,w,h);                          // Définit la zone d’affichage (viewport)
    glMatrixMode(GL_PROJECTION);                  // Passe en mode projection
    glLoadIdentity();                             // Réinitialise la matrice de projection
    gluPerspective(60,(float)w/h,0.1,50);         // Définit une projection en perspective
}


/* ============================================================= */
/*                         GESTION DE LA SOURIS                  */
/* ============================================================= */

void mouse(int button,int state,int x,int y)
{
    if(button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) // Si clic gauche enfoncé
    {
        presse=1;                                   // Indique que la souris est pressée
        xold=x;                                     // Sauvegarde la position X de départ
        yold=y;                                     // Sauvegarde la position Y de départ
    }
    if(button==GLUT_LEFT_BUTTON && state==GLUT_UP)  // Si clic gauche relâché
    {
        presse=0;                                   // On indique que la souris n’est plus pressée
    }
}

void mousemotion(int x,int y)
{
    if(presse)                                      // Si le bouton est enfoncé
    {
        anglex+=(x-xold);                           // Met à jour anglex en fonction du déplacement
        angley+=(y-yold);                           // Met à jour angley en fonction du déplacement
        glutPostRedisplay();                        // Demande un réaffichage (si jamais utilisé)
    }
    xold=x;                                         // Mémorise la nouvelle position X
    yold=y;                                         // Mémorise la nouvelle position Y
}


/* ============================================================= */
/*                           ANIMATION (IDLE)                    */
/* ============================================================= */
/* - Gère la rotation automatique de la caméra si idleEnabled    */
/* - Met à jour jumpTime / jumpHeight pour le mouvement de saut  */
/* ============================================================= */

void idle()
{
    if(idleEnabled)                                 // Si l’animation de rotation est active
    {
        camera_angleY+=0.5f;                        // Fait tourner la caméra autour de Y
        if(camera_angleY>360.0f)                    // Si on dépasse 360°
            camera_angleY-=360.0f;                  // On recale dans [0,360)
        glutPostRedisplay();                        // Redessine la scène
    }

    // Animation automatique du saut
    jumpTime += 0.025f;                             // Incrémente le temps (pas de temps fixe approx.)
    if(jumpTime > jumpDuration)                     // Si on dépasse la durée d’un cycle
        jumpTime -= jumpDuration;                   // On "rembobine" dans [0, jumpDuration]

    // Hauteur du saut : utilisation d’un sinus pour un mouvement doux (0 → pi)
    jumpHeight = jumpAmplitude * sin(M_PI * jumpTime / jumpDuration); // Montée puis descente

    glutPostRedisplay();                            // Demande le réaffichage avec la nouvelle hauteur
}


/* ============================================================= */
/*              CHARGEMENT D’UNE IMAGE JPEG AVEC LIBJPEG         */
/* ============================================================= */
/* Charge une image 256x256 dans un buffer (format RGB)          */
/* ============================================================= */

void loadJpegImage(const char *filename, unsigned char *buffer)
{
    struct jpeg_decompress_struct cinfo;            // Structure principale de libjpeg
    struct jpeg_error_mgr jerr;                     // Gestionnaire d’erreurs
    FILE *file;                                     // Pointeur de fichier
    unsigned char *ligne;                           // Pointeur vers une ligne de l’image

    cinfo.err = jpeg_std_error(&jerr);              // Initialise la structure d’erreur
    jpeg_create_decompress(&cinfo);                 // Initialise la décompression JPEG

    file = fopen(filename, "rb");                   // Ouvre le fichier en lecture binaire
    if(!file)                                       // Si l’ouverture échoue
    {
        cerr << "Erreur : impossible d'ouvrir " << filename << endl; // Message d’erreur
        exit(1);                                    // Quitte le programme
    }

    jpeg_stdio_src(&cinfo, file);                   // Associe le fichier à la structure libjpeg
    jpeg_read_header(&cinfo, TRUE);                 // Lit l’entête JPEG

    if(cinfo.image_width != 256 || cinfo.image_height != 256) // Vérifie la taille de l’image
    {
        cerr << "Erreur : " << filename << " doit être en 256x256." << endl; // Erreur si taille incorrecte
        exit(1);                                    // Quitte le programme
    }

    jpeg_start_decompress(&cinfo);                  // Lance la décompression

    while(cinfo.output_scanline < cinfo.output_height) // Tant qu’il reste des lignes à lire
    {
        ligne = buffer + 3 * 256 * cinfo.output_scanline; // Pointeur vers la ligne à remplir
        jpeg_read_scanlines(&cinfo, &ligne, 1);           // Lit une ligne de pixels
    }

    jpeg_finish_decompress(&cinfo);                 // Termine la décompression
    jpeg_destroy_decompress(&cinfo);                // Libère les ressources libjpeg
    fclose(file);                                   // Ferme le fichier ouvert
}