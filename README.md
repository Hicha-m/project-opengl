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
│   │   ├── Renderer.h / Renderer.cpp
│   │   └── ParticleRenderer.h / ParticleRenderer.cpp
│   ├── geometry/
│   │   ├── Sphere.h / Sphere.cpp
│   │   └── SphereCollider.h
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
│   │   ├── Meteor.h / MeteorImpact.h
│   │   ├── MeteorSystem.h / MeteorSystem.cpp
│   │   ├── MeteorResources.h / MeteorResources.cpp
│   │   ├── ImpactLight.h
│   │   ├── ImpactLightSystem.h / ImpactLightSystem.cpp
│   │   ├── MeteorShower.h / MeteorShower.cpp
│   │   ├── MeteorTrailEmitter.h / MeteorTrailEmitter.cpp
│   │   ├── EarthDamageSystem.h / EarthDamageSystem.cpp
│   │   ├── Particle.h
│   │   ├── ParticleSystem.h / ParticleSystem.cpp
│   │   ├── ParticleEmitter.h / ParticleEmitter.cpp
│   │   └── ImpactParticleEmitter.h / ImpactParticleEmitter.cpp
│   └── cinematic/
│       └── MainSequence.h / MainSequence.cpp
├── shaders/                  # programmes GLSL exécutés par le GPU
├── textures/                 # images de la Terre, du Soleil et de l'espace
├── tests/
│   ├── timeline.cpp
│   ├── main_sequence.cpp
│   ├── meteor_system.cpp
│   ├── meteor_shower.cpp
│   ├── meteor_collision.cpp
│   ├── earth_damage.cpp
│   ├── meteor_trail.cpp
│   ├── particle_system.cpp
│   ├── impact_light.cpp
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
| `scene/LightManager` | Stocke une lumière directionnelle, les lumières ponctuelles permanentes et temporaires. Transmet au shader les 32 plus intenses, avec ordre d'insertion conservé à intensité égale. |
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
| `systems/Meteor.h` | Données de chaque instance vivante : transformation, vitesse linéaire, durée de vie restante et MeteorId stable. |
| `systems/MeteorSystem` | Possède la population, simule les instances, détecte les contacts continus et expose les impacts de frame. Dessine avec les ressources communes, sans SceneObject par météore. |
| `systems/MeteorResources` | Possède une sphère peu détaillée, un shader, la texture lunaire réutilisée et un seul matériau pour toute la population. |
| `geometry/SphereCollider.h` | Centre et rayon monde, sans dépendance à la scène ou au rendu. |
| `systems/MeteorImpact.h` | Contact sur la cible, normale, vitesse et taille ; aucune logique d’effet. |
| `systems/ImpactLight.h` | Position, couleur, intensités, âge, durée et état de naissance d'un flash. |
| `systems/ImpactLightSystem` | Consomme les impacts sans connaître la Terre ni MeteorSystem. Crée les flashes, les fait décroître et publie les lumières temporaires au LightManager. |
| `systems/MeteorTrailEmitter` | Observe les météores en lecture seule, échantillonne leurs segments par distance et émet dans ParticleSystem ; état et RNG par MeteorId. |
| `tests/meteor_trail.cpp` | Vérifie densité à 30/60/144 FPS, grandes frames, vieillissement dans la frame, suppressions, réallocations et replay. |
| `systems/EarthDamageSystem` | Consomme les impacts, transforme leurs positions monde en UV locales et accumule les dégâts permanents et la chaleur temporaire dans deux cartes CPU. Refroidit la chaleur et envoie les cartes modifiées aux textures terrestres. |
| `tests/earth_damage.cpp` | Vérifie UV, rotation, accumulation, saturation, couture, pôles, refroidissement et replay sans contexte OpenGL. |
| `systems/Particle.h` | Données runtime : position, vitesse, taille monde, âge et durée de vie. |
| `systems/ParticleSystem` | Stocke, déplace et expire les particules sur CPU ; délègue le rendu sans dépendre des météores ou de SceneObject. |
| `systems/ParticleEmitter` | Burst générique dans un cône : nombre, vitesse, taille, lifetime et seed configurables ; RNG réinitialisable. |
| `systems/ImpactParticleEmitter` | Consomme les MeteorImpact et demande un burst à l'émetteur générique suivant la normale extérieure. |
| `graphics/ParticleRenderer` | Un buffer d'instances et un draw call de billboards orientés caméra pour toute la population ; restaure les états OpenGL. |
| `tests/particle_system.cpp` | Vérifie simulation, expiration, cône, impacts simultanés, plages, seed et reset jusqu'à 10 000 particules. |
| `tests/impact_light.cpp` | Vérifie la première frame, la décroissance, l'expiration, les impacts simultanés, la sélection GPU et la relecture. |
| `tests/meteor_collision.cpp` | Vérifie les contacts continus, le tunneling, les lifetimes et les impacts multiples. |
| `systems/MeteorShower` | Générateur CPU indépendant : boîte de spawn, direction avec dispersion conique, cadence par seconde, plages de paramètres et seed reproductible. |
| `tests/meteor_shower.cpp` | Vérifie start/stop, validation, plages, dispersion, seed et cadence à 30/60/144 FPS. |
| `tests/meteor_system.cpp` | Vérifie sans OpenGL le mouvement indépendant, les expirations, les entrées invalides, clear et 1 000 instances. |
| `tests/timeline.cpp` | Vérifie les pistes, la pause, la reprise, la fin, les événements et la relecture. |
| `tests/main_sequence.cpp` | Vérifie les paramètres de la séquence complète et les bindings après ajout d'objets. |
| `tests/application.cpp` | Vérifie le chargement réel, le rendu OpenGL, les flashes sur la Terre, la limite GPU, la relecture des lumières et des images, les particules instanciées jusqu’à 10 000, l’occlusion, le reset et la récupération après ressources absentes. Exporte les captures dans `/tmp`. |
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
- `particle.vert` / `particle.frag` : billboards instanciés lumineux avec disparition progressive.
- `meteor.vert` / `meteor.frag` : roche texturée avec éclairage directionnel et ambiant.

