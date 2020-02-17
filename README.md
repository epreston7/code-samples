# code-samples
A selection of code samples for review. These are snippets of code I am particularly proud of, and feel demonstrate my abilities well.

__Advanced Programming for Games:__
 * _puzzle.cpp:_ Tasked to solve a 15-Puzzle for continuous and partial-continuous rows and columns, this cpp file contains the methods used to solve a given configuration.


__Advanced Graphics for Games:__
* _renderer.cpp:_ The renderer file from my coursework, where I created a landscape that changed over time.
* _bumpFragment.glsl:_ Fragment shader for the point light and spotlight calculations, alongside the textures for the heightmap. Light calculations use the Phong Lighting model, so the final fragment colour uses ambient, diffuse, specular and attenuation components. The heightmap textures mix together, as well as their respective bump maps.
* _bumpVertex.glsl:_ The associated vertex shader to the fragment shader above. Includes a uniform float that increases over time, and this controls how high the heightmap y coordinates are.

__Advanced Game Technologies:__
* _CourseworkGame.cpp:_ Includes following notable methods-
  * Move the player.
  * Set up pathfinding AI and move the character to the player if a path is found.
  * Set up networking components, and send position information across the network.
  * Set up AI states, and update the AI which uses this finite state machine to chase the goose.
  * Debug mode which uses raycasting.
* _CourseworkGame.h:_ Header file for the cpp file above.
* _Goose.h:_ Goose class that inherits from the GameObject class. The player controls the goose; this class describes specific behaviour that happens when a collision starts or ends, and what happens whilst the goose stays colliding. It makes use of the collision layers given when an object is instantiated.

__Engineering Gaming Solutions within a Team:__
These scripts are C# scripts written for my fast prototype of the team project game made in Unity. The project was a golf game where the fundamentals can be changed by the player (a mixture of What the Golf, and Baba is You)
* _bhColl.cs:_ Collision behaviour for the golf ball.
* _golfBallMovement.cs:_ How the golf ball moves, using the mouse click to decide direction and how long the button is down to determine the power of the hit.
* _snowballStick.cs:_ Adds a fixed joint to the golf ball and 'sticky snowball' that a snowman throws.


