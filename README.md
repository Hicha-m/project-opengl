# Projet OpenGL — espace et cinématique

## Organisation

```text
projet/
├── Makefile
├── README.md
├── src/
│   ├── main.cpp
│   ├── Application.h / Application.cpp
│   ├── audio/
│   │   └── MusicPlayer.h / MusicPlayer.cpp
│   ├── camera/
│   │   ├── Camera.h / Camera.cpp
│   │   └── CinematicCamera.h / CinematicCamera.cpp
│   ├── graphics/
│   │   ├── Mesh.h / Mesh.cpp
│   │   ├── ShaderProgram.h / ShaderProgram.cpp
│   │   ├── Texture2D.h / Texture2D.cpp
│   │   ├── Renderer.h / Renderer.cpp
│   │   ├── ParticleRenderer.h / ParticleRenderer.cpp
│   │   └── HDRPipeline.h / HDRPipeline.cpp
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
│   │   ├── SolarSystem.h / SolarSystem.cpp
│   │   ├── MeteorResources.h / MeteorResources.cpp
│   │   ├── ImpactLight.h
│   │   ├── ImpactLightSystem.h / ImpactLightSystem.cpp
│   │   ├── MeteorShower.h / MeteorShower.cpp
│   │   ├── MeteorTrailEmitter.h / MeteorTrailEmitter.cpp
│   │   ├── EarthDamageSystem.h / EarthDamageSystem.cpp
│   │   ├── EarthBreakupSystem.h / EarthBreakupSystem.cpp
│   │   ├── Particle.h
│   │   ├── ParticleSystem.h / ParticleSystem.cpp
│   │   ├── ParticleEmitter.h / ParticleEmitter.cpp
│   │   └── ImpactParticleEmitter.h / ImpactParticleEmitter.cpp
│   └── cinematic/
│       └── MainSequence.h / MainSequence.cpp
├── shaders/                  # programmes GLSL exécutés par le GPU
├── textures/                 # images des planètes, de la Lune, du Soleil et de l'espace
├── music/                    # morceau MP3 fourni
├── tests/
│   ├── music_player.cpp
│   ├── solar_system.cpp
│   ├── timeline.cpp
│   ├── main_sequence.cpp
│   ├── meteor_system.cpp
│   ├── meteor_shower.cpp
│   ├── meteor_collision.cpp
│   ├── earth_breakup.cpp
│   ├── destruction_level.cpp
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
| `cinematic/MainSequence` | Décrit le film de 110,17 secondes : suivi de la Terre, bombardement, rupture, plan final puis recul sur le système solaire et la Voie lactée. |
| `audio/MusicPlayer` | Lecture SDL3 du morceau converti en PCM, horloge audio, rewind, volume et mute ; la cinématique suit le temps consommé par le flux. |
| `systems/SolarSystem` | Orbites circulaires comprimées des huit planètes, suivi des nuages et anneaux, orbite lunaire puis dérive à vitesse conservée après rupture. Aucun solveur gravitationnel. |
| `tests/solar_system.cpp` | Vérifie les rayons et vitesses orbitales, la dérive tangentielle de la Lune, la pose capturée de la Terre et le reset déterministe. |
| `systems/Meteor.h` | Données de chaque instance vivante : transformation, vitesse linéaire, durée de vie restante et MeteorId stable. |
| `systems/MeteorSystem` | Possède la population, simule les instances, détecte les contacts continus et expose les impacts de frame. Dessine avec les ressources communes, sans SceneObject par météore. |
| `systems/MeteorResources` | Possède une sphère peu détaillée, un shader, la texture lunaire réutilisée et un seul matériau pour toute la population. |
| `geometry/SphereCollider.h` | Centre et rayon monde, sans dépendance à la scène ou au rendu. |
| `systems/MeteorImpact.h` | Contact sur la cible, normale, vitesse et taille ; aucune logique d’effet. |
| `systems/ImpactLight.h` | Position, couleur, intensités, âge, durée et état de naissance d'un flash. |
| `systems/ImpactLightSystem` | Consomme les impacts sans connaître la Terre ni MeteorSystem. Crée les flashes, les fait décroître et publie les lumières temporaires au LightManager. |
| `systems/MeteorTrailEmitter` | Observe les météores en lecture seule, échantillonne leurs segments par distance et émet dans ParticleSystem ; état et RNG par MeteorId. |
| `tests/meteor_trail.cpp` | Vérifie densité à 30/60/144 FPS, grandes frames, vieillissement dans la frame, suppressions, réallocations et replay. |
| `graphics/HDRPipeline` | Rend la scène dans une texture RGBA16F, extrait les sources lumineuses, floute le bloom à demi-résolution et compose une image avec tone mapping. |
| `systems/EarthBreakupSystem` | Prépare 32 morceaux de croûte, capture la pose terrestre à la rupture, simule leur mouvement déterministe et rend un noyau émissif avec sa lumière. |
| `tests/earth_breakup.cpp` | Vérifie le seuil, la pose capturée, le mouvement, le reset, le replay, la lumière du noyau et la destruction atteinte par la pluie existante. |
| `systems/EarthDamageSystem` | Consomme les impacts, transforme leurs positions monde en UV locales et accumule les dégâts permanents et la chaleur temporaire dans deux cartes CPU. Refroidit la chaleur et envoie les cartes modifiées aux textures terrestres. |
| `tests/destruction_level.cpp` | Vérifie les contributions de petits/gros impacts, vitesse, accumulation, bornes, entrées invalides, indépendance du refroidissement et replay du niveau global. |
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
- `sun.vert` / `sun.frag` : surface lumineuse du Soleil, émission HDR configurable
  par son matériau (1 par défaut), avec halo produit par le bloom commun.
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
météores. Il n'y a à cette phase aucun cratère géométrique. Captures du test de contact : `/tmp/impact-flash-peak.ppm`,
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
Les valeurs lumineuses dépassent 1 dans le shader. Depuis 4.8.5, elles sont
conservées par la cible HDR et alimentent également le bloom.
Après refroidissement, seule la marque brûlée persiste.

`R` efface et renvoie les deux cartes au GPU. Les tests CPU vérifient le
refroidissement à 30/60/144 FPS, l'accumulation, les limites, la couture, la
rotation et le replay. Les tests OpenGL vérifient les valeurs thermiques
réellement uploadées, le centre jaune-blanc côté nuit, le refroidissement,
le réchauffement par un second impact, les dégâts après refroidissement et
le replay des deux cartes et des images de la cinématique.
Captures : `/tmp/earth-heat-night-peak.ppm`, `/tmp/earth-heat-night-cooled.ppm`,
`/tmp/earth-heat-night-reheated.ppm` et `/tmp/earth-heat-cold-burn.ppm`.

La phase 4.8.5 ajoute la rupture avec fragments préparés, noyau blanc,
éclairage du noyau et bloom, décrits plus bas.

## Niveau de destruction global — phase 4.8.3

`EarthDamageSystem::destructionLevel()` expose un état normalisé entre 0 et 1,
initialement nul, cumulatif et monotone jusqu'au reset. Chaque impact accepté
par le mapping et doté d'une vitesse finie contribue selon une approximation
simple d'énergie : `meteorScale³ * dot(velocity, velocity) / 450`.
Le cube de la taille représente une masse relative ; le carré de la vitesse
représente l'énergie relative. Doubler la taille multiplie la contribution par
8, doubler la vitesse la multiplie par 4. Un impact immobile ne contribue pas
au niveau global. Le budget `DestructionEnergyBudget = 450` est un paramètre
cinématique, sans prétention de simulation géologique.

L'accumulation se fait en double précision et sature à 1. Elle ne dépend ni
des texels déjà brûlés, ni de la chaleur, ni du temps entre les impacts. Des
impacts répétés au même endroit continuent donc d'augmenter le niveau alors
que la marque peut déjà être saturée. Les vitesses non finies ne contribuent
pas au niveau global ; elles conservent le traitement de surface existant.
`update(dt)` ne modifie pas le niveau et `clear()` le remet à zéro, ce qui couvre
`R`, la fermeture et la réinitialisation de l'application.

Le système expose seulement ce signal et ne déclenche pas d'effets lui-même.
Depuis 4.8.4, Application le transmet au matériau terrestre pour les fissures,
sans dépendance inverse vers les systèmes de météores, lumières ou particules.

`make test` vérifie les petits/gros impacts, les vitesses, la progression,
la saturation, les valeurs invalides ou extrêmes, le refroidissement, les
impacts groupés et le replay. `make test-runtime` vérifie aussi la progression
et la relecture exacte du niveau à chaque frame de la cinématique, ainsi que
son reset dans `Application`.

## Fissuration de la croûte — phase 4.8.4

`Application::render` transmet `destructionLevel` au matériau terrestre à
chaque rendu. Le matériau démarre à zéro et le reset de `EarthDamageSystem`
ramène aussi le signal des fissures à zéro au rendu suivant. Le système de
dégâts reste indépendant de l'effet graphique.

Le shader évalue un réseau cellulaire 3D sur la direction sphérique locale
reconstruite depuis les UV (fréquence 8). Les frontières entre cellules forment les fissures.
Le motif est déterministe et ne dépend ni du temps ni des coordonnées monde ;
il reste donc attaché à la croûte pendant la rotation, sans couture U ni
singularité aux pôles. Aucune texture de fissures supplémentaire n'est requise.

Au début, les marques brûlées et leur voisinage favorisent l'apparition des
fissures. Entre 0,18 et 0,8 de destruction, leur couverture devient globale.
Chaque cellule possède un seuil déterministe : le réseau s'ouvre progressivement.
La largeur augmente avec le carré du niveau, avec anti-aliasing par dérivées.
La palette passe de rouge/orange à jaune-blanc, avec une émission croissante
ajoutée après l'éclairage. Les crevasses restent visibles côté nuit ; les dégâts
permanents et la chaleur temporaire continuent de fonctionner ensemble.

La phase 4.8.4 garde la géométrie intacte. Depuis 4.8.5, les fissures, les
impacts et le noyau peuvent alimenter le pipeline HDR et son bloom.

Le test OpenGL vérifie l'absence initiale de fissures, l'augmentation de leur
surface et de leur luminosité à 0,2/0,5/0,8/0,95, l'intérieur jaune-blanc sans
Soleil ni lumière ponctuelle, la stabilité du motif quand Terre et caméra
tournent ensemble, la silhouette et le depth buffer inchangés, les fissures
locales près d'un impact refroidi, la chaleur superposée et le reset. La
cinématique complète vérifie le replay des images avec le signal réel.
Captures : `/tmp/earth-cracks-20.ppm`, `/tmp/earth-cracks-50.ppm`,
`/tmp/earth-cracks-80.ppm`, `/tmp/earth-cracks-95.ppm` et
`/tmp/earth-cracks-local.ppm`.

## Rupture finale, noyau et bloom — phase 4.8.5

`EarthBreakupSystem` prépare la croûte lors du chargement : les triangles de
la sphère terrestre 32 × 32 sont répartis en 32 patches (8 secteurs × 4 bandes).
Chaque morceau conserve exactement les triangles et UV extérieurs d'origine,
possède une coque intérieure à 72 % du rayon et des parois radiales fermant
ses bords. Les meshes sont construits une seule fois avant l'animation ; aucune
fracture dynamique n'est calculée au moment de la rupture.

Quand `destructionLevel >= 0.95`, le système capture la transformation de la
Terre, place tous les morceaux dans cette pose et verrouille son état actif.
La première frame reconstitue la sphère ; les suivantes déplacent les morceaux
vers l'extérieur à 6–10 unités/s et les font tourner. La seed 485 fixe les
vitesses et rotations. La transformation capturée ne suit plus les rotations
de la Timeline. La caméra continue son mouvement normalement.

`Application` masque alors la Terre et les nuages d'origine, puis dessine les
fragments. Les dégâts, la chaleur et les fissures restent attachés aux UV de
leurs faces extérieures. Les faces intérieures utilisent un matériau rocheux
avec éclairage dédié. Le système ne connaît ni météores ni impacts : il reçoit
seulement le niveau global et la transformation terrestre. Le collider des
météores reste l'approximation sphérique d'origine ; aucune collision physique
avec les fragments n'est ajoutée.

Un noyau distinct de rayon local 0,55 reste au centre. Son shader émet à une
intensité HDR de 20, avec centre blanc et périphérie jaune/orange. Une lumière
ponctuelle de puissance 450, publiée après les flashes dans les lumières
temporaires, éclaire les faces exposées des morceaux. La sélection des lumières
GPU conserve le noyau parmi les plus intenses. Les lumières permanentes ne
sont pas dupliquées ou remplacées.

Le budget cinématique de destruction est désormais 450 : les impacts de la
pluie existante atteignent ainsi le seuil de rupture en fin de bombardement
(avec le budget précédent de 1 000, le niveau final restait proche de 0,486).
Le seuil reste centralisé dans `EarthBreakupSystem::Threshold`. La phase 4.9
orchestre désormais la caméra et le rythme du bombardement.

`HDRPipeline` rend toute la scène dans une cible RGBA16F avec profondeur,
extrait les valeurs lumineuses au-dessus de 1, applique huit passes de flou
séparable à demi-résolution et combine le halo à une force de 0,12. La
composition préserve les couleurs ordinaires jusqu’à 0,8 et comprime
progressivement les hautes lumières, sans appliquer une seconde correction
gamma aux textures existantes. Le
depth test masque le noyau derrière les morceaux avant le bloom, qui reste
localisé autour des sources lumineuses. Les buffers sont recréés lors d'un
changement de taille et libérés avant la destruction du contexte OpenGL.

`R` efface les états de rupture et les autres effets, reconstruit la pose de
séquence et restaure la Terre intacte au rendu suivant. Le pipeline n'utilise
pas d'accumulation entre frames : aucun halo résiduel ne survit au reset.

Les tests vérifient le mouvement et le replay CPU, la reconstruction de la
sphère dans le depth buffer à la naissance, les fragments séparés, les valeurs
HDR supérieures à 10, l'effet réel de la lumière du noyau, le bloom sans
blanchiment global, le redimensionnement, le reset après rupture dans
Application et le nettoyage si un shader du noyau ou du bloom manque.
La cinématique complète est rejouée avec rupture et bloom, avec comparaison
des images. Captures : `/tmp/earth-breakup-fragments.ppm`,
`/tmp/earth-breakup-bloom.ppm` et `/tmp/earth-breakup-core-exposed.ppm`.

Le Soleil utilise le même bloom que le noyau, sans géométrie de halo
supplémentaire. Le paramètre `emission` de son matériau vaut 1 et conserve
les détails de sa surface ; `bloomEmission = 3` pilote indépendamment une
source chaude envoyée à une seconde cible RGBA16F. L'extraction combine cette
source avec les hautes lumières ordinaires, puis utilise le même flou et la
même composition que le noyau. Les autres objets écrivent zéro dans cette
cible pour masquer les sources cachées, avec le depth buffer partagé. Cela
permet de régler le halo solaire séparément de sa surface, sans augmenter le bloom global. Les tests
OpenGL comparent aussi les couleurs sombres avant/après composition, l'image
terrestre avec le rendu précédent, et le halo solaire avec/sans bloom. Les
captures de comparaison sont `/tmp/earth-color-legacy.ppm`,
`/tmp/earth-color-corrected.ppm`, `/tmp/sun-without-halo.ppm` et
`/tmp/sun-with-halo.ppm`.

## Séquence finale — phase 4.9

`MainSequence` assemble un film de **110,17 secondes**, dont les 90 premières
conservent le plan de destruction avant le recul sur le système solaire.
La Terre et les nuages tournent lentement (2 et 2,3 degrés/s), pour laisser
lire les impacts attachés à la surface. La caméra garde la Terre au centre,
orbite doucement puis recule pour suivre l'expansion des morceaux.

| Temps | Mise en scène |
| --- | --- |
| 0–8 s | Terre intacte, plan d'installation. |
| 8–24 s | Premiers météores à 0,5/s, échelles 0,15–0,35 ; temps d'approche réel. |
| 24–40 s | Vague à 2/s, échelles 0,18–0,5 ; impacts, brûlures et incandescence. |
| 40–48 s | Vague à 5/s, échelles 0,22–0,75 ; montée du niveau de destruction et fissures. |
| 48–62 s | Vague maximale à 9/s, échelles 0,25–1,1 si la Terre est encore intacte. |
| Après la rupture–90 s | Arrêt des naissances, recul de caméra, fragments autour du noyau blanc et bloom. |
| 90–96 s | Recentrage sur le Soleil et recul à 1 800 unités : vue du système solaire, fond étoilé conservé. |
| 96–100 s | Recul à 6 500 unités, transition tardive vers la Voie lactée. |
| 100–105 s | Recul à 16 000 unités : la galaxie diminue dans le champ. |
| 105–110,17 s | Recul à 35 000 unités et disparition progressive de la galaxie pendant la fin du morceau. |

La zone d'émission est centrée 160 unités au-dessus de la Terre, avec
110 unités d'étendue horizontale et 10 en hauteur. Les météores naissent
hors du cadre des plans cinématiques, puis entrent naturellement dans le
champ. `MeteorShower` peut viser un disque de rayon 8,5 autour du centre
terrestre : les origines variées produisent des approches diagonales depuis
plusieurs directions, sans dépendre de la caméra. Les vitesses de 12–18 et
les durées de vie de 30–34 secondes permettent d'atteindre la Terre.
`setEmission` change la cadence et la plage de tailles sans resemer le RNG
ni perdre le crédit d'émission ; `reset` restaure les valeurs initiales.

Les dégâts, la chaleur et les fissures restent calculés par leurs systèmes.
La rupture reste déclenchée par le seuil de destruction, sans forcer un
instant d'explosion dans la Timeline. Au stepping de 0,25 s testé, elle
survient autour de **52 s**, laissant environ **38 s de plan sur les fragments**
avant le recul solaire. La Terre se déplace désormais sur son orbite ;
la caméra et la zone de naissance suivent sa position.
Le noyau publie alors sa lumière et son émission HDR alimente le bloom
existant. Après rupture, `Application` arrête la pluie et simule les météores
restants avec le collider du noyau (rayon local 0,55), jusqu'à leur contact
ou leur expiration. Le rendu réduit progressivement la taille du météore dans une zone allant
de trois rayons du noyau à sa surface (avec marge pour la taille du météore),
sans modifier sa trajectoire ou sa collision. Un contact absorbe le météore sans inscrire de dégâts
terrestres ni déclencher de nouveaux flashes ou bursts. Le collider utilise
la pose capturée lors de la rupture et ne tient pas compte du halo de bloom. Les 32 fragments
préparés s'éloignent : aucune population supplémentaire de débris n'est créée.
Les particules disparaissent selon leur courte durée de vie. La roche des
météores reste texturée, sans nouvelle fissuration ou fragmentation.

`R` remet à zéro la Timeline, les poses, l'émission, le RNG, les IDs, les
flashes, les particules, les traînées, les cartes GPU de dégâts/chaleur,
le niveau de destruction et la rupture ; la Terre et ses nuages redeviennent
visibles immédiatement. Les tests vérifient les naissances hors champ, les
angles et tailles variés, la progression jusqu'à la rupture et l'égalité
exacte de deux replays avec le même stepping. Les tests OpenGL comparent
également douze images sur les 110,17 secondes, les cartes, les impacts, les
particules et les lumières. Captures : `/tmp/space-start.ppm`, `/tmp/space-middle.ppm`,
`/tmp/space-bombardment.ppm`, `/tmp/space-cracks.ppm`,
`/tmp/space-breakup.ppm`, `/tmp/space-core.ppm`,
`/tmp/space-fragments.ppm`, `/tmp/space-aftermath.ppm`,
`/tmp/solar-system.ppm`, `/tmp/milky-way.ppm`,
`/tmp/milky-way-small.ppm` et `/tmp/milky-way-gone.ppm`.
Le temps de rupture peut légèrement varier avec
un autre stepping ; tous les resets à stepping identique rejouent le même film.

## Système solaire, Lune et Voie lactée

Les huit planètes utilisent les textures existantes et une sphère partagée.
Le Soleil reste émissif. Les nouvelles planètes et la Lune utilisent un
éclairage dirigé vers la position du Soleil ; la Terre conserve son matériau,
ses cartes de dégâts et sa lumière directionnelle actualisée. Saturne possède
un anneau transparent texturé et incliné. Les guides orbitaux réutilisent un
mesh annulaire commun, avec une opacité qui augmente lors du recul solaire
puis disparaît entre 12 000 et 24 000 unités dans le plan galactique.

Les tailles et distances sont comprimées pour rester lisibles :

| Corps | Rayon visible | Rayon orbital autour du Soleil |
| --- | ---: | ---: |
| Soleil | 50 | — |
| Mercure | 3,8 | 120 |
| Vénus | 9,5 | 220 |
| Terre | 10 | 320 |
| Mars | 5,3 | 440 |
| Jupiter | 28 | 600 |
| Saturne | 23 | 760 |
| Uranus | 17 | 920 |
| Neptune | 16,5 | 1 080 |
| Lune | 2,7 | 28 autour de la Terre |

Les orbites de `SolarSystem` sont analytiques et déterministes : les planètes
intérieures se déplacent plus vite que les extérieures. La Terre démarre à
sa position historique `(30,50,0)` ; le Soleil est placé à `(-290,50,0)`.
Une piste de la Timeline met à jour les poses avant la caméra et les naissances.
Après la rupture, la position terrestre reste capturée pour le noyau et les
fragments ; les autres planètes poursuivent leur orbite. La Lune perd son
orbite terrestre et conserve sa vitesse tangentielle, plus la vitesse
orbitale de la Terre. Elle dérive alors en ligne droite, sans simulation de
la gravitation solaire ou des collisions de débris. Son guide orbital et
celui de la Terre disparaissent. `R` restaure toutes les poses et l'orbite lunaire.

Le fond fait une transition selon la distance au Soleil, y compris en mode
FPS : `2k_stars.jpg` vers `2k_stars_milky_way.jpg` entre 3 000 et 5 000 unités,
puis vers `2k_milky_way.jpg` entre 5 000 et 7 000 unités. Le premier fond
n'est plus visible après la transition. La texture de galaxie carrée est
projetée dans une direction fixe du ciel, plutôt qu'étirée sur les UV de la
sphère ; elle ne suit donc pas la rotation de la caméra. Sa taille angulaire
diminue avec la distance (`5000 / distance`), puis son opacité décroît entre
18 000 et 32 000 unités jusqu’à zéro, sans ramener l’ancien fond étoilé. Le ciel est rendu
avant les éléments transparents pour laisser visibles anneaux et orbites.

Les tests CPU vérifient les orbites, la libération de la Lune et le replay.
Les tests OpenGL vérifient les anneaux et remplacent temporairement la
texture du fond étoilé pour prouver qu'elle ne contribue plus au plan éloigné.
Ils comparent les douze images de deux replays complets, dont les plans solaires.
Captures supplémentaires : `/tmp/saturn-rings.ppm`, `/tmp/solar-system.ppm`
et `/tmp/milky-way.ppm`.

## Musique et synchronisation

Le morceau fourni est `music/Can You Hear The Music.mp3`. Make le convertit
avec FFmpeg en PCM stéréo 48 kHz dans `build/music/cinematic.wav` (110,165625 s).
Le MP3 reste intact ; le fichier WAV est généré et disparaît avec `make clean`.
La lecture utilise SDL3, sans nouveau processus lancé pendant le film.
SDL3 (bibliothèque de développement et fichier pkg-config `sdl3`) et FFmpeg
sont requis en plus des dépendances graphiques.

L'application visible démarre la musique au début de `run`. Chaque frame
avance la simulation jusqu'à la position consommée par le flux audio :
le film suit donc le morceau plutôt que d'utiliser deux horloges indépendantes.
La durée de la Timeline est ajustée à celle du PCM chargé. Les 90 premières
secondes conservent le bombardement et le plan des fragments ; le recul
solaire et la révélation galactique occupent la dernière montée, puis la
galaxie diminue et s'efface pendant la fin calme du morceau.
Les repères de montage restent centralisés dans `MainSequence`.

`R` rembobine le flux audio et réinitialise la totalité du film. `M` coupe ou
rétablit le son, sans arrêter l'horloge de lecture. Le gain initial est 0,7.
À la fermeture, le flux et le sous-système audio sont libérés. En l'absence
d'audio disponible, la séquence utilise le temps de frame habituel.
Les applications masquées et les tests graphiques n'ouvrent pas de sortie
sonore ; les replays restent comparés avec un stepping fixe. Le test
`music_player` utilise le périphérique SDL `dummy` pour vérifier la durée,
l'avancement monotone, le rewind, le mute et la récupération après erreur.

Pour changer un mouvement ou un événement, modifier `MainSequence`. Pour changer
les objets, leurs textures ou leurs matériaux, modifier `SceneSetup` et, si
nécessaire, `SceneResources`. Pour changer le fonctionnement du rendu, modifier
`Renderer` ou les shaders.

Les ressources GPU sont construites après la création du contexte OpenGL.
À la fermeture, `Application` libère les pistes, les objets et les ressources
ainsi que les ressources de rupture et de post-traitement avant de détruire la fenêtre et de terminer GLFW.

## Compilation et inclusions

### Commandes

- `M` : couper ou rétablir la musique sans interrompre le film.
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
make test             # tests CPU et audio (périphérique dummy), sans fenêtre
make test-sequence    # seulement le test de séquence
make test-runtime     # test OpenGL masqué, nécessite un affichage X11
make clean            # supprime build/ et l'exécutable project
```