`textures/earth/` contient les cartes jour, nuit, nuages, normales et spéculaire ;
`textures/sun/` la surface du Soleil ; `textures/space/` le ciel étoilé.

## Comment les systèmes travaillent ensemble

`Application` demande à `SceneSetup` de construire le monde et à `MainSequence`
de configurer le film. À chaque frame, elle traite les entrées, avance `Timeline`
(qui met à jour la caméra et les transformations), avance aussi la simulation de
`MeteorSystem` (après enregistrement des nouveaux météores par l’émetteur de
traînées), avance les particules existantes, émet les traînées puis consomme les impacts avec
`EarthDamageSystem`, `ImpactParticleEmitter` et `ImpactLightSystem`. La carte
de dégâts est envoyée à la texture terrestre après consommation. Elle met à jour et publie les
flashes, avance la génération de `MeteorShower`, puis dessine la scène, les
météores et les particules avec les mêmes matrices de caméra.

Chaque flash est placé à `impact.position + normal * 0.2` en coordonnées monde.
Son intensité initiale vaut `clamp(speed * meteorScale * 3, 2, 30)`, sa couleur
est orangée et sa durée vaut 0,5 seconde. La première mise à jour après sa
création conserve son âge à zéro, même si `dt` est grand : dans l'ordre
`consume -> update -> publish -> render`, il est présenté à son intensité
initiale. Les frames suivantes appliquent une décroissance quadratique
`initialIntensity * (1 - age / lifetime)^2`, puis suppriment le flash expiré.
L'atténuation ponctuelle (`constant = 1`, `linear = 1`, `quadratic = 2`) limite
l'éclairage au voisinage du contact. Au-delà de 32 lumières, seules les plus
intenses sont envoyées au GPU ; les autres continuent de vieillir sur le CPU.

`R` vide aussi les flashes et leur publication dans `LightManager`. Les lumières
permanentes restent présentes. `R` vide les particules et réinitialise aussi
le RNG de leur émetteur ainsi que les états de traînée et les identifiants des
météores. Il n'y a à cette phase ni bloom, ni cratère. Captures du test de contact : `/tmp/impact-flash-peak.ppm`,
`/tmp/impact-flash-faded.ppm` et `/tmp/impact-flash-expired.ppm`.

