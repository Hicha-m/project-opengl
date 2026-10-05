# Audit et refactorisation du projet

Périmètre : le dossier `projet/`, le 5 octobre 2026. Les exercices du cours et
les anciennes versions situés à côté ne font pas partie de cet exécutable.
L’analyse porte sur les sources, leurs références dans l’application et les
tests, les shaders, les ressources chargées et les outils de compilation/export.

## Code et ressources inutilisés

Tous les fichiers `.cpp` de `src/` participent à la compilation ; tous les
headers sont inclus par l’application ou ses tests. Aucun fichier C++ entier
sans usage n’a été identifié. Cela ne signifie pas que chaque API générique
est utilisée par la cinématique.

| Élément | Constat et traitement |
| --- | --- |
| `Scene::getObject` | Aucun appel ; méthode retirée. La recherche par nom est utilisée. |
| `LightManager::clearPointLights` | Aucun appel ; méthode retirée. La réinitialisation remplace le gestionnaire complet. |
| `SceneResources::sunSphere`, `starSphere` | Même rayon et même tessellation que `earthSphere` ; une géométrie partagée suffit. |
| Journal `[DEBUG]` dans `Application::simulate` | Écrivait chaque seconde sans option de diagnostic ; retiré avec son compteur. F2 conserve les informations de caméra dans le titre. |
| Entrées GLSL inutilisées | `FragPos` des nuages, `FragPos`/`Normal` du fragment des orbites et attribut normal du Soleil/fond étoilé retirés. |
| Première affectation de `bloomSource` dans `sun.frag` | Toujours écrasée ; retirée. |
| `textures/planetes/2k_venus_surface.jpg` | Jamais chargée : Vénus utilise `2k_venus_atmosphere.jpg`. Archivée. |
| `models/shuttle/metalnessMap2.png` à `metalnessMap5.png` | Référencées dans le MTL original, mais les directives `map_Pm`, `map_Pr`, `map_Ka` ne sont jamais lues par le chargeur. Archivées, références retirées du MTL actif. Les coefficients `Pm`/`Pr` utilisés sont conservés. |

Les cinq textures et le MTL original sont sauvegardés dans
`../../archives/refactor-2026-10-05/`, avec les instructions de restauration.
Le nettoyage retire environ 30 Mo de ressources du projet actif.

Quelques parties génériques sont conservées et signalées explicitement :

- `OrbitCamera`, dans les fichiers Camera du cours, n’est jamais instanciée.
  La cinématique emploie `CinematicCamera`. Les quatre classes du cours sont
  conservées sans modification pour cette refactorisation.
- `TransformTrack::scaleTrack` n’est pas utilisée ; la piste de position sert
  aux tests et celle de rotation à la cinématique. Les trois pistes forment
  une interface cohérente pour animer une transformation.
- `Scene::getObjectCount`, plusieurs accesseurs d’état et les paramètres de
  configuration des émetteurs servent surtout aux tests. Leur rôle est de
  vérifier la simulation et sa reproductibilité.
- Le mode de déplacement libre de `CinematicCamera` est exercé dans les tests ;
  la séquence principale utilise son mode orbital.
- La surcharge `ShaderProgram::setUniform` pour `vec4` et les déplacements
  privés déclarés dans `Texture2D` ne sont pas utilisés actuellement. Ils
  appartiennent aux classes techniques conservées.

Les dossiers `build/`, les objets compilés, les caches Python, `.DS_Store` et
l’exécutable `project` sont des fichiers générés, déjà ignorés par Git. Ils
ne constituent pas du code à présenter. Les vidéos dans `exports/` sont des
résultats de travail, conservés.

## Responsabilités des fichiers

Les lignes ci-dessous regroupent les couples `.h`/`.cpp` lorsqu’ils existent.

| Fichiers ou groupe | Responsabilité |
| --- | --- |
| `main.cpp` | Lire les options puis lancer l’application, le test de démarrage ou l’export. |
| `Application` | Posséder les ressources, gérer la fenêtre et les commandes, coordonner simulation/rendu/son. |
| `Scene`, `SceneObject`, `Transform`, `Material` | Décrire les objets et leurs propriétés visuelles. |
| `SceneResources`, `SceneSetup` | Posséder les ressources OpenGL et assembler la scène initiale. |
| `LightManager` | Fournir l’éclairage solaire et les lumières ponctuelles aux shaders. |
| `Keyframe`, `AnimationTrack`, `Easing` | Décrire et interpoler les valeurs animées. |
| `Timeline`, `TimelineTrack` | Fournir le temps commun et l’interface des pistes. |
| `TransformTrack`, `CameraTrack`, `EventTrack` | Appliquer les transformations et les poses, déclencher les événements. |
| `MainSequence`, `CinematicCamera` | Décrire la progression du film et évaluer les poses animées. |
| `Sphere`, `SphereCollider` | Construire les sphères et décrire les cibles de collision. |
| `Meteor`, `MeteorImpact`, `MeteorResources` | Données des météores, contacts et ressources graphiques partagées. |
| `MeteorShower`, `MeteorSystem` | Générer la pluie puis simuler les météores et leurs collisions. |
| `Particle`, `ParticleSystem`, `ParticleEmitter` | Données, durée de vie et émission générique de particules. |
| `ImpactParticleEmitter`, `MeteorTrailEmitter` | Traduire les impacts ou les trajectoires en particules. |
| `ImpactLight`, `ImpactLightSystem` | Produire et faire disparaître les éclairs d’impact. |
| `EarthDamageSystem`, `EarthBreakupSystem` | Accumuler dégâts/chaleur puis animer la rupture. |
| `SolarSystem` | Calculer les orbites et la dérive de la Lune après destruction. |
| `Renderer`, `ParticleRenderer`, `HDRPipeline` | Dessiner les objets, les particules et la composition HDR/bloom. |
| `MusicPlayer`, `ResourcePaths` | Lire le son et retrouver les ressources depuis l’installation. |
| Camera, Mesh, ShaderProgram, Texture2D | Bases techniques du cours, exclues de la présentation. |
| `shaders/` | Calculs visuels effectivement chargés : astres, navette, effets et post-traitement. |
| `tests/` | Vérifier les modules sur CPU, l’audio et l’intégration avec OpenGL. |

