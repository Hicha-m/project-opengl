# Projet OpenGL — espace et cinématique

## Organisation

```text
projet/
├── Makefile
├── README.md
├── src/
│   ├── main.cpp
│   ├── Application.h / Application.cpp
│   ├── camera/
│   │   ├── Camera.h / Camera.cpp
│   │   └── CinematicCamera.h / CinematicCamera.cpp
│   ├── graphics/
│   │   ├── Mesh.h / Mesh.cpp
│   │   ├── ShaderProgram.h / ShaderProgram.cpp
│   │   ├── Texture2D.h / Texture2D.cpp
│   │   └── Renderer.h / Renderer.cpp
│   ├── geometry/
│   │   └── Sphere.h / Sphere.cpp
│   ├── scene/
│   │   ├── Scene.h
│   │   ├── SceneObject.h
│   │   ├── Transform.h
│   │   ├── Material.h
│   │   ├── LightManager.h / LightManager.cpp
│   │   ├── SceneResources.h
│   │   └── SceneSetup.h / SceneSetup.cpp
│   ├── animation/
│   │   ├── Keyframe.h
│   │   ├── Easing.h / Easing.cpp
│   │   ├── AnimationTrack.h
│   │   ├── Timeline.h / Timeline.cpp
│   │   ├── TimelineTrack.h
│   │   ├── CameraTrack.h
│   │   ├── TransformTrack.h
│   │   └── EventTrack.h
│   ├── systems/
│   │   ├── Meteor.h
│   │   ├── MeteorSystem.h / MeteorSystem.cpp
│   │   ├── MeteorResources.h / MeteorResources.cpp
│   │   └── MeteorShower.h / MeteorShower.cpp
│   └── cinematic/
│       └── MainSequence.h / MainSequence.cpp
├── shaders/                  # programmes GLSL exécutés par le GPU
├── textures/                 # images de la Terre, du Soleil et de l'espace
├── tests/
│   ├── timeline.cpp
│   ├── main_sequence.cpp
│   ├── meteor_system.cpp
│   ├── meteor_shower.cpp
│   └── application.cpp
├── build/                    # objets, dépendances et tests compilés, ignorés par Git
└── project                   # exécutable généré
```

Un `.h` expose les classes et fonctions utilisables depuis d'autres fichiers.
Le `.cpp` correspondant contient leur implémentation. Les petites classes et
les templates comme `AnimationTrack<T>` sont entièrement définis dans leur `.h`.

## Rôle de chaque fichier