Chaque impact émet 48 fragments orangés à `impact.position + normal * 0.12`.
Le cône de 1,3 radian reste dans l'hémisphère extérieur ; les vitesses vont de
0,8 à 3 unités/s, les diamètres de 0,06 à 0,16 unité et les lifetimes de 0,6 à
1,4 seconde. La seed par défaut est 46. `ParticleBurstConfig` permet de changer
ces plages pour d'autres usages ; `ImpactParticleEmitter::configure` expose
aussi la seed et impose un cône contenu dans l'hémisphère extérieur. Le système
de particules ne connaît pas les impacts. Les particules naissent après la simulation de la frame, à l'âge zéro.

Le rendu construit un buffer compact position/taille/opacité puis appelle une
seule fois `glDrawArraysInstanced` (deux triangles par billboard). Le shader
travaille en espace caméra, sans texture. La transparence additive évite le tri,
le test de profondeur conserve l'occlusion par la Terre et les particules
n'écrivent pas dans le depth buffer. L'opacité décroît avec l'âge. Il n'y a pas
de collision des particules, gravité, fumée ou simulation de feu dans cette phase.

Les tests CPU et OpenGL couvrent 100, 1 000 et 10 000 particules, le mouvement,
l'expiration, l'occlusion, le reset et le replay des données et des images.
Captures : `/tmp/particles-100.ppm`, `/tmp/particles-1000.ppm`,
`/tmp/particles-10000.ppm`, `/tmp/impact-particles-birth.ppm` et
`/tmp/impact-particles-moved.ppm`. Ces tests valident le fonctionnement à ces
populations, sans constituer un benchmark FPS.

## Marques de dégâts terrestres — phase 4.8.1

`EarthDamageSystem` reçoit les `MeteorImpact`, la transformation actuelle de
la Terre et son rayon local. Il applique l'inverse de la matrice de modèle à
chaque position monde, puis calcule les UV conformes à `Sphere` :
`u = atan2(z, x) / (2π)` ramené dans `[0,1)`,
`v = 1 - acos(y / longueur) / π`. La carte reste en coordonnées locales :
les marques suivent ensuite la rotation de la Terre sans repeindre la texture.

La carte CPU de 512 × 256 valeurs contient des calottes circulaires mesurées
sur la sphère, avec un bord progressif. Leur rayon angulaire est
`clamp(atan2(2 * meteorScale, rayonMondeTerre), 0.025, 0.25)` radian. Chaque
passage ajoute jusqu'à 0,7 de dégâts, avec saturation à 1. La distance sphérique
traite naturellement la couture U et les pôles ; aucune duplication manuelle
ou déformation de disque en UV n'est nécessaire.

`SceneResources` possède la texture `earthDamageTexture` au format `GL_R8`,
liée au matériau terrestre sous `damageMap` sur l'unité 4. `Texture2D` expose
la création et la mise à jour d'une texture rouge dynamique. U utilise
`GL_REPEAT`, V utilise `GL_CLAMP_TO_EDGE`, avec filtrage linéaire sans mipmaps.
L'upload complet ne se produit que si la carte est marquée modifiée. Le shader
assombrit sa couleur finale vers un charbon brun, y compris les lumières
nocturnes et les reflets ; la carte vide conserve l'apparence initiale.

`R` remet la carte CPU à zéro et envoie immédiatement cette carte vide au GPU.
La fermeture efface aussi l'état CPU et libère la texture avec les autres
ressources avant la destruction du contexte. Aucun changement n'a été apporté
aux systèmes de météores, de flashes ou de particules pour gérer les dégâts.
La phase 4.8.1 conserve la géométrie ; la chaleur temporaire est ajoutée en 4.8.2.

`make test-runtime` vérifie les texels réellement envoyés au GPU,
l'assombrissement, l'accumulation, la couture, la disparition du marqueur de
la face visible après rotation et la restauration de l'image après reset.
Il compare aussi les cartes CPU et les images de deux relectures complètes.
Captures : `/tmp/earth-damage-before.ppm`, `/tmp/earth-damage-burned.ppm`
et `/tmp/earth-damage-accumulated.ppm`.