## Outils utiles, hors présentation

| Fichier | Usage |
| --- | --- |
| `CMakeLists.txt` | Compilation multiplateforme, tests et paquets distribuables. |
| `Makefile` | Compilation et tests locaux sur Linux. La liste des tests sert maintenant aussi à leur exécution. |
| `vcpkg.json` | Dépendances de la compilation Windows. |
| `Dockerfile`, `.dockerignore` | Compilation et exécution en conteneur Linux. |
| `.github/workflows/cmake-multi-platform.yml` | Vérifications et création des paquets dans la CI. |
| `scripts/build_osmesa.sh` | Fournir OpenGL logiciel aux tests macOS de la CI. |
| `scripts/verify_package.py` | Tester le paquet extrait, déplacé dans un chemin Unicode. |
| `tools/export_mp4.py` | Exporter le film avec FFmpeg et mixer les impacts. |
| `third_party/` | Chargeur GLAD, lecture d’images stb et notices des dépendances. |
| `docs/MOBILE.md` | Expliquer les limites du portage mobile ; documentation, sans effet à l’exécution. |
| `README.md`, `image.gif` | Instructions et aperçu du projet. |
| `.gitignore`, `.gitattributes`, `.clang-format` | Ignorer les résultats générés, régler les fichiers texte et fixer le style. |

## Changements de lisibilité

- `SceneSetup::build` expose les étapes d’assemblage ; les fonctions auxiliaires
  restent dans le même fichier pour ne pas multiplier les modules.
- `MainSequence::build` sépare configuration, pistes et événements.
- `Application::simulate` conserve l’ordre des systèmes ; l’envoi des effets
  d’impact et la mise à jour du Soleil ont des fonctions nommées.
- `main` délègue la lecture des arguments et les modes optionnels à des fonctions locales.
- `AnimationTrack` construit directement les images clés et expose le calcul d’interpolation.
- Les paramètres de temps utilisent `deltaTime` ; les variables de particules,
  planètes et configurations ont des noms explicites.
- Le C++ ajouté, les tests et les shaders utilisent le même format : quatre
  espaces, accolades explicites, lignes limitées à 110 colonnes, accesseurs courts.
  Les dépendances et les quatre classes du cours ne sont pas reformatées.
- Le Makefile inclut désormais `resource_paths` dans ses tests, comme CMake,
  et conserve les assertions même avec une compilation utilisant `NDEBUG`.

Le fichier `tests/application.cpp` reste volumineux : il contient de nombreux
scénarios graphiques nommés, tous appelés. Sa taille ne signifie pas qu’il est
inutile. Il n’est pas à parcourir pendant l’exposé. Les calculs de fragmentation
et de cadrage restent les parties les plus techniques de l’application.

## Vérification

Résultats après refactorisation :

- Compilation CMake Release et compilation GNU Make : réussies, sans avertissement.
- Les 14 tests CPU/audio/ressources passent avec CTest et avec `make test`.
- Test de démarrage : chargement de la scène, rendu et initialisation audio réussis avec OSMesa.
- Suite graphique complète : réussie avec
  `SDL_AUDIO_DRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 build/cmake/test_application --software-context`.
  Cette option a été ajoutée au test pour réutiliser le contexte logiciel existant.
- Export MP4 de contrôle : une seconde, 320 × 180, quatre images, flux H.264 et AAC vérifiés.
- Format vérifié avec `clang-format --dry-run --Werror` et différences contrôlées avec `git diff --check`.
- Sur les 76 fichiers dont le changement concerne uniquement la présentation
  du code, les tokens restent identiques après exclusion des commentaires,
  de l’ordre des includes et des accolades ajoutées.

L’affichage X11/Wayland étant inaccessible dans cette session, les vérifications
graphiques ont utilisé OpenGL logiciel. Les tests vérifient les pixels et les
états OpenGL ; l’audio avec le pilote `dummy` vérifie le fonctionnement logiciel,
sans vérifier le son d’un haut-parleur. Les tests de récupération retirent
volontairement des ressources et produisent des messages d’erreur attendus.