| Fichier ou paire `.h` / `.cpp` | Ce qu'il fait |
| --- | --- |
| `src/main.cpp` | Crée `Application`, appelle `init()` puis `run()`. Le destructeur ferme l'application. |
| `src/Application` | Possède la fenêtre et les systèmes du moteur. Initialise OpenGL, traite clavier/souris, avance la Timeline, appelle le Renderer, calcule les FPS et gère la fermeture. |
| `camera/Camera` | Définit la base `Camera` et ses vecteurs de vue. Contient aussi `FPSCamera` (déplacement libre) et `OrbitCamera` (rotation autour d'une cible). |
| `camera/CinematicCamera` | Évalue les pistes de position, de cible et de FOV, ou les pistes d'orbite (rayon, yaw, pitch, cible), pour calculer la caméra à un instant donné. |
| `graphics/Mesh` | Stocke les sommets (positions, normales, UV, tangentes), charge un OBJ, crée les buffers OpenGL et dessine la géométrie. |
| `graphics/ShaderProgram` | Lit, compile et lie les shaders GLSL. Active le programme et transmet les uniforms, par exemple les matrices et les paramètres de lumière. |
| `graphics/Texture2D` | Charge une image avec stb_image, crée la texture OpenGL et la lie à une unité de texture. |
| `graphics/Renderer` | Parcourt les objets de la scène. `renderMesh` permet aussi de dessiner un mesh et un matériau partagés avec une transformation indépendante. Applique leurs matériaux, textures, uniforms et états OpenGL, puis appelle leur mesh. |
| `geometry/Sphere` | Génère les sommets d'une sphère avec normales, UV et tangentes, puis les fournit à un `Mesh`. |
| `scene/Scene.h` | Contient les objets et permet de les ajouter ou de les retrouver par nom/index. |
| `scene/SceneObject.h` | Représente un objet nommé : une transformation, un mesh et un matériau. |
| `scene/Transform.h` | Stocke position, rotation et échelle. Produit la matrice de modèle utilisée pour placer l'objet. Les rotations sont en radians. |
| `scene/Material.h` | Décrit l'apparence et les états de rendu : shader, textures, uniforms, transparence, profondeur et réception de lumière. |
| `scene/LightManager` | Stocke une lumière directionnelle et les lumières ponctuelles, puis transmet leurs paramètres aux shaders. |
| `scene/SceneResources.h` | Possède les shaders, textures et sphères de la démonstration. Les objets empruntent ces ressources sans les posséder. |
| `scene/SceneSetup` | Charge les ressources et construit Terre, nuages, Soleil, étoiles et lumière. Maintient aussi les étoiles autour de la caméra. |
| `animation/Keyframe.h` | Définit une clé : instant, valeur et easing du segment suivant. |
| `animation/Easing` | Définit les courbes Linear, EaseIn, EaseOut et EaseInOut utilisées pour accélérer ou ralentir les interpolations. |
| `animation/AnimationTrack.h` | Stocke des clés d'une valeur (`float`, `glm::vec3`, etc.) et calcule la valeur interpolée à un instant donné. Ajouter les clés par temps croissant. |
| `animation/TimelineTrack.h` | Définit l'interface commune des pistes exécutables : `update(previousTime, time)` et `reset(time)`. |
| `animation/Timeline` | Possède et exécute les pistes, gère le temps, la durée, la lecture, la pause et le retour à zéro. Ne connaît pas le contenu du film. |
| `animation/CameraTrack.h` | Relie la Timeline à `CinematicCamera` pour évaluer la caméra au temps courant. |
| `animation/TransformTrack.h` | Anime position, rotation et échelle d'une transformation. Un résolveur permet de retrouver un objet même après réallocation du vecteur de scène. |
| `animation/EventTrack.h` | Déclenche des callbacks aux instants prévus, une fois par passage, même si une frame traverse plusieurs événements. |
| `cinematic/MainSequence` | Décrit le film de 30 secondes : caméra, rotations, configuration de pluie, événements start/stop à 10/20 secondes et reset de son état. |
| `systems/Meteor.h` | Données de chaque instance vivante : transformation, vitesse linéaire et durée de vie restante. |
| `systems/MeteorSystem` | Possède la population, expose spawn/update/clear et dessine les instances avec les ressources communes, sans créer de SceneObject. |
| `systems/MeteorResources` | Possède une sphère peu détaillée, un shader, la texture lunaire réutilisée et un seul matériau pour toute la population. |
| `systems/MeteorShower` | Générateur CPU indépendant : boîte de spawn, direction avec dispersion conique, cadence par seconde, plages de paramètres et seed reproductible. |
| `tests/meteor_shower.cpp` | Vérifie start/stop, validation, plages, dispersion, seed et cadence à 30/60/144 FPS. |
| `tests/meteor_system.cpp` | Vérifie sans OpenGL le mouvement indépendant, les expirations, les entrées invalides, clear et 1 000 instances. |
| `tests/timeline.cpp` | Vérifie les pistes, la pause, la reprise, la fin, les événements et la relecture. |
| `tests/main_sequence.cpp` | Vérifie les paramètres de la séquence complète et les bindings après ajout d'objets. |
| `tests/application.cpp` | Vérifie le chargement réel, le rendu OpenGL de la séquence et des météores, la fermeture, la réinitialisation et la récupération après shaders/textures absents. Exporte quatorze captures dans `/tmp`. |
| `Makefile` | Compile et lie l'application et les tests, suit les dépendances entre headers et sources, lance l'application ou nettoie les fichiers générés. |
| `.gitignore` | Exclut notamment l'exécutable et le dossier de compilation `build/` du suivi Git. |

Les chemins `camera/`, `graphics/`, etc. de ce tableau sont relatifs à `src/`.

## Shaders et textures

Chaque paire de shaders comporte un `.vert` qui traite les sommets et un `.frag`
qui calcule la couleur des fragments :

- `earth.vert` / `earth.frag` : Terre, éclairage jour/nuit, normal map et spéculaire.
- `clouds.vert` / `clouds.frag` : couche de nuages avec transparence et éclairage.
- `sun.vert` / `sun.frag` : surface lumineuse du Soleil.
- `stars.vert` / `stars.frag` : fond étoilé.
- `meteor.vert` / `meteor.frag` : roche texturée avec éclairage directionnel et ambiant.

`textures/earth/` contient les cartes jour, nuit, nuages, normales et spéculaire ;
`textures/sun/` la surface du Soleil ; `textures/space/` le ciel étoilé.

## Comment les systèmes travaillent ensemble

`Application` demande à `SceneSetup` de construire le monde et à `MainSequence`
de configurer le film. À chaque frame, elle traite les entrées, avance `Timeline`
(qui met à jour la caméra et les transformations), avance aussi la simulation de
`MeteorSystem`, puis la génération de `MeteorShower`, et demande à `Renderer`
de dessiner `Scene` et à `MeteorSystem` de dessiner sa population avec les mêmes
lumières et matrices de caméra.

Pour changer un mouvement ou un événement, modifier `MainSequence`. Pour changer
les objets, leurs textures ou leurs matériaux, modifier `SceneSetup` et, si
nécessaire, `SceneResources`. Pour changer le fonctionnement du rendu, modifier
`Renderer` ou les shaders.

Les ressources GPU sont construites après la création du contexte OpenGL.
À la fermeture, `Application` libère les pistes, les objets et les ressources
avant de détruire la fenêtre et de terminer GLFW.

## Compilation et inclusions

Toutes les inclusions internes partent de `src/`, fourni par `-Isrc` :

```cpp
#include "camera/Camera.h"
#include "graphics/Mesh.h"
#include "geometry/Sphere.h"
#include "animation/Timeline.h"
```

Les objets `.o`, dépendances `.d` et exécutables de test vont dans `build/`.
Les options `-MMD -MP` permettent à Make de recompiler les fichiers affectés
lorsqu'un header change. Pour ajouter un nouveau `.cpp` au moteur, l'ajouter à
`COMMON_SRC` dans le Makefile. Les nouveaux headers sont détectés par les inclusions.

Depuis `projet` (les chemins de shaders et textures sont relatifs à ce dossier) :

```sh
make                  # compile l'application, sans la lancer
make project          # même compilation
make run              # compile si nécessaire, puis lance l'application
make test             # tests Timeline, MainSequence, MeteorSystem et MeteorShower, sans fenêtre
make test-sequence    # seulement le test de séquence
make test-runtime     # test OpenGL masqué, nécessite un affichage X11
make clean            # supprime build/ et l'exécutable project
```