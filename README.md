[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/4B2l0wK5)
# NAAA-No-Afterlife-At-All-
UWE Game Engine Programming Scarle 2021

Yat Long Chan 22025398
1. Menu
In the menu a title text "DirectX Game" is shown. Press Enter key to start game.

2. Controlling
In the game, use W, A, S, D and mouse (third person mode only) to control the player character's movement. Use G to toggle first/third person camera. Use E to shoot bullet.

3. UI
The current score, number of bullets got and enemy spawning interval is shown at the side.

4. Main game
There are some collider objects spawned randomly on the ground. They can only be collided by player but not enemies or bullets.

Player character will be spawned at a random position in the game world. An enemy will be spawned shortly after player is spawned. Shoot bullets to deal damage on enemy. A stronger new enemy will be spawned after the previous one is defeated.

5. Losing
When any enemy character touches your character, you lose.   

6. Winning
After defeating 5 enemies, you will "win" the game. Press Enter to continue playing in endless mode.

7. Endless mode
The enemy will keep spawning faster and faster. All enemies are set to be killed in one shot. Hint: keep running when the spawn rate goes high.

8. Score
Score gain by defeating each enemy is 20 * it's difficulty level (1 ~ 5).