## Incandescence de surface — phase 4.8.2

`EarthDamageSystem` conserve désormais une `heatMap` temporaire à côté de la
`damageMap` permanente. Les deux cartes partagent la conversion monde/local/UV,
la résolution 512 × 256 et la distance sphérique, y compris aux pôles et à la
couture U. L'empreinte thermique est 1,5 fois plus large que la marque brûlée.
La chaleur initiale vaut `clamp(speed * meteorScale * 0.4, 0.8, 3)` et les
impacts proches s'additionnent jusqu'à un maximum de 6 par texel.

`update(dt)` refroidit la chaleur existante avant la consommation des nouveaux
impacts : `heat *= exp(-dt / 1.5)`. Les valeurs inférieures à 0,001 sont remises
à zéro. Les nouveaux impacts sont donc visibles à leur chaleur initiale, même
sur une frame longue. Le refroidissement ne modifie jamais la damage map.

`SceneResources` possède `earthHeatTexture`, au format `GL_R32F`, sur l'unité 5
sous le sampler `heatMap`. Ce format conserve l'accumulation au-delà de 1.
Chaque carte possède son propre indicateur de modification : une carte de dégâts
inchangée n'est pas renvoyée pendant le refroidissement. Les deux textures
utilisent les mêmes règles de wrapping et de filtrage.

Le shader ajoute l'émission thermique après l'éclairage et les dégâts. La
palette va du rouge au bord vers orange, jaune puis blanc-jaune au centre le
plus chaud. L'émission reste visible côté nuit sans Soleil ni flash ponctuel.
Les valeurs lumineuses dépassent 1 dans le shader et sont limitées par le
framebuffer actuel ; aucun pipeline HDR ou bloom n'est ajouté à cette phase.
Après refroidissement, seule la marque brûlée persiste.

`R` efface et renvoie les deux cartes au GPU. Les tests CPU vérifient le
refroidissement à 30/60/144 FPS, l'accumulation, les limites, la couture, la
rotation et le replay. Les tests OpenGL vérifient les valeurs thermiques
réellement uploadées, le centre jaune-blanc côté nuit, le refroidissement,
le réchauffement par un second impact, les dégâts après refroidissement et
le replay des deux cartes et des images de la cinématique.
Captures : `/tmp/earth-heat-night-peak.ppm`, `/tmp/earth-heat-night-cooled.ppm`,
`/tmp/earth-heat-night-reheated.ppm` et `/tmp/earth-heat-cold-burn.ppm`.

Les étapes suivantes restent successives : 4.8.3 ajoutera le niveau de
destruction global, 4.8.4 les fissures émissives et 4.8.5 la rupture avec
fragments préparés, noyau blanc, éclairage du noyau et bloom. Elles ne sont
pas implémentées par la phase 4.8.2.

Pour changer un mouvement ou un événement, modifier `MainSequence`. Pour changer
les objets, leurs textures ou leurs matériaux, modifier `SceneSetup` et, si
nécessaire, `SceneResources`. Pour changer le fonctionnement du rendu, modifier
`Renderer` ou les shaders.

Les ressources GPU sont construites après la création du contexte OpenGL.
À la fermeture, `Application` libère les pistes, les objets et les ressources
avant de détruire la fenêtre et de terminer GLFW.

## Compilation et inclusions

### Commandes

- `F3` : basculer entre la caméra cinématique et la caméra FPS. La caméra FPS
  démarre depuis la vue actuelle ; la cinématique continue pendant l'exploration.
- En mode FPS : souris pour regarder, `W` / `S` pour avancer / reculer,
  `A` / `D` pour aller à gauche / droite, `Z` / `X` pour monter / descendre.
- `G` / `H` : doubler / diviser par deux la vitesse de déplacement.
- `F1` : affichage filaire ; `F2` : informations de caméra ; `R` : relancer
  la cinématique ; `Échap` : quitter.

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
make test             # tests Timeline, séquence, météores, flashes, particules et dégâts, sans fenêtre
make test-sequence    # seulement le test de séquence
make test-runtime     # test OpenGL masqué, nécessite un affichage X11
make clean            # supprime build/ et l'exécutable project
```
